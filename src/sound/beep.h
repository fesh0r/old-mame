#ifndef BEEP_H
#define BEEP_H
int beep_sh_start(const struct MachineSound *msound);
void beep_sh_stop(void);
void beep_sh_update(void);
void beep_set_state(int,int);
void beep_set_frequency(int,int);
void beep_set_volume(int,int);

struct beep_interface
{
        int num;
};
#endif
