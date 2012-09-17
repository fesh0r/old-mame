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

    formats/xdf_dsk.c

    x68k bare-bones formats

*********************************************************************/

#include "emu.h"
#include "formats/xdf_dsk.h"

xdf_format::xdf_format() : upd765_format(formats)
{
}

const char *xdf_format::name() const
{
	return "xdf";
}

const char *xdf_format::description() const
{
	return "XDF disk image";
}

const char *xdf_format::extensions() const
{
	return "xdf,hdm,2hd";
}

// Unverified gap sizes
const xdf_format::format xdf_format::formats[] = {
	{
		floppy_image::FF_525, floppy_image::DSHD,
		1200, // 1us, 360rpm
		8, 77, 2,
		1024, {},
		1, {},
		80, 50, 22, 84
	},
	{}
};

const floppy_format_type FLOPPY_XDF_FORMAT = &floppy_image_format_creator<xdf_format>;
