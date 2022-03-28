#ifndef PROGRAMS
#define PROGRAMS

#define CHAUFFAGE 0
#define EMPATAGE_PALIER1 1
#define EMPATAGE_PALIER2 2
#define EMPATAGE_PALIER3 3
#define EBULITION 4
#define REFOIDISSEMENT 5

#define AJOUTS_MAX 5  //Houblons ou Irish moss ...

struct program
{
    float temp_goals[6];

    unsigned char minutes_palier1;
    unsigned char minutes_palier2;
    unsigned char minutes_palier3;
    unsigned char minutes_ebulition;
    unsigned char minutes_ajouts[5];
};


#endif