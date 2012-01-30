#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "nextmo.h"

const device_type NEXTMO = &device_creator<nextmo_device>;

DEVICE_ADDRESS_MAP_START(map, 8, nextmo_device)
	AM_RANGE(0x04, 0x04) AM_READWRITE(r4_r, r4_w)
	AM_RANGE(0x05, 0x05) AM_READWRITE(r5_r, r5_w)
	AM_RANGE(0x06, 0x06) AM_READWRITE(r6_r, r6_w)
	AM_RANGE(0x07, 0x07) AM_READWRITE(r7_r, r7_w)
	AM_RANGE(0x08, 0x08) AM_READWRITE(r8_r, r8_w)
	AM_RANGE(0x09, 0x09) AM_READWRITE(r9_r, r9_w)
	AM_RANGE(0x0a, 0x0a) AM_READWRITE(ra_r, ra_w)
	AM_RANGE(0x0b, 0x0b) AM_READWRITE(rb_r, rb_w)
	AM_RANGE(0x10, 0x17) AM_READWRITE(r10_r, r10_w)
ADDRESS_MAP_END

nextmo_device::nextmo_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
        : device_t(mconfig, NEXTMO, "NeXT Magneto-optical drive", tag, owner, clock)
{
}

void nextmo_device::set_cb(cb_t _irq_cb, cb_t _drq_cb)
{
	irq_cb = _irq_cb;
	drq_cb = _drq_cb;
}

void nextmo_device::device_start()
{
}

void nextmo_device::device_reset()
{
}

READ8_MEMBER(nextmo_device::r4_r)
{
	fprintf(stderr, "nextmo: r4_r %02x (%08x)\n", r4, cpu_get_pc(&space.device()));
	return r4;
}

WRITE8_MEMBER(nextmo_device::r4_w)
{
	if(r4 & 1)
		device_reset();
	r4 = (r4 & (~data & 0xfc)) | (data & 3);
	fprintf(stderr, "nextmo: r4_w %02x (%08x)\n", r4, cpu_get_pc(&space.device()));
}

READ8_MEMBER(nextmo_device::r5_r)
{
	fprintf(stderr, "nextmo: r5_r %02x (%08x)\n", r5, cpu_get_pc(&space.device()));
	return r5;
}

WRITE8_MEMBER(nextmo_device::r5_w)
{
	r5 = data;
	fprintf(stderr, "nextmo: r5_w %02x (%08x)\n", r5, cpu_get_pc(&space.device()));
}

READ8_MEMBER(nextmo_device::r6_r)
{
	fprintf(stderr, "nextmo: r6_r %02x (%08x)\n", r6, cpu_get_pc(&space.device()));
	return r6;
}

WRITE8_MEMBER(nextmo_device::r6_w)
{
	r6 = data;
	fprintf(stderr, "nextmo: r6_w %02x (%08x)\n", r6, cpu_get_pc(&space.device()));
}

READ8_MEMBER(nextmo_device::r7_r)
{
	fprintf(stderr, "nextmo: r7_r %02x (%08x)\n", r7, cpu_get_pc(&space.device()));
	return r7;
}

WRITE8_MEMBER(nextmo_device::r7_w)
{
	r7 = data;
	fprintf(stderr, "nextmo: r7_w %02x (%08x)\n", r7, cpu_get_pc(&space.device()));
	if(r7 & 0xc0) {
		fprintf(stderr, "nextmo: start dma %02x %02x\n", r6, r7);
		sector_pos = 0;
		if(!drq_cb.isnull())
			drq_cb(true);
	}
}

UINT8 nextmo_device::dma_r()
{
	UINT8 r = 0;
	if(r7 & 0x80)
		r = sector[sector_pos++];
	check_dma_end();
	return r;
}

void nextmo_device::dma_w(UINT8 data)
{
	if(r7 & 0x40)
		sector[sector_pos++] = data;
	check_dma_end();
}

void nextmo_device::check_dma_end()
{
	int limit;
	if(r7 & 0x40)
		limit = r6 & 0x20 ? 0x510 : 0x400;
	else
		limit = r6 & 0x20 ? 0x400 : 0x510;
	if(sector_pos == limit) {
		if(!drq_cb.isnull())
			drq_cb(false);
		if(r7 & 0x40) {
			if(r6 & 0x20)
				check_ecc();
			else
				compute_ecc();
		}
		r7 &= 0x3f;
		r4 |= 0x08;
	}
}

READ8_MEMBER(nextmo_device::r8_r)
{
	fprintf(stderr, "nextmo: r8_r (%08x)\n", cpu_get_pc(&space.device()));
	return 0x00;
}

WRITE8_MEMBER(nextmo_device::r8_w)
{
	fprintf(stderr, "nextmo: r8_w %02x (%08x)\n", data, cpu_get_pc(&space.device()));
}

READ8_MEMBER(nextmo_device::r9_r)
{
	fprintf(stderr, "nextmo: r9_r (%08x)\n", cpu_get_pc(&space.device()));
	return 0x00;
}

WRITE8_MEMBER(nextmo_device::r9_w)
{
	fprintf(stderr, "nextmo: r9_w %02x (%08x)\n", data, cpu_get_pc(&space.device()));
}

READ8_MEMBER(nextmo_device::ra_r)
{
	fprintf(stderr, "nextmo: ra_r (%08x)\n", cpu_get_pc(&space.device()));
	return 0x00;
}

WRITE8_MEMBER(nextmo_device::ra_w)
{
	fprintf(stderr, "nextmo: ra_w %02x (%08x)\n", data, cpu_get_pc(&space.device()));
}

READ8_MEMBER(nextmo_device::rb_r)
{
	fprintf(stderr, "nextmo: rb_r (%08x)\n", cpu_get_pc(&space.device()));
	return 0x24;
}

WRITE8_MEMBER(nextmo_device::rb_w)
{
	fprintf(stderr, "nextmo: rb_w %02x (%08x)\n", data, cpu_get_pc(&space.device()));
}

READ8_MEMBER(nextmo_device::r10_r)
{
	fprintf(stderr, "nextmo: r10_r %d (%08x)\n", offset, cpu_get_pc(&space.device()));
	return 0x00;
}

WRITE8_MEMBER(nextmo_device::r10_w)
{
	fprintf(stderr, "nextmo: r10_w %d, %02x (%08x)\n", offset, data, cpu_get_pc(&space.device()));
}

void nextmo_device::check_ecc()
{
	fprintf(stderr, "nextmo: check_ecc\n");
	for(int i=0; i<0x400; i++)
		sector[i] = i;
}

void nextmo_device::compute_ecc()
{
	fprintf(stderr, "nextmo: compute_ecc\n");
	memset(sector+0x400, 0, 0x110);
}
