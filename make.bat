:: Builds GBA rom

gcc -c -O3 -mthumb -mthumb-interwork gfx.c
gcc -c -O3 -mthumb -mthumb-interwork main.c
gcc -mthumb -mthumb-interwork -o flappy.elf main.o gfx.o

objcopy -O binary flappy.elf flappy.gba

pause
