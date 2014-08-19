/****************************************
                gfx.h
	 From the tank gba demo
 http://www.loirak.com/gameboy/tank.php
****************************************/

#ifndef GFX_H
#define GFX_H

#include "gba.h"

#define RGB(r,g,b) ((r)+((g)<<5)+((b)<<10))

void Sleep(int i);
// 125 is a good delay for displaying something
// 1 is a good delay for a game (like 30/60 fps) or something like that

#define RAND_MAX 32767
volatile s32 RAND_RandomData;
void SeedRandom(void);
s32 RAND(s32 Value);
// usage: SeedRandom();
// then: xrand = RAND(MAX_X); where MAX_X is the upper bound for the value you want to return

#endif
