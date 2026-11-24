// Mossy Hollow - Cozy pixel-art stealth game for Nintendo 3DS (libctru + citro2d)
// Controls: Circle Pad / D-Pad = move | L = sneak | A = give gift / advance dialogue | B = talk / close dialogue | Touch = menu | START = quit | SELECT = back to menu
#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SCR_W 400
#define SCR_H 240
#define BOT_W 320
#define BOT_H 240
#define TS 16
#define MAP_W 64
#define MAP_H 48
#define RGB(r, g, b) C2D_Color32((r), (g), (b), 255)
#define RGBA(r, g, b, a) C2D_Color32((r), (g), (b), (a))
#define NUM_GIFTS 8
#define PLAYER_SIZE 10

// ---- Palette
#define INK RGB(54, 38, 48)
#define INK_SOFT RGB(102, 76, 72)
#define PAPER RGB(254, 244, 224)
#define PAPER_DK RGB(226, 202, 168)
#define WOOD_HI RGB(232, 180, 118)
#define WOOD_MID RGB(190, 132, 84)
#define WOOD_DK RGB(128, 82, 54)
#define GRASS_A RGB(168, 206, 134)
#define GRASS_B RGB(156, 196, 124)
#define PATH_HI RGB(248, 226, 182)
#define PATH_MID RGB(232, 202, 154)
#define PATH_EDGE RGB(186, 154, 114)
#define WATER_HI RGB(178, 228, 238)
#define WATER_MID RGB(142, 204, 222)
#define WATER_DK RGB(108, 174,
