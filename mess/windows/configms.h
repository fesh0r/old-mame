#ifndef CONFIGMS_H
#define CONFIGMS_H

#include "rc.h"

extern struct rc_option mess_opts[];
extern int win_write_config;

int write_config (const char* filename, const struct GameDriver *gamedrv);

#endif /* CONFIGMS_H */

