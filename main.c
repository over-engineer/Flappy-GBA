/**********************************************\
*																					    *
*       					Flappy GBA								  *
*				          version 0.1		              *
*																					    *
*					Flappy Bird clone for the GBA				*
*																					    *
* *******************************************	*
*																					    *
*						   	Developed by dn0z							*
*          				August, 2014         				*
*																					    *
\**********************************************/

#include "gba.h"
#include "screenmode.h"
#include "keypad.h"
#include "gfx.h"
#include "sfx.h"

// graphics
#include "gfx\logo.c"
#include "gfx\palette.c"
#include "gfx\graphics.c"

// sounds
#include "snd\swooshing.c"

// configuration
#define LAND_STARTING_Y 									133
#define DISTANCE_BETWEEN_TOP_BOT_PIPES 		40
#define SPACE_BETWEEN_PIPE_SETS 					32

#define CEILING_POS(X) ((X-(int)(X)) > 0 ? (int)(X+1) : (int)(X))

u16* paletteMem = (u16*)0x5000000;
u16* FrontBuffer = (u16*)0x6000000;
u16* BackBuffer = (u16*)0x600A000;
u16* videoBuffer;

volatile u16* ScanlineCounter = (volatile u16*)0x4000006;

// Global sound variables
SAMPLE s_swooshing = { &swooshing, 10144 };
int SoundIsPlaying = 0;
u16 SoundPosition = 0;
u16 SoundVblanks = 0;

// Global variables
int pipesX[3] = { 120, 165, 210 };
int pipesH[3];
int gameIsOver = 0;

// Prototypes
void InitSfxSystem(void);
void PlaySfx(const SAMPLE *pSample);

void EraseScreen(void);
void DisplayIntro(void);

void DrawBackground(void);
void DrawLand(int frame);
void DrawTitle(void);
void DrawAllPipes(void);
void DrawPipe(int pipeX, int h, int pointing);								// if the pipe is pointing up, the pointing variable has a value of 0
void DrawBird(int birdX, int birdY, int frame);

void Collisions(int birdY);

void ChangePalette(int pal);
void PlotPixel(int x, int y, unsigned short int c);
void PlotPixelTransparent(int x, int y, unsigned short int c);

void WaitForVblank(void);															// waits for the drawer to get to the end before flipping buffers
void Flip(void);																			// flips between front/back buffer

int RandomPipeHeight(void);
int customRoundNum(float num);

/****************************************** MAIN ******************************************/

int main(void) {
	// bird coordinates
	int birdX = 54;
	int birdY = 52;
	
	// initialization
	SetMode(MODE_4 | BG2_ENABLE);
	
	EraseScreen();
	DisplayIntro();
	
	ChangePalette(1);
	
	InitSfxSystem();
	SeedRandom();
	
	int stillInMenu = 1;
	
	// animations
	int birdAnimationFrame = 0;
	int landAnimationFrame = 0;
	
	// physics
	int jumpSpeed = 35;										        // positive constant, resets whenever we press A
	int fallingConstant = 80;								      // positive constant, accelerates the bird on each update
	int framesPerSecond = 30;								      // frames per second to calculate delta time
	
	float deltaTime = 1.0f / framesPerSecond;		  // delta time for the physics, we use 1.0f to get a float from the division operator
	float vertSpeed = 0;										      // TODO: Tweak the above values to get a more natural bird movement
	float tmp;
	
	// init pipes height
	int i;
	for (i = 0; i < 3; i++) {
		pipesH[i] = RandomPipeHeight();
	}
	
	// main loop
	while (1) {
		if (!((*KEYS) & KEY_A) && !stillInMenu && !gameIsOver) {
			if (vertSpeed <= 0) {
				vertSpeed = jumpSpeed;
			}
			
			PlaySfx(&s_swooshing);
		}
		
		tmp = vertSpeed * deltaTime;
		birdY -= customRoundNum(tmp);
		
		// bird boundaries
		if (birdY <= 1) {
			birdY = 1;
		} else if (birdY >= 148) {
			birdY = 148;
		}
		
		vertSpeed -= fallingConstant * deltaTime;
		
		DrawBackground();
		DrawLand(landAnimationFrame);
		
		if (stillInMenu) {
			DrawTitle();
			
			if (birdY > 70) {
				vertSpeed = jumpSpeed;			// jump
			}
			
			if (!((*KEYS) & KEY_START)) {
				stillInMenu = 0;
			}
		} else {
			DrawAllPipes();
		}
		
		DrawBird(birdX, birdY, birdAnimationFrame);
		Collisions(birdY);
		
		WaitForVblank();
		Flip();
		
		if (!gameIsOver) {
			birdAnimationFrame = (birdAnimationFrame == 3 ? 0 : birdAnimationFrame + 1);
			landAnimationFrame = (landAnimationFrame == 2 ? 0 : landAnimationFrame + 1);
		}
	}
	
	return 0;
}

/****************************************** SOUND ******************************************/

void PlaySfx(const SAMPLE *pSample) {
	if (!SoundIsPlaying) {
		SoundVblanks = pSample->length / SAMPLES_PER_VBLANK;
		SoundVblanks *= 0.1;		// workaround to avoid interrupts (1/2)
		
		REG_TM0CNT = 0;
		REG_DMA1CNT = 0;
		
		REG_SOUNDCNT_H |= DSA_FIFO_RESET;
		
		REG_TM0D = TIMER_INTERVAL;
		REG_TM0CNT = TIMER_ENABLED;
		
		REG_DMA1SAD = (u32)(pSample->pData);
		REG_DMA1DAD = (u32)REG_FIFO_A;
		REG_DMA1CNT = ENABLE_DMA | START_ON_FIFO_EMPTY | WORD_DMA | DMA_REPEAT;
		
		SoundPosition = 0;
		SoundIsPlaying = 1;
	}
}

void InitSfxSystem(void) {
	REG_SOUNDCNT_X = SND_ENABLED;
	REG_SOUNDCNT_L = 0;
	
	REG_SOUNDCNT_H = SND_OUTPUT_RATIO_100 | DSA_OUTPUT_RATIO_100 | DSA_OUTPUT_TO_BOTH | DSA_TIMER0 | DSA_FIFO_RESET;
}

/****************************************** BASIC FUNCTIONS ******************************************/

void EraseScreen(void) {
	int x, y;
	
	for (y = 0; y < 160; y++) {
		for (x = 0; x < 120; x++) {
			PlotPixel(x, y, 0x0000);
		}
	}
	
	WaitForVblank();
	Flip();
}

void DisplayIntro(void) {
	int x, y;
	
	// put palette in memory
	ChangePalette(0);
	
	// draw image
	for (y = 0; y < 160; y++) {
		for (x = 0; x < 120; x++) {
			PlotPixel(x, y, logoData[x + (y * 120)]);
		}
	}
	
	WaitForVblank();
	Flip();
	Sleep(125);
}

/****************************************** DRAWING ******************************************/

void DrawBackground(void) {
	int x, y;
	
	const int skyHeight = 91;
	const int treesAndBuildingsHeight = 43;
	
	const u16 skyColor = 0x1111;
	
	for (y = 0; y < skyHeight; y++) {
		for (x = 0; x < 120; x++) {
			PlotPixel(x, y, skyColor);
		}
	}
	
	for (y = 0; y < treesAndBuildingsHeight; y++) {
		for (x = 0; x < 120; x++) {
			PlotPixel(x, y + skyHeight, backgroundData[x + (y * 120)]);
		}
	}
}

void DrawLand(int frame) {
	int x, y, h;
	
	h = frame;
	
	for (y = 0; y < 26; y++) {
		for (x = 0; x < 120; x++) {
			PlotPixel(x, y + (LAND_STARTING_Y + 1), landData[h + (y * 3)]);
			h = (h == 2 ? 0 : (h + 1));
		}
	}
}

void DrawTitle(void) {
	int x, y;
	int loopsX = title_WIDTH / 2;
	
	for (y = 0; y < title_HEIGHT; y++) {
		for (x = 0; x < loopsX; x++) {
			PlotPixelTransparent(x + 23, y + 40, titleData[x + (y * 74)]);
		}
	}
}

void DrawAllPipes() {
	int i, j;
	int h[2];
	
	int totalHeight = LAND_STARTING_Y - DISTANCE_BETWEEN_TOP_BOT_PIPES;
	
	for (i = 0; i < 3; i++) {
		// pipes height
		h[0] = pipesH[i];
		h[1] = totalHeight - h[0];
		
		// draw the pipe set
		for (j = 0; j < 2; j++) {
			DrawPipe(pipesX[i], h[j], j);
		}
		
		if (pipesX[i] == -(pipe_WIDTH / 2)) {
			pipesH[i] = RandomPipeHeight();
		}
		
		if (!gameIsOver) {
			pipesX[i] = (pipesX[i] == -(pipe_WIDTH / 2)) ? (pipesX[(i == 0) ? 2 : (i - 1)] + SPACE_BETWEEN_PIPE_SETS + (pipe_WIDTH / 2)) : (pipesX[i] - 1);
		}
	}
}

void DrawPipeSet(int pipeX, int h1, int h2) {
	int i;
	for (i = 0; i < 2; i++) {
		DrawPipe(pipeX, (i == 0) ? h1 : h2, i);
	}
}

void DrawPipe(int pipeX, int h, int pointing) {
	int x, y;
	int pipeY, loopsX, loopsY;
	
	const int halfPipeWidth = pipeUp_WIDTH / 2;
	
	int negativeXOffset = 0;
	if (pipeX < 0) {
		negativeXOffset = pipeX * -1;
	}
	
	h = (h < (pipeUp_HEIGHT + 1)) ? (pipeUp_HEIGHT + 1) : h;
	loopsX = ((((SCREEN_WIDTH / 2) - pipeX) < halfPipeWidth) ? ((SCREEN_WIDTH / 2) - pipeX) : halfPipeWidth) - negativeXOffset;
	loopsY = h - pipeUp_HEIGHT;
	
	if (pointing == 0) {
		// The pipe is pointing up
		pipeY = LAND_STARTING_Y - h;
		
		for (y = 0; y < pipeUp_HEIGHT; y++) {
			for (x = 0; x < loopsX; x++) {
				PlotPixelTransparent(x + pipeX + negativeXOffset, y + pipeY, pipeUpData[x + (y * halfPipeWidth) + negativeXOffset]);
			}
		}
		
		pipeY += pipeUp_HEIGHT;
		
		for (y = 0; y < loopsY; y++) {
			for (x = 0; x < loopsX; x++) {
				PlotPixelTransparent(x + pipeX + negativeXOffset, y + pipeY, pipeData[x + negativeXOffset]);
			}
		}
		
	} else {
		// The pipe is pointing down
		for (y = 0; y < loopsY; y++) {
			for (x = 0; x < loopsX; x++) {
				PlotPixelTransparent(x + pipeX + negativeXOffset, y, pipeData[x + negativeXOffset]);
			}
		}
		
		pipeY = h - pipeUp_HEIGHT;
		
		for (y = 0; y < pipeUp_HEIGHT; y++) {
			for (x = 0; x < loopsX; x++) {
				PlotPixelTransparent(x + pipeX + negativeXOffset, y + pipeY, pipeDownData[x + (y * halfPipeWidth) + negativeXOffset]);
			}
		}
		
	}
}

void DrawBird(int birdX, int birdY, int frame) {
	int x, y;
	
	int loopsX = bird_WIDTH / 2;
	int loopsY = bird_HEIGHT / 4;
	
	int animationOffset = frame * (bird_HEIGHT / 4);			// how many vertical pixels to skip for that frame of the animation
	
	for (y = 0; y < loopsY; y++) {
		for (x = 0; x < loopsX; x++) {
			PlotPixelTransparent(x + birdX, y + birdY, birdData[x + ((y + animationOffset) * loopsX)]);
		}
	}
}

/****************************************** COLLISIONS ******************************************/

void Collisions(int birdY) {
	// not yet implemented
	// TODO: Add collision detection and set the gameIsOver to 1 when the bird collides with a pipe
}

/****************************************** PALETTE AND VIDEOBUFFER ******************************************/

void ChangePalette(int pal) {
	int i;
	
	if (pal == 0) {
		// icy hippo logo palette
		for (i = 0; i < 256; i++)
			paletteMem[i] = logoPalette[i];
	} else if (pal == 1) {
		// graphics master palette
		for (i = 0; i < 256; i++)
			paletteMem[i] = masterPalette[i];
	}
}

void PlotPixel(int x, int y, unsigned short int c) {
	/* in mode 4, we multiply by 120 instead of 240
	because video memory is written in 16 bit chunks */
	videoBuffer[x + (y * 120)] = c;
}

void PlotPixelTransparent(int x, int y, unsigned short int c) {
	unsigned short int temp;
	
	if ((c & 0x00FF) == 0) {
		// bottom is transparent
		if ((c & 0xFF00) == 0) {
			// top is also transparent
			return;
		}
		// bottom is transparent, top is not
		temp = ((videoBuffer[(y) * 120 + (x)]) & 0x00FF);
		temp |= c;
		videoBuffer[(y) *120 + (x)] = (temp);
	} else {
		// bottom is not transparent
		if 	((c & 0xFF00) == 0) {
			// top is transparent, bottom is not
			temp = ((videoBuffer[(y) * 120 + (x)]) & 0xFF00);
			temp |= c;
			videoBuffer[(y) *120 + (x)] = (temp);
		} else {
			// top and bottom are not transparent
			videoBuffer[(y) *120 + (x)] = (c);
		}
	}
}

void WaitForVblank(void) {
	while (*ScanlineCounter < 160) {}
	// now we are in the vblank period
	
	/* workaround to avoid interrupts (2/2) */
	if (SoundIsPlaying) {
		while(*ScanlineCounter == 160) {}
		
		if (SoundPosition == SoundVblanks) {
			REG_DMA1CNT = 0;
			SoundIsPlaying = 0;
		} else {
			SoundPosition++;
		}
	}
}

void Flip(void) {
	if (REG_DISPCNT & BACKBUFFER) {
		// back buffer is the current buffer
		REG_DISPCNT &= ~BACKBUFFER;             // flips active buffer to front buffer by clearing back buffer bit
		videoBuffer = BackBuffer;               // points the drawing buffer to the back buffer
	} else {
		// front buffer is the current buffer
		REG_DISPCNT |= BACKBUFFER;			        // flips active buffer to back buffer by setting the back buffer bit
		videoBuffer = FrontBuffer;					    // points the drawing buffer to the front buffer
	}
}

/****************************************** MATH FUNCTIONS ******************************************/

int RandomPipeHeight() {
	int minHeight = pipeUp_HEIGHT + 1;
	int maxHeight = (LAND_STARTING_Y - DISTANCE_BETWEEN_TOP_BOT_PIPES) - minHeight;
	
	return RAND(maxHeight - minHeight) + minHeight;
}

int customRoundNum(float num) {
	return (num < 0 ? (CEILING_POS(num * -1) * -1) : CEILING_POS(num));
}
