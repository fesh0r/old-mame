#ifndef CONFIGMS_H
#define CONFIGMS_H

#include "rc.h"

#define IMAGE_SEPARATOR		'|'


extern struct rc_option mess_opts[];
extern int win_write_config;

int write_config (const char* filename, const struct GameDriver *gamedrv);
const char *get_devicedirectory(int dev);
void set_devicedirectory(int dev, const char *dir);

#endif /* CONFIGMS_H */

