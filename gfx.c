/****************************************
                gfx.c
	  gfx.h implementation
	 From the tank gba demo
 http://www.loirak.com/gameboy/tank.php
****************************************/

#include "screenmode.h"
#include "gfx.h"

void Sleep(int i) {
	int x, y;
	int c;
	for (y = 0; y < i; y++) {
		for (x = 0; x < 30000; x++) {
			c = c + 2;
		}
	}
}

void SeedRandom(void) {
   RAND_RandomData = REG_VCOUNT;
}

s32 RAND(s32 Value) {
   RAND_RandomData *= 14867;
   RAND_RandomData += 4567;

   return ((((RAND_RandomData >> 16) & RAND_MAX) * Value) >> 15);
}
