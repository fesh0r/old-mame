#include "emu.h"
#include "mfi_dsk.h"
#include <zlib.h>

/*
  Mess floppy image structure:

  - header with signature, number of cylinders, number of heads.  Min
    track and min head are considered to always be 0.

  - vector of track descriptions, looping on cylinders and sub-lopping
    on heads, each description composed of:
    - offset of the track data in bytes from the start of the file
    - size of the compressed track data in bytes (0 for unformatted)
    - size of the uncompressed track data in bytes (0 for unformatted)

  - track data

  All values are 32-bits lsb first.

  Track data is zlib-compressed independently for each track using the
  simple "compress" function.

  Track data consists of a series of 32-bits lsb-first values
  representing magnetic cells.  Bits 0-27 indicate the sizes, and bits
  28-31 the types.  Type can be:
  - 0, MG_A -> Magnetic orientation A
  - 1, MG_B -> Magnetic orientation B
  - 2, MG_N -> Non-magnetized zone (neutral)
  - 3, MG_D -> Damaged zone, reads as neutral but cannot be changed by writing

  Remember that the fdcs detect transitions, not absolute levels, so
  the actual physical significance of the orientation A and B is
  arbitrary.

  Tracks data is aligned so that the index pulse is at the start,
  whether the disk is hard-sectored or not.

  The size is the angular size in units of 1/200,000,000th of a turn.
  Such a size, not coincidentally at all, is also the flyover time in
  nanoseconds for a perfectly stable 300rpm drive.  That makes the
  standard cell size of a MFM 3.5" DD floppy at 2000 exactly for
  instance (2us).  Smallest expected cell size is 500 (ED density
  drives).

  The sum of all sizes must of course be 200,000,000.

  An unformatted track is equivalent to one big MG_N cell covering a
  whole turn, but is encoded as zero-size.

  TODO: big-endian support, cleanup pll, move it where it belongs.
*/

const char mfi_format::sign[16] = "MESSFLOPPYIMAGE"; // Includes the final \0

mfi_format::mfi_format() : floppy_image_format_t()
{
}

const char *mfi_format::name() const
{
	return "mfi";
}

const char *mfi_format::description() const
{
	return "MESS floppy image";
}

const char *mfi_format::extensions() const
{
	return "mfi";
}

bool mfi_format::supports_save() const
{
	return false;
}

int mfi_format::identify(floppy_image *image)
{
	header h;

	image->image_read(&h, 0, sizeof(header));
	if(memcmp( h.sign, sign, 16 ) == 0 &&
	   h.cyl_count > 0 && h.cyl_count <= 84 &&
	   h.head_count > 0 && h.head_count <= 2)
		return 100;
	return 0;
}

bool mfi_format::load(floppy_image *image)
{
	header h;
	entry entries[84*2];
	image->image_read(&h, 0, sizeof(header));
	image->image_read(&entries, sizeof(header), h.cyl_count*h.head_count*sizeof(entry));
	image->set_meta_data(h.cyl_count, h.head_count);

	UINT8 *compressed = 0;
	int compressed_size = 0;

	entry *ent = entries;
	for(unsigned int cyl=0; cyl != h.cyl_count; cyl++)
		for(unsigned int head=0; head != h.head_count; head++) {
			if(ent->uncompressed_size == 0) {
				// Unformatted track
				image->set_track_size(cyl, head, 0);
				continue;
			}

			if(ent->compressed_size > compressed_size) {
				if(compressed)
					global_free(compressed);
				compressed_size = ent->compressed_size;
				compressed = global_alloc_array(UINT8, compressed_size);
			}

			image->image_read(compressed, ent->offset, ent->compressed_size);

			unsigned int cell_count = ent->uncompressed_size/4;
			image->set_track_size(cyl, head, cell_count);
			UINT32 *trackbuf = image->get_buffer(cyl, head);

			uLongf size = ent->uncompressed_size;
			if(uncompress((Bytef *)trackbuf, &size, compressed, ent->compressed_size) != Z_OK)
				return true;

			UINT32 cur_time = 0;
			for(unsigned int i=0; i != cell_count; i++) {
				UINT32 next_cur_time = cur_time + (trackbuf[i] & TIME_MASK);
				trackbuf[i] = (trackbuf[i] & MG_MASK) | cur_time;
				cur_time = next_cur_time;
			}
			if(cur_time != 200000000)
				return true;

			ent++;
		}

	if(compressed)
		global_free(compressed);

	return false;
}

const floppy_format_type FLOPPY_MFI_FORMAT = &floppy_image_format_creator<mfi_format>;
