/***************************************************************************

    infomess.h

    MESS-specific augmentation to info.c

***************************************************************************/

#ifndef INFOMESS_H
#define INFOMESS_H

/* code used by print_mame_xml() */
void print_mess_game_xml(FILE *out, const game_driver *game, const machine_config *config);

#endif /* INFOMESS_H */
