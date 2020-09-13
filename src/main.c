#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdio.h>
#include <conio.h>
#include <c64.h>

#include "types.h"
#include "memory.h"

#define WT_WATER 6
#define WT_FISH 5
#define WT_SHARK 2

#define WIDTH 80
#define HEIGHT 50
#define WSIZE 4000

#define UPDATE_SCREEN lcopy((long)canvas, 0xff80000, WSIZE)

byte *vic3_control = (byte *)0xd031;
byte *vic3_border = (byte *)0xd020;
byte *vic3_bg = (byte *)0xd021;

byte *vic3_scnptr4 = (byte *)0xd060;
byte *vic3_scnptr3 = (byte *)0xd061;
byte *vic3_scnptr2 = (byte *)0xd062;
byte *vic3_scnptr1 = (byte *)0xd063;

byte *canvas;
byte *sharkEnergy;

byte *surviveTime;

byte dirPermutations[16][4] = {

    {0, 2, 1, 3},
    {2, 1, 3, 0},
    {1, 3, 0, 2},
    {3, 0, 1, 2},

    {3, 2, 1, 0},
    {2, 1, 0, 3},
    {1, 0, 3, 2},
    {0, 3, 2, 1},

    {0, 1, 2, 3},
    {1, 2, 3, 0},
    {2, 3, 0, 1},
    {3, 0, 1, 2},

    {0, 3, 1, 2},
    {3, 1, 2, 0},
    {1, 2, 0, 3},
    {2, 0, 3, 1}

};

signed char directions[4] = {-1, 1, -WIDTH, WIDTH};

byte fishTimeToReproduce = 30;
byte sharkTimeToReproduce = 60;
byte initialSharkEnergy = 40;
int initialSharks = 20;
int initialFish = 80;

void init()
{
    cbm_k_bsout(0x8e); // select graphic charset
    cbm_k_bsout(0x08); // disable c= + shift

    mega65_io_enable();

    POKE(0xd030U, PEEK(0xd030U) | 4); // enable palette
    POKE(0xd100U + 6, 0);
    POKE(0xd200U + 6, 2); // nice sea blue
    POKE(0xd300U + 6, 4);

    canvas = (byte *)malloc(WSIZE);
    sharkEnergy = (byte *)malloc(WSIZE);
    surviveTime = (byte *)malloc(WSIZE);
}

void dealloc()
{
    free(canvas);
    free(sharkEnergy);
    free(surviveTime);
}

void setTextScreen()
{
    *vic3_control &= (255 ^ 128); // disable 80chars
    *vic3_control &= (255 ^ 8);   // disable interlace
    *vic3_scnptr4 = 0x00;
    *vic3_scnptr3 = 0x04;
    *vic3_scnptr2 = 0x00;
    *vic3_scnptr1 = 0x00;
}

void setWatorScreen()
{

    long textScreen = 0x40000;

    mega65_io_enable();

    *vic3_control |= 128; // enable 80chars
    *vic3_control |= 8;   // enable interlace
    *vic3_bg = 0;
    *vic3_border = 0;

    *vic3_scnptr4 = 0x00;
    *vic3_scnptr3 = 0x00;
    *vic3_scnptr2 = 0x04;
    *vic3_scnptr1 = 0x00;

    lfill(0x40000, 81, WSIZE); // fill text screen with solid circles
    lfill((long)canvas, WT_WATER, WSIZE);

    lfill((long)sharkEnergy, initialSharkEnergy, WSIZE);
    lfill((long)surviveTime, 0, WSIZE);

    UPDATE_SCREEN;
}

void doFish(int idx)
{
    register byte i;
    byte dir;
    register byte *dirPermutation;
    int newIdx;
    int indexToGo;
    byte didMove;

    dirPermutation = dirPermutations[rand() % 16];
    indexToGo = 0xffff;

    didMove = false;

    surviveTime[idx] += 1;

    for (i = 0; i < 4; ++i)
    {
        dir = dirPermutation[i];
        newIdx = idx + directions[dir];

        if (newIdx < 0)
        {
            newIdx += WSIZE;
        }
        else if (newIdx > WSIZE)
        {
            newIdx -= WSIZE;
        }

        if (canvas[newIdx] == WT_WATER)
        {
            didMove = true;
            indexToGo = newIdx;
            break;
        }
    }

    if (didMove)
    {
        canvas[indexToGo] = WT_FISH;
        surviveTime[indexToGo] = surviveTime[idx];
        canvas[idx] = WT_WATER;
    }
    else
    {
        return;
    }

    if (surviveTime[indexToGo] > fishTimeToReproduce)
    {
        {
            canvas[idx] = WT_FISH;
            surviveTime[idx] = 0;
        }
    }
}

void doShark(int idx)
{
    register byte i;
    register byte dir;
    byte *dirPermutation;
    int newIdx;
    int newSharkIndex;
    byte didEat;

    dirPermutation = dirPermutations[rand() % 16];
    newSharkIndex = idx;
    didEat = false;

    for (i = 0; i < 4; ++i)
    {
        dir = dirPermutation[i];
        newIdx = idx + directions[dir];

        if (newIdx < 0)
        {
            newIdx += WSIZE;
        }
        else if (newIdx > WSIZE)
        {
            newIdx -= WSIZE;
        }

        if (canvas[newIdx] == WT_FISH)
        {
            didEat = true;
            newSharkIndex = newIdx;
            break;
        }

        if (canvas[newIdx] == WT_WATER)
        {
            newSharkIndex = newIdx;
        }
    }

    sharkEnergy[idx] -= 1;

    if (newSharkIndex != idx)
    {
        // move
        canvas[newSharkIndex] = WT_SHARK;
        surviveTime[newSharkIndex] = surviveTime[idx];
        sharkEnergy[newSharkIndex] = sharkEnergy[idx];
        canvas[idx] = WT_WATER;
    }

    if (didEat)
    {
        // shark did eat -- increase energy
        if (sharkEnergy[newSharkIndex] < 255)
        {
            sharkEnergy[newSharkIndex] = sharkEnergy[newSharkIndex] + 1;
        }
    }
    else
    {
        // check shark energy
        if (sharkEnergy[newSharkIndex] == 0)
        {
            canvas[newSharkIndex] = WT_WATER;
            return;
        }
    }

    if (surviveTime[newSharkIndex] >= sharkTimeToReproduce)
    {
        if (idx != newSharkIndex)
        {
            canvas[idx] = WT_SHARK;
            surviveTime[idx] = 0;
            surviveTime[newSharkIndex] = 0;
            sharkEnergy[idx] = initialSharkEnergy;
        }
    }

    surviveTime[newSharkIndex] += 1;
}

long mainloop(void)
{
    int i;
    long generations = 0;

    int numSharks;

    do
    {
        generations++;
        numSharks = 0;

        for (i = 0; i < WSIZE; ++i)
        {
            if (canvas[i] == WT_FISH)
            {
                doFish(i);
            }
            else if (canvas[i] == WT_SHARK)
            {
                numSharks++;
                doShark(i);
            }
        }
        UPDATE_SCREEN;
    } while (numSharks && !kbhit());

    return generations;
}

void initWorld(int numFish, int numSharks)
{
    int i;
    byte x, y;
    for (i = 0; i < numFish; ++i)
    {
        do
        {
            x = rand() % WIDTH;
            y = rand() % HEIGHT;
        } while (canvas[x + (WIDTH * y)] != WT_WATER);
        canvas[x + (WIDTH * y)] = WT_FISH;
    }
    for (i = 0; i < numSharks; ++i)
    {
        do
        {
            x = rand() % WIDTH;
            y = rand() % HEIGHT;
        } while (canvas[x + (WIDTH * y)] != WT_WATER);
        canvas[x + (WIDTH * y)] = WT_SHARK;
    }
    UPDATE_SCREEN;
}

void main()
{
    long gens;
    char yn;
    char buf[10];

    do
    {
        init();

        textcolor(COLOR_GREEN);
        bordercolor(COLOR_BLUE);
        bgcolor(COLOR_BLACK);
        clrscr();
        cputs("================================\r\n");
        cputs("== wa-tor for the mega65      ==\r\n");
        cputs("== s.kleinert, september 2020 ==\r\n");
        cputs("================================\r\n\r\n");

        printf("\nfish time to reproduce (1-255) [30]:\n");
        fgets(buf, 8, stdin);
        fishTimeToReproduce = atoi(buf);
        if (!fishTimeToReproduce)
        {
            fishTimeToReproduce = 30;
        }

        printf("\nsharks time to reproduce (1-255) [60]:\n");
        fgets(buf, 8, stdin);
        sharkTimeToReproduce = atoi(buf);
        if (!sharkTimeToReproduce)
        {
            sharkTimeToReproduce = 60;
        }

        printf("\ninitial shark energy (1-255) [40]:\n");
        fgets(buf, 8, stdin);
        initialSharkEnergy = atoi(buf);
        if (!initialSharkEnergy)
        {
            initialSharkEnergy = 40;
        }

        printf("\n# of initial sharks (1-4000) [20]:\n");
        fgets(buf, 8, stdin);
        initialSharks = atoi(buf);
        if (!initialSharks)
        {
            initialSharks = 20;
        }

        printf("\n# of initial fish (1-4000) [80]:\n");
        fgets(buf, 8, stdin);
        initialFish = atoi(buf);
        if (!initialFish)
        {
            initialFish = 80;
        }

        setWatorScreen();
        initWorld(initialFish, initialSharks);
        gens = mainloop();
        dealloc();

        setTextScreen();
        cbm_k_bsout(147);
        printf("simulation ended after %ld generations\nagain?(y/n)", gens);
        // empty buffer
        while (kbhit())
        {
            cgetc();
        }
        cursor(1);
        yn = cgetc();
        cursor(0);
    } while (yn != 'n');
    clrscr();
    printf("\n\ntschuessn!");
}