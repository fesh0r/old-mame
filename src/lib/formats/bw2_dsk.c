/***************************************************************************

    Copyright Olivier Galibert
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in
          the documentation and/or other materials provided with the
          distribution.
        * Neither the name 'MAME' nor the names of its contributors may be
          used to endorse or promote products derived from this software
          without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY AARON GILES ''AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL AARON GILES BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
    STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
    IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

****************************************************************************/

/*********************************************************************

    formats/bw2_dsk.c

    bw2 format

*********************************************************************/

#include "emu.h"
#include "formats/bw2_dsk.h"

bw2_format::bw2_format() : wd177x_format(formats)
{
}

const char *bw2_format::name() const
{
	return "bw2";
}

const char *bw2_format::description() const
{
	return "Bondwell 2 disk image";
}

const char *bw2_format::extensions() const
{
	return "dsk";
}

// Unverified gap sizes
const bw2_format::format bw2_format::formats[] = {
	{   /*  340K 3 1/2 inch double density */
		floppy_image::FF_35,  floppy_image::SSDD,
		2000, 17, 80, 1, 256, {}, 0, {}, 100, 22, 20
	},
	{   /*  360K 3 1/2 inch double density */
		floppy_image::FF_35,  floppy_image::SSDD,
		2000, 18, 80, 1, 256, {}, 0, {}, 100, 22, 20
	},
	{}
};

const floppy_format_type FLOPPY_BW2_FORMAT = &floppy_image_format_creator<bw2_format>;
