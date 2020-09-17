#include <stdlib.h>
#include <stdio.h>
#include <6502.h>
#include <conio.h>
#include <c64.h>
#include "types.h"
#include "memory.h"

#define WT_WATER 6
#define WT_FISH 5
#define WT_SHARK 2

#define WIDTH 80
#define HEIGHT 50
#define WSIZE WIDTH *HEIGHT
#define WSIZE2 WSIZE * 2

#define UPDATE_SCREEN lcopy((long)canvas, 0xff80000, WSIZE)

byte *vic3_ctl1 = (byte *)0xd016;
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
    {3, 0, 2, 1},

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

byte _fishTimeToReproduce = 10;
byte _sharkTimeToReproduce = 50;
byte _initialSharkEnergy = 40;
int _initialSharks = 100;
int _initialFish = 180;

byte fishTimeToReproduce;
byte sharkTimeToReproduce;
byte initialSharkEnergy;
int initialSharks;
int initialFish;

unsigned int idx;

void initDefaults()
{
    fishTimeToReproduce = _fishTimeToReproduce;
    sharkTimeToReproduce = _sharkTimeToReproduce;
    initialSharkEnergy = _initialSharkEnergy;
    initialSharks = _initialSharks;
    initialFish = _initialFish;
}

void init()
{
    cbm_k_bsout(0x8e); // select graphic charset
    cbm_k_bsout(0x08); // disable c= + shift

    mega65_io_enable();

    POKE(0xd030U, PEEK(0xd030U) | 4); // enable palette
    POKE(0xd100U + 6, 1);
    POKE(0xd200U + 6, 2); // nice sea blue
    POKE(0xd300U + 6, 6);
}

void dealloc()
{
    free(canvas);
    free(sharkEnergy);
    free(surviveTime);
}

char getkey(byte cursorFlag)
{
    char res;
    if (cursorFlag)
    {
        cursor(1);
    }

    while (kbhit())
    {
        cgetc();
    }

    res = cgetc();

    if (cursorFlag)
    {
        cursor(0);
    }
    return res;
}

void setTextScreen()
{
    *vic3_control &= (255 ^ 128); // disable 80chars
    *vic3_control &= (255 ^ 8);   // disable interlace
    *vic3_ctl1 &= (255 ^ 3);      // reset xscl
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
    *vic3_ctl1 |= 1;      // shift screen 1px to the right
    *vic3_bg = 0;
    *vic3_border = 0;

    *vic3_scnptr4 = 0x00;
    *vic3_scnptr3 = 0x00;
    *vic3_scnptr2 = 0x04;
    *vic3_scnptr1 = 0x00;

    lfill(0x40000, 81, WSIZE); // fill text screen with solid circles

    UPDATE_SCREEN;
}

void doFish(void)
{
    byte i;
    unsigned int newIdx;
    byte s;

    byte *dirPermutation = dirPermutations[rand() % 16];

    s = ++surviveTime[idx];

    for (i = 0; i < 4; ++i)
    {
        newIdx = idx + directions[dirPermutation[i]];

        if (newIdx > WSIZE2)
        {
            newIdx += WSIZE;
        }
        else if (newIdx > WSIZE)
        {
            newIdx -= WSIZE;
        }

        if (canvas[newIdx] == WT_WATER)
        {
            canvas[newIdx] = WT_FISH;
            surviveTime[newIdx] = s;
            canvas[idx] = WT_WATER;
            if (s > fishTimeToReproduce)
            {
                canvas[idx] = WT_FISH;
                surviveTime[idx] = 0;
                surviveTime[newIdx] = 0;
            }
            return;
        }
    }
}

void doShark(void)
{
    byte i;
    byte s;
    byte currentEnergy;
    unsigned int newIdx;
    byte ctest;

    unsigned int newSharkIndex = idx;
    byte didEat = false;
    byte *dirPermutation = dirPermutations[rand() % 16];

    s = ++surviveTime[newSharkIndex];

    for (i = 0; i < 4; ++i)
    {
        newIdx = idx + directions[dirPermutation[i]];

        if (newIdx > WSIZE2)
        {
            newIdx += WSIZE;
        }
        else if (newIdx > WSIZE)
        {
            newIdx -= WSIZE;
        }

        ctest = canvas[newIdx];

        if (ctest == WT_FISH)
        {
            didEat = true;
            newSharkIndex = newIdx;
            break;
        }

        if (ctest == WT_WATER)
        {
            newSharkIndex = newIdx;
        }
    }

    currentEnergy = --sharkEnergy[idx];

    if (!currentEnergy)
    {
        canvas[idx] = WT_WATER;
        return;
    }

    if (newSharkIndex != idx)
    {
        // move
        canvas[newSharkIndex] = WT_SHARK;
        surviveTime[newSharkIndex] = s;
        sharkEnergy[newSharkIndex] = currentEnergy;
        canvas[idx] = WT_WATER;
        if (s >= sharkTimeToReproduce)
        {
            canvas[idx] = WT_SHARK;
            surviveTime[idx] = 0;
            surviveTime[newSharkIndex] = 0;
            sharkEnergy[idx] = initialSharkEnergy;
        }
    }

    if (didEat)
    {
        // shark did eat -- increase energy
        if (currentEnergy < 255)
        {
            ++sharkEnergy[newSharkIndex];
        }
    }
}

long mainloop(void)
{
    long generations = 0;
    byte t;

    unsigned int numSharks;

    do
    {
        generations++;
        numSharks = 0;

        for (idx = 0; idx < WSIZE; ++idx)
        {
            t = canvas[idx];

            if (t == WT_FISH)
            {
                doFish();
            }
            else if (t == WT_SHARK)
            {
                ++numSharks;
                doShark();
            }
        }
        UPDATE_SCREEN;

    } while (numSharks && !kbhit());
    cgetc();

    return generations;
}

#define MAXTRIES 1000

byte initWorld(int numFish, int numSharks)
{
    int i;
    byte x, y;

    int tries = 0;

    srand(290672); // fix random seed for reproducible results

    canvas = (byte *)malloc(WSIZE);
    sharkEnergy = (byte *)malloc(WSIZE);
    surviveTime = (byte *)malloc(WSIZE);

    lfill((long)canvas, WT_WATER, WSIZE);
    lfill((long)sharkEnergy, initialSharkEnergy, WSIZE);
    lfill((long)surviveTime, 0, WSIZE);

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
        tries = 0;
        do
        {
            tries++;
            x = rand() % WIDTH;
            y = rand() % HEIGHT;
        } while (canvas[x + (WIDTH * y)] != WT_WATER && tries < MAXTRIES);
        if (tries == MAXTRIES)
        {
            return false;
        }
        canvas[x + (WIDTH * y)] = WT_SHARK;
    }
    return true;
}

int getValue(int min, int max, int stdVal)
{
    int retVal;
    char buf[10];

    textcolor(COLOR_LIGHTRED);
    cprintf("\r\n  new value (%d-%d): ", min, max);
    fgets(buf, 8, stdin);
    retVal = atoi(buf);
    if (!retVal || retVal < min || retVal > max)
    {
        retVal = stdVal;
    }
    return retVal;
}

void main()
{
    long gens;
    char cmd;
    byte quit = false;
    struct regs r;

    initDefaults();

    do
    {
        init();
        setTextScreen();

        textcolor(COLOR_GRAY3);
        bordercolor(COLOR_BLACK);
        bgcolor(COLOR_BLUE);
        clrscr();
        gotoxy(0, 1);

        chline(40);
        cputs("wa-tor for the mega65\r\n");
        cputs("s. kleinert, september 2020\r\n");
        chline(40);

        gotoxy(0, 8);

        textcolor(COLOR_WHITE);
        cprintf("  1) fish reproduction time  : %4d\r\n", fishTimeToReproduce);
        cprintf("  2) shark reproduction time : %4d\r\n", sharkTimeToReproduce);
        cprintf("  3) initial shark energy    : %4d\r\n", initialSharkEnergy);
        cprintf("  4) initial # of fish       : %4d\r\n", initialFish);
        cprintf("  5) initial # of sharks     : %4d\r\n", initialSharks);

        textcolor(COLOR_YELLOW);
        cputsxy(2, 15, "s) start simulation");
        cputsxy(2, 16, "q) quit program");

        textcolor(COLOR_GRAY3);
        cputsxy(2, 19, "your choice: ");

        cmd = getkey(1);
        cputc(cmd);

        switch (cmd)
        {

        case '1':
            fishTimeToReproduce = getValue(1, 255, fishTimeToReproduce);
            break;

        case '2':
            sharkTimeToReproduce = getValue(1, 255, sharkTimeToReproduce);
            break;

        case '3':
            initialSharkEnergy = getValue(1, 255, initialSharkEnergy);
            break;

        case '4':
            initialFish = getValue(1, 3000, initialFish);
            break;

        case '5':
            initialSharks = getValue(1, 3000, initialSharks);
            break;

        case 's':
        {
            clrscr();
            if (initialSharkEnergy > sharkTimeToReproduce)
            {
                cputs("error: initial shark energy is greater\r\n"
                      "than shark reproduction time!\r\n\r\n"
                      "--key--");
                getkey(0);
                break;
            }

            if (initWorld(initialFish, initialSharks))
            {
                setWatorScreen();
                gens = mainloop();
                dealloc();
                setTextScreen();
            }
            else
            {
                cputs("error: could not fit sharks.\r\n"
                      "try creating less sharks or less fish.\r\n\r\n"
                      "--key--");
                getkey(0);
                dealloc();
            }
        }
        break;

        case 'q':
            r.pc = 58552U;
            _sys(&r);
            exit(0);
            break;

        default:
            break;
        }

    } while (!quit);
    printf("\n\ntschuessn!");
}