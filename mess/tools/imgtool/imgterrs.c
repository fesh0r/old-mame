#include <assert.h>
#include "imgterrs.h"

static const char *msgs[] =
{
	"Out of memory",
	"Unexpected error",
	"Argument too long",
	"Read error",
	"Write error",
	"Image is read only",
	"Corrupt image",
	"File not found",
	"Unrecognized format",
	"Not implemented",
	"Parameter too small",
	"Parameter too large",
	"Missing parameter not found",
	"Inappropriate parameter",
	"Invalid parameter",
	"Bad file name",
	"Out of space on image",
	"Input past end of file"
};

const char *imgtool_error(imgtoolerr_t err)
{
	err = ERRORCODE(err) - 1;
	assert(err >= 0);
	assert(err < (sizeof(msgs) / sizeof(msgs[0])));
	return msgs[err];
}
