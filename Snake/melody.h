#ifndef MELODY_H
#define MELODY_H

typedef struct {
    unsigned int freq;
    unsigned int duration;
} Tone;

const Tone melody[] = {
    { 3000, 550 },
    { 3500, 550 },
    { 4000, 550 },
    { 3200, 550 },
    { 3700, 550 },
    { 4200, 550 },
    { 3400, 550 },
    { 3900, 550 },
    { 4400, 550 },
    { 3600, 550 },
    { 4100, 550 },
    { 4600, 550 },
    { 3800, 550 },
    { 4300, 550 },
    { 4600, 550 },
    { 4000, 550 }
};

#endif // MELODY_H