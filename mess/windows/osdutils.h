#ifdef UNDER_CE
#include <string.h>
#endif

#define strcmpi		stricmp
#define strncmpi	strnicmp

/* some irritating redefinitions */
#define read	read_
#define write	write_
#define access	access_
#define tell	tell_

#define tcsicmp		_tcsicmp
#define tcsnicmp	_tcsnicmp
#define tcslen		_tcslen

#define PATH_SEPARATOR	'\\'

#define EOLN "\r\n"
