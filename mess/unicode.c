/*********************************************************************

	unicode.c

	Unicode related functions

*********************************************************************/

#include "unicode.h"

int uchar_from_utf8(unicode_char_t *uchar, const char *utf8char, size_t count)
{
	unicode_char_t c, minchar;
	int auxlen, i;
	char auxchar;

	if (!utf8char || !count)
		return 0;

	c = (unsigned char) *utf8char;
	count--;
	utf8char++;

	if (c < 0x80)
	{
		/* unicode char 0x00000000 - 0x0000007F */
		c &= 0x7f;
		auxlen = 0;
		minchar = 0x00000000;
	}
	else if ((c >= 0xc0) & (c < 0xe0))
	{
		/* unicode char 0x00000080 - 0x000007FF */
		c &= 0x1f;
		auxlen = 1;
		minchar = 0x00000080;
	}
	else if ((c >= 0xe0) & (c < 0xf0))
	{
		/* unicode char 0x00000800 - 0x0000FFFF */
		c &= 0x0f;
		auxlen = 2;
		minchar = 0x00000800;
	}
	else if ((c >= 0xf0) & (c < 0xf8))
	{
		/* unicode char 0x00010000 - 0x001FFFFF */
		c &= 0x07;
		auxlen = 3;
		minchar = 0x00010000;
	}
	else if ((c >= 0xf8) & (c < 0xfc))
	{
		/* unicode char 0x00200000 - 0x03FFFFFF */
		c &= 0x03;
		auxlen = 4;
		minchar = 0x00200000;
	}
	else if ((c >= 0xfc) & (c < 0xfe))
	{
		/* unicode char 0x04000000 - 0x7FFFFFFF */
		c &= 0x01;
		auxlen = 5;
		minchar = 0x04000000;
	}
	else
	{
		/* invalid */
		return -1;
	}

	/* exceeds the count? */
	if (auxlen > count)
		return -1;

	/* we now know how long the char is, now compute it */
	for (i = 0; i < auxlen; i++)
	{
		auxchar = utf8char[i];

		/* all auxillary chars must be between 0x80-0xbf */
		if ((auxchar & 0xc0) != 0x80)
			return -1;

		c = c << 6;
		c |= auxchar & 0x3f;
	}

	/* make sure that this char is above the minimum */
	if (c < minchar)
		return -1;

	*uchar = c;
	return auxlen + 1;
}



int utf16_from_uchar(utf16_char_t *utf16string, size_t count, unicode_char_t uchar)
{
	int rc;
	if ((uchar >= 0xd800) && (uchar <= 0xdfff))
		return -1;

	if (uchar < 0x10000)
	{
		if (count < 1)
			return -1;
		utf16string[0] = (utf16_char_t) uchar;
		rc = 1;
	}
	else if (uchar < 0x100000)
	{
		if (count < 2)
			return -1;
		utf16string[0] = ((uchar >> 10) & 0x03ff) | 0xd800;
		utf16string[1] = ((uchar >>  0) & 0x03ff) | 0xdc00;
		rc = 2;
	}
	else
		return -1;
	return rc;
}



