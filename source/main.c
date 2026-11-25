// Mahal Kita - Cozy pixel-art village game for Nintendo 3DS (libctru + citro2d)
// Controls: Circle Pad / D-Pad = move | A = enter / leave the cottage
//           A near Jay, Aling Rosa or Ate Mila (bakery) = talk | A = next line | B = close dialogue
//           Bakery menu: D-Pad Up/Down = pick | A = buy | B = leave
//           B = open / close My Journal (character customization)
//           In the journal: L/R = page | D-Pad = pick and change | A = change / type
//           L/R = pick a hotbar slot (bread you buy goes into the hotbar)
//           Y = eat or drink the food you are holding (fills your BUSOG bar, top-left)
//           A near a dog or cat (askal / puspin) = pet it
//           Kiko the baby kalabaw lives by the bahay kubo: press A next to him to pet him
//           A near Mang Taho (he walks around calling "Taho!") = talk, then buy a cup of taho
//           Beach: cross the kalsada to the waiting shed, then walk the boardwalk south
//           Village: plaza fountain, market street, sari-sari store (snacks + drinks),
//           veggie garden, lily pond, a bahay kubo, a neighbour's house and street lamps
//           Across the bridge: a kalsada with jeepneys and tricycles, an ukay-ukay
//           (clothes), a pet shop and a karenderya
//           Villagers walk around the village and the street: press A next to one to chat
//           Kids play tumbang preso, piko, Chinese garter and patintero on the new street
//           (press A next to one to ask about the game). Trash bins line the streets.
//           Pet food: hold it in the hotbar, then press A next to a dog or cat
//           School (east side of the village): A at the door = go in. Inside, press A next to
//           the 2 teachers (Miss Luz, Sir Dante) or the 3 students (Angel, Miko, Renz) to chat
//           Weather is sunny or rainy: it changes by itself, or tap the weather button
//           Day and night: the sun rises, the sky turns orange at sunset, and the village
//           lights up at night (lamps glow, fireflies come out). One day lasts 7 minutes.
//           Tap the clock on the bottom screen to skip to the next part of the day
//           At sunset the kids walk home along the sidewalk, and the shop windows glow after dark;
//           in the morning the kids come back out to play
//           When it rains, Jay and the villagers walking outside open umbrellas
//           Touch = menu | SELECT = pause menu | START = quit
//           Seasonal events (by the 3DS date): HALLOWEEN = Oct 1 - Nov 2, PASKO = Nov 3 - Jan 6.
//           Tap the EVENT button on the bottom screen: AUTO / HALLOWEEN / PASKO / OFF.
//           Chat with grown-ups to trick or treat (candy + coins) or get your aguinaldo.
//           During Pasko, children go namamasko outside the houses and shops (press A to chat).
//           Pause menu > SAVE GAME writes sdmc:/3ds/mahal_kita.sav; it loads by itself next launch
// Music: put audio.wav in romfs/ (or on the SD card at /3ds/audio.wav).
//        Sound also needs the DSP firmware file at sdmc:/3ds/dspfirm.cdc

#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <stddef.h>
#include <time.h>

#define SCR_W 400
#define SCR_H 240
#define BOT_W 320
#define BOT_H 240
#define TS 16
#define MAP_W 64
#define MAP_H 76        // the beach and the sea are the bottom rows (see stampBeach)
#define VILLAGE_H 48      // height of the original village (the street is below it)
// beach, south of the waiting shed
#define BEACH_Y0   62     // first row of dry sand
#define SHALLOW_Y0 69     // shallow water you can wade in
#define SEA_Y0     71     // deep sea (solid)
#define PIER_TX    33     // wooden boardwalk + pier, 2 tiles wide

#define RGB(r, g, b)    C2D_Color32((r), (g), (b), 255)
#define RGBA(r, g, b, a) C2D_Color32((r), (g), (b), (a))

#define PLAYER_SIZE 10
// Buildings are drawn BLD_NUM / BLD_DEN times bigger than the art in their draw functions, so the
// cottage, shops, school and houses tower over the villagers. Each building is scaled around the
// bottom of its door tile, so the door tile (and the path to it) never moves. The solid footprint
// of every building in genMap() is sized for 3 / 2 - change them together.
#define BLD_NUM 3
#define BLD_DEN 2
// most shapes citro2d can draw per frame (both screens together)
#define MAX_SHAPES 12288

// cottage position and door (in tiles)
#define COTTAGE_TX 28
#define COTTAGE_TY 13
#define DOOR_TX (COTTAGE_TX + 3)
#define DOOR_TY (COTTAGE_TY + 3)

// bakery position and door (in tiles)
#define BAK_TX 10
#define BAK_TY 30
#define BDOOR_TX (BAK_TX + 3)
#define BDOOR_TY (BAK_TY + 3)
#define BAKER_X 132       // Ate Mila's screen position inside the bakery
#define BAKE_COUNT 4      // items sold in each shop
#define ITEM_COUNT 21     // 0-3 bakery, 4-7 sari-sari, 8-11 clothes, 12-15 pets, 16-19 karenderya, 20 taho
#define SHOP_COUNT 7      // 0 flower, 1 bakery, 2 sari-sari, 3 ukay-ukay, 4 pet shop, 5 karenderya, 6 school
#define SCHOOL_ID 6       // the school is "shop" 6: no counter, but teachers and students to chat with

// school on the east side of the village (top-left corner of its original 7 x 4 tile art;
// it is drawn 1.5x bigger, so it covers 11 x 6 tiles around it)
#define SCHOOL_TX 54
#define SCHOOL_TY 21
#define SCH_DOOR_TX (SCHOOL_TX + 3)
#define SCH_DOOR_TY (SCHOOL_TY + 3)
#define SCHOOL_NPC_COUNT 5   // 2 teachers and 3 students

// sari-sari store position and door (in tiles)
#define SARI_TX 46
#define SARI_TY 30
#define SDOOR_TX (SARI_TX + 3)
#define SDOOR_TY (SARI_TY + 3)
#define SARI_X 136        // Kuya Ben's screen position inside the store

// shops along the new street (top-left corner of the original 6 x 4 art, in tiles)
#define UKAY_TX 18
#define PETS_TX 40
#define KAR_TX  50
#define STREET_TY 47
#define STREET_DOOR_TY (STREET_TY + 3)
static const int STREET_SHOP_TX[3] = { UKAY_TX, PETS_TX, KAR_TX }; // shop ids 3, 4, 5

// the kalsada (in tiles)
#define ROAD_Y0 53        // first road row
#define ROAD_Y1 57        // last road row
#define CROSS_X0 31       // pedestrian lane, in line with the bridge
#define CROSS_X1 32
#define SHED_TX 24        // waiting shed, across the road
#define SHED_TY 59
#define VEH_COUNT 4
#define WALKER_COUNT 7
#define TAHO_WALKER 6     // Mang Taho, the taho vendor (the last villager in WALKER_INFO)
#define KID_COUNT 12
#define BIN_COUNT 17

// village landmarks (top-left corner, in tiles)
#define FOUNTAIN_TX 30    // in the middle of the plaza; drawn 1.5x, so it covers 6 x 3 tiles
#define FOUNTAIN_TY 31
#define HUT_TX 5          // bahay kubo, 4 x 3 tiles
#define HUT_TY 13
#define HOUSE_TX 50       // neighbour's house, 5 x 3 tiles
#define HOUSE_TY 13
#define STALL_COUNT 3     // market stalls, 2 x 2 tiles each

// ---- Cozy pastel garden & stream palette
#define INK        RGB(54, 38, 48)
#define INK_SOFT   RGB(102, 76, 72)
#define PAPER      RGB(254, 244, 224)
#define PAPER_DK   RGB(226, 202, 168)
#define WOOD_HI    RGB(232, 180, 118)
#define WOOD_MID   RGB(190, 132, 84)
#define WOOD_DK    RGB(128, 82, 54)
#define GRASS_A    RGB(156, 174, 116)
#define GRASS_B    RGB(144, 164, 106)
#define PATH_HI    RGB(198, 198, 186)
#define PATH_MID   RGB(172, 174, 162)
#define PATH_EDGE  RGB(128, 130, 122)
#define WATER_HI   RGB(184, 220, 222)
#define WATER_MID  RGB(146, 194, 200)
#define WATER_DK   RGB(112, 160, 170)
#define SHADOW     RGBA(44, 52, 54, 64)
#define SKIN       RGB(255, 226, 198)
#define SKIN_SHD   RGB(228, 186, 158)
#define ROSE       RGB(240, 112, 140)
#define SKY_UI     RGB(118, 168, 214)
#define MINT_UI    RGB(122, 196, 148)
#define GOLD_UI    RGB(248, 202, 88)

// ---- Tea-shop street palette (muted, dusty pastels)
#define GRASS_TUFT RGB(118, 142, 88)
#define PATH_SEAM  RGB(148, 150, 140)
#define ROOF_HI    RGB(128, 148, 152)
#define ROOF_MID   RGB(92, 112, 120)
#define ROOF_DK    RGB(60, 76, 88)
#define SHOP_WOOD    RGB(160, 110, 78)
#define SHOP_WOOD_DK RGB(104, 70, 58)
#define SHOP_WOOD_HI RGB(196, 146, 100)
#define SHOP_DARK  RGB(58, 48, 52)
#define SHOP_GLOW  RGB(248, 214, 148)
#define NOREN      RGB(228, 140, 152)
#define NOREN_DK   RGB(188, 100, 120)
#define BENCH_HI   RGB(170, 124, 100)
#define BENCH_MID  RGB(142, 100, 84)
#define BENCH_DK   RGB(100, 68, 60)
#define DECK_HI    RGB(190, 146, 114)
#define DECK_MID   RGB(168, 124, 98)
#define DECK_DK    RGB(128, 92, 76)
#define LEAF_HI    RGB(138, 176, 102)
#define LEAF_MID   RGB(100, 142, 84)
#define LEAF_DK    RGB(72, 108, 70)
#define BLOSSOM_HI  RGB(246, 182, 204)
#define BLOSSOM_MID RGB(236, 130, 172)
#define BLOSSOM_DK  RGB(200, 96, 148)
#define BARK       RGB(112, 74, 58)
#define FENCE_HI   RGB(122, 82, 64)
#define FENCE_MID  RGB(88, 58, 48)
#define FENCE_DK   RGB(56, 36, 32)
#define ROSE_DK    RGB(184, 56, 96)
#define VINE       RGB(88, 142, 80)
#define VINE_DK    RGB(60, 104, 62)
#define BARK_DK    RGB(78, 50, 42)
#define BLOSSOM_PALE RGB(255, 226, 236)
#define BLOSSOM_GOLD RGB(250, 190, 70)
#define STONE_HI   RGB(196, 198, 190)
#define STONE_MID  RGB(152, 156, 154)
#define STONE_DK   RGB(106, 110, 114)

// ---- Tree: bright pixel-art leaves with dark outlines, warm brown trunk
#define TREE_OUT    RGB(22, 92, 34)
#define TREE_DK     RGB(38, 142, 46)
#define TREE_MID    RGB(66, 188, 54)
#define TREE_HI     RGB(116, 226, 80)
#define TREE_GLINT  RGB(176, 246, 114)
#define TRUNK_OUT   RGB(84, 44, 26)
#define TRUNK_DK    RGB(122, 68, 34)
#define TRUNK_MID   RGB(164, 92, 42)
#define TRUNK_HI    RGB(214, 138, 62)

// ---- Village extras: thatch, bamboo, soil and corrugated tin
#define THATCH     RGB(218, 188, 104)
#define THATCH_DK  RGB(172, 140, 76)
#define BAMBOO     RGB(222, 192, 128)
#define BAMBOO_DK  RGB(178, 146, 94)
#define SOIL       RGB(148, 106, 78)
#define SOIL_DK    RGB(112, 78, 60)
#define TIN_HI     RGB(150, 190, 210)
#define TIN_MID    RGB(112, 156, 184)
#define TIN_DK     RGB(76, 112, 144)

// ---- Beach colors
#define SAND_HI    RGB(250, 234, 190)
#define SAND_MID   RGB(240, 216, 166)
#define SAND_DK    RGB(214, 184, 132)
#define SAND_WET   RGB(202, 172, 128)
#define SHALLOW_C  RGB(146, 216, 220)
#define SHALLOW_HI RGB(192, 240, 240)
#define SEA_LT     RGB(150, 210, 228)
#define SEA_MID    RGB(92, 168, 206)
#define SEA_DK     RGB(66, 134, 182)
#define FOAM       RGB(252, 254, 255)

// ---- Bakery bread colors
#define BREAD_HI   RGB(246, 208, 142)
#define BREAD_MID  RGB(224, 164, 94)
#define BREAD_DK   RGB(160, 100, 62)

// ---- Pink title-screen palette
#define PINK_DK    RGB(200, 104, 144)
#define PINK_MID   RGB(230, 134, 170)
#define PINK_LT    RGB(246, 168, 196)
#define PINK_PALE  RGB(252, 214, 230)
#define MAUVE      RGB(176, 118, 156)
#define PLUM       RGB(120, 66, 104)
#define WHITE      RGB(255, 252, 253)

#define UNUSED_FN __attribute__((unused))

enum {
	T_GRASS, T_PATH, T_TREE, T_APPLE, T_ROCK, T_ROOF,
	T_WALL, T_DOOR, T_FENCE, T_MUSH, T_FLOWER,
	T_WATER, T_BRIDGE, T_TABLE, T_BLANKET, T_SUNFLOWER,
	T_CROP, T_SCARE, T_LAMP, T_ROAD, T_POLE, T_TRASH,
	T_SAND, T_SHALLOW, T_SEA, T_BOARD, T_PALM, T_PARASOL, T_TOWEL, T_SIGN
};

enum {
	STATE_MENU,
	STATE_PLAYING
};

enum {
	BTN_PLAY,
	BTN_CUSTOMIZE,
	BTN_SAVE,
	BTN_HOWTO,
	BTN_RESTART,
	BTN_COUNT
};

typedef struct {
	int x, y, w, h;
	const char *label;
} MenuBtn;

static const MenuBtn MENU_BTNS[BTN_COUNT] = {
	{ 36,  40, 248, 30, "START" },
	{ 36,  74, 248, 30, "CUSTOMIZE" },
	{ 36, 108, 248, 30, "SAVE GAME" },
	{ 36, 142, 248, 30, "HOW TO PLAY" },
	{ 36, 176, 248, 30, "NEW GAME" }
};

// "MENU" button shown on the bottom screen while playing
#define PLAY_MENU_X 10
#define PLAY_MENU_Y 200
#define PLAY_MENU_W 100
#define PLAY_MENU_H 30
// weather button on the bottom screen while playing (tap to switch sunny / rainy)
#define WX_X 116
#define WX_Y 200
#define WX_W 100
#define WX_H 30
// clock on the bottom screen (tap to skip to the next part of the day)
#define CLK_X 16
#define CLK_Y 42
#define CLK_W 112
#define CLK_H 40
// seasonal event button on the bottom screen (tap to switch AUTO / HALLOWEEN / PASKO / OFF)
#define EVB_X 192
#define EVB_Y 42
#define EVB_W 112
#define EVB_H 40

static u8 map[MAP_H][MAP_W];
static const int STALLS[STALL_COUNT][2] = { { 19, 33 }, { 22, 33 }, { 25, 33 } }; // market stall tiles
static u32 flowerCols[5];
static int frameCount = 0;
static int gameState = STATE_MENU;
static bool hasStartedOnce = false;
static bool showHowTo = false;
static bool showCustomize = false;
static int journalTab = 0;
static int journalRow = 0;
// appearance
static int skinIndex = 2;
static int hairIndex = 0;
static int styleIndex = 0;
static int accIndex = 0;
static int outfitIndex = 0;
static int skirtIndex = 0;
static int shoeIndex = 0;
// profile (names allow 12 characters; extra room for multi-byte letters)
#define NAME_CHARS 12
#define NAME_BYTES (NAME_CHARS * 3 + 1)
static char playerName[NAME_BYTES] = "em";
static char homeName[NAME_BYTES] = "Bahay Kubo";
static int pronounIndex = 0;
#define FIRST_YEAR 1950
#define LAST_YEAR 2026
static int monthIndex = 10;                   // 0 = January, so 10 = November
static int dayIndex = 4;                      // 0 = the 1st, so 4 = the 5th
static int yearIndex = 2007 - FIRST_YEAR;
static int statusIndex = 0;
static int spriteRows = 40; // lowered to draw head-and-shoulders portraits
// C2D_Color32 is a function-like color constructor, so initialize these at runtime.
static u32 skinTones[5];
static u32 hairColors[6];
static u32 outfitColors[6];
static u32 skirtColors[5];
static u32 shoeColors[5];

static int selectedBtn = BTN_PLAY;

static bool solid(u8 t) {
	return t == T_TREE || t == T_APPLE || t == T_ROCK || t == T_ROOF ||
	       t == T_WALL || t == T_DOOR || t == T_FENCE || t == T_WATER ||
	       t == T_TABLE || t == T_SCARE || t == T_LAMP || t == T_POLE || t == T_TRASH ||
	       t == T_SEA || t == T_PALM || t == T_PARASOL || t == T_SIGN;
}

static u32 hash2(int x, int y) {
	u32 h = (u32)x * 374761393u + (u32)y * 668265263u;
	h = (h ^ (h >> 13)) * 1274126177u;
	return h ^ (h >> 16);
}

static u8 tileAt(float px, float py) {
	int tx = (int)floorf(px / TS);
	int ty = (int)floorf(py / TS);
	if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) return T_TREE;
	return map[ty][tx];
}

static bool inYard(int x, int y) {
	return (x >= 22 && x <= 38 && y >= 10 && y <= 24) ||
	       (y >= 40 && y <= 43 && x >= 24 && x <= 37);
}

// Keep trees sparse and out of the way of paths, water and each other.
static bool treeSpotOk(int x, int y) {
	for (int d = 1; d <= 2; d++) {
		if (y - d < 0) continue;
		u8 t = map[y - d][x];
		if (t == T_PATH || t == T_BRIDGE || t == T_WATER) return false;
	}
	for (int dy = -1; dy <= 1; dy++)
		for (int dx = -1; dx <= 1; dx++)
			if (map[y + dy][x + dx] == T_TREE) return false;
	return true;
}

// small helpers for stamping the village landmarks
static void clearArea(int x0, int y0, int x1, int y1) {
	for (int y = y0; y <= y1; y++)
		for (int x = x0; x <= x1; x++)
			if (x > 0 && y > 0 && x < MAP_W - 1 && y < MAP_H - 1) map[y][x] = T_GRASS;
}

static void fillPath(int x0, int y0, int x1, int y1) {
	for (int y = y0; y <= y1; y++)
		for (int x = x0; x <= x1; x++)
			if (x > 0 && y > 0 && x < MAP_W - 1 && y < MAP_H - 1) map[y][x] = T_PATH;
}

// Stamps a solid building footprint: the first `roofRows` rows are roof, the rest are wall.
static void stampBuilding(int x0, int y0, int x1, int y1, int roofRows) {
	for (int y = y0; y <= y1; y++)
		for (int x = x0; x <= x1; x++)
			if (x > 0 && y > 0 && x < MAP_W - 1 && y < MAP_H - 1)
				map[y][x] = (y - y0 < roofRows) ? T_ROOF : T_WALL;
}

// The beach: sand south of the waiting shed, wadeable shallows, then deep sea. A wooden
// boardwalk leads down from the street and turns into a pier. (Called at the end of genMap.)
static void stampBeach(void) {
	for (int y = BEACH_Y0; y < MAP_H; y++)
		for (int x = 0; x < MAP_W; x++)
			map[y][x] = y < SHALLOW_Y0 ? T_SAND : y < SEA_Y0 ? T_SHALLOW : T_SEA;

	// boardwalk from the sidewalk, over the sand and out to sea
	for (int y = ROAD_Y1 + 2; y < MAP_H - 2; y++) {
		map[y][PIER_TX] = T_BOARD;
		map[y][PIER_TX + 1] = T_BOARD;
	}

	// palm trees
	static const int PALMS[][2] = {
		{ 5, 62 }, { 12, 63 }, { 19, 62 }, { 39, 62 }, { 46, 63 }, { 55, 62 }, { 60, 63 }
	};
	for (int i = 0; i < 7; i++) map[PALMS[i][1]][PALMS[i][0]] = T_PALM;

	// beach parasols, each with a towel beside it
	static const int PARASOLS[][2] = { { 14, 65 }, { 26, 66 }, { 42, 65 }, { 50, 66 } };
	for (int i = 0; i < 4; i++) {
		int x = PARASOLS[i][0], y = PARASOLS[i][1];
		map[y][x] = T_PARASOL;
		map[y][x + 1] = T_TOWEL;
		map[y][x + 2] = T_TOWEL;
	}

	map[62][36] = T_SIGN; // "DAGAT / BEACH" next to the boardwalk
}

static void genMap(void) {
	srand(1234);
	memset(map, T_GRASS, sizeof(map));

	// Main horizontal garden path
	for (int x = 0; x < MAP_W; x++)
		for (int y = 37; y <= 38; y++) map[y][x] = T_PATH;

	// Cozy cottage / garden pavilion in clearing
	const int hx = COTTAGE_TX, hy = COTTAGE_TY;
	for (int x = hx; x < hx + 6; x++) {
		map[hy][x] = T_ROOF;
		map[hy + 1][x] = T_ROOF;
		map[hy + 2][x] = T_WALL;
		map[hy + 3][x] = T_WALL;
	}
	map[DOOR_TY][DOOR_TX] = T_DOOR;

	// Path connecting clearing down to the stream bridge
	for (int y = hy + 4; y < ROAD_Y0; y++) {
		map[y][hx + 3] = T_PATH;
		map[y][hx + 4] = T_PATH;
	}

	// Paved sidewalk strip along the shop front (Jay strolls along it)
	for (int x = 25; x <= 34; x++) map[hy + 4][x] = T_PATH;

	// White picket fence around picnic garden
	for (int x = 24; x <= 35; x++) {
		map[11][x] = T_FENCE;
		if (x != hx + 3 && x != hx + 4) map[22][x] = T_FENCE;
	}
	for (int y = 11; y <= 22; y++) {
		map[y][24] = T_FENCE;
		map[y][35] = T_FENCE;
	}

	// Wooden benches & a tatami-style deck with cushions in front of the shop
	map[18][26] = T_BLANKET;
	map[18][27] = T_BLANKET;
	map[18][33] = T_TABLE;
	map[18][34] = T_TABLE;

	// Pastel stream across lower area + wooden bridge over path
	for (int x = 1; x < MAP_W - 1; x++) {
		map[41][x] = T_WATER;
		map[42][x] = T_WATER;
	}
	map[41][hx + 3] = T_BRIDGE;
	map[41][hx + 4] = T_BRIDGE;
	map[42][hx + 3] = T_BRIDGE;
	map[42][hx + 4] = T_BRIDGE;

	// Border: continuous rose-vine fence
	for (int x = 0; x < MAP_W; x++) { map[0][x] = T_FENCE; map[MAP_H - 1][x] = T_FENCE; }
	for (int y = 0; y < MAP_H; y++) { map[y][0] = T_FENCE; map[y][MAP_W - 1] = T_FENCE; }

	// Scatter garden props, flowers & trees outside protected areas
	for (int i = 0; i < 900; i++) {
		int x = 1 + rand() % (MAP_W - 2);
		int y = 1 + rand() % (VILLAGE_H - 2);
		if (map[y][x] != T_GRASS) continue;
		if (inYard(x, y)) continue;
		if (x >= 27 && x <= 36 && y >= 32 && y <= 36) continue;
		int r = rand() % 100;
		if (r < 6) { if (treeSpotOk(x, y)) map[y][x] = T_TREE; }
		else if (r < 12) map[y][x] = T_ROCK;
		else if (r < 18) map[y][x] = T_SUNFLOWER;
		else if (r < 28) map[y][x] = T_MUSH;
		else if (r < 52) map[y][x] = T_FLOWER;
	}

	// Dedicated flower & sunflower accents along fence and stream bank
	for (int i = 0; i < 14; i++) {
		int x = 25 + rand() % 10;
		int y = 19 + rand() % 3;
		if (map[y][x] == T_GRASS) map[y][x] = (i & 1) ? T_SUNFLOWER : T_FLOWER;
	}
	for (int x = 18; x < 44; x += 3) {
		if (map[40][x] == T_GRASS) map[40][x] = T_FLOWER;
	}

	// The cottage is drawn 1.5x bigger, so it needs a bigger footprint and a wider, taller garden
	// fence around it. Stamped after the random scatter so the rest of the village stays the same.
	for (int x = 24; x <= 35; x++) { map[11][x] = T_GRASS; map[22][x] = T_GRASS; }
	for (int y = 11; y <= 22; y++) { map[y][24] = T_GRASS; map[y][35] = T_GRASS; }
	for (int x = 22; x <= 37; x++) {
		map[9][x] = T_FENCE;
		if (x != hx + 3 && x != hx + 4) map[22][x] = T_FENCE;
	}
	for (int y = 9; y <= 22; y++) { map[y][22] = T_FENCE; map[y][37] = T_FENCE; }
	stampBuilding(hx - 2, hy - 2, hx + 6, hy + 3, 3);
	map[DOOR_TY][DOOR_TX] = T_DOOR;

	// Bakery on the west side of the village. It is stamped last (after the random
	// scatter) so the rest of the village looks exactly as before.
	for (int y = BAK_TY - 2; y <= BAK_TY + 6; y++)
		for (int x = BAK_TX - 2; x <= BAK_TX + 7; x++)
			map[y][x] = T_GRASS;
	stampBuilding(BAK_TX - 2, BAK_TY - 2, BAK_TX + 6, BAK_TY + 3, 3);
	map[BDOOR_TY][BDOOR_TX] = T_DOOR;
	for (int x = BAK_TX - 1; x <= BAK_TX + 6; x++) map[BAK_TY + 4][x] = T_PATH;
	for (int y = BAK_TY + 4; y <= 37; y++) {   // path down to the main road
		map[y][BDOOR_TX] = T_PATH;
		map[y][BDOOR_TX + 1] = T_PATH;
	}

	// ---- Village life. Everything below is stamped last, after the random scatter,
	// so the rest of the village stays exactly as it was.

	// Plaza with a fountain, benches and lamps (the cottage path runs through it)
	clearArea(27, 30, 36, 36);
	fillPath(28, 30, 35, 36);
	for (int y = FOUNTAIN_TY; y < FOUNTAIN_TY + 3; y++)
		for (int x = FOUNTAIN_TX - 1; x < FOUNTAIN_TX + 5; x++) map[y][x] = T_WALL;
	map[30][27] = T_LAMP; map[30][36] = T_LAMP;
	map[36][27] = T_LAMP; map[36][36] = T_LAMP;
	map[32][27] = T_TABLE; map[34][27] = T_TABLE;
	map[32][36] = T_TABLE; map[34][36] = T_TABLE;

	// Market street: a paved strip with three stalls behind it
	clearArea(17, 31, 26, 36);
	fillPath(17, 35, 26, 36);
	for (int i = 0; i < STALL_COUNT; i++)
		for (int y = STALLS[i][1]; y < STALLS[i][1] + 2; y++)
			for (int x = STALLS[i][0]; x < STALLS[i][0] + 2; x++) map[y][x] = T_WALL;

	// Sari-sari store on the east side of the village
	clearArea(SARI_TX - 2, SARI_TY - 2, SARI_TX + 7, SARI_TY + 6);
	stampBuilding(SARI_TX - 2, SARI_TY - 2, SARI_TX + 6, SARI_TY + 3, 3);
	map[SDOOR_TY][SDOOR_TX] = T_DOOR;
	for (int x = SARI_TX - 1; x <= SARI_TX + 6; x++) map[SARI_TY + 4][x] = T_PATH;
	for (int y = SARI_TY + 4; y <= 37; y++) {   // path down to the main road
		map[y][SDOOR_TX] = T_PATH;
		map[y][SDOOR_TX + 1] = T_PATH;
	}

	// Veggie garden: fenced crop rows, a scarecrow and a gate that opens onto a path
	clearArea(38, 19, 47, 26);
	for (int y = 21; y <= 24; y++)
		for (int x = 40; x <= 45; x++) map[y][x] = T_CROP;
	for (int x = 39; x <= 46; x++) {
		map[20][x] = T_FENCE;
		if (x != 42 && x != 43) map[25][x] = T_FENCE;
	}
	for (int y = 20; y <= 25; y++) { map[y][39] = T_FENCE; map[y][46] = T_FENCE; }
	map[22][43] = T_SCARE;
	fillPath(42, 25, 43, 36);

	// Lily pond with flowers and stones around it
	clearArea(13, 16, 20, 22);
	static const int POND[5][2] = { { 15, 18 }, { 14, 19 }, { 14, 19 }, { 14, 19 }, { 15, 18 } };
	for (int i = 0; i < 5; i++)
		for (int x = POND[i][0]; x <= POND[i][1]; x++) map[17 + i][x] = T_WATER;
	for (int y = 16; y <= 22; y++)
		for (int x = 13; x <= 20; x++) {
			if (map[y][x] != T_GRASS) continue;
			u32 q = hash2(x, y);
			if (q % 4 == 0) map[y][x] = T_FLOWER;
			else if (q % 11 == 1) map[y][x] = T_ROCK;
		}

	// Bahay kubo in the west and a neighbour's house in the east, each with a path to the road
	clearArea(HUT_TX - 1, HUT_TY - 2, HUT_TX + 4, HUT_TY + 3);
	stampBuilding(HUT_TX - 1, HUT_TY - 1, HUT_TX + 4, HUT_TY + 2, 1);
	fillPath(HUT_TX + 2, HUT_TY + 3, HUT_TX + 2, 36);

	clearArea(HOUSE_TX - 1, HOUSE_TY - 2, HOUSE_TX + 6, HOUSE_TY + 3);
	stampBuilding(HOUSE_TX - 1, HOUSE_TY - 1, HOUSE_TX + 5, HOUSE_TY + 2, 1);
	fillPath(HOUSE_TX + 2, HOUSE_TY + 3, HOUSE_TX + 2, HOUSE_TY + 4); // short front walkway

	// School on the east side: a 7 x 4 building with a clock gable, and a path to the main road
	clearArea(SCHOOL_TX - 2, SCHOOL_TY - 3, SCHOOL_TX + 8, SCHOOL_TY + 5);
	stampBuilding(SCHOOL_TX - 2, SCHOOL_TY - 2, SCHOOL_TX + 8, SCHOOL_TY + 3, 3);
	map[SCH_DOOR_TY][SCH_DOOR_TX] = T_DOOR;
	fillPath(SCHOOL_TX - 2, SCHOOL_TY + 4, SCHOOL_TX + 8, SCHOOL_TY + 4);
	fillPath(SCH_DOOR_TX, SCHOOL_TY + 4, SCH_DOOR_TX + 1, 36);

	// Street lamps along the south side of the main road
	for (int x = 5; x < MAP_W - 2; x += 6)
		if (map[39][x] == T_GRASS) map[39][x] = T_LAMP;

	// ---- The street across the bridge
	// sidewalks on both sides, then the road (it runs right off both edges of the map)
	for (int x = 1; x < MAP_W - 1; x++) {
		map[ROAD_Y0 - 2][x] = T_PATH;
		map[ROAD_Y0 - 1][x] = T_PATH;
		map[ROAD_Y1 + 1][x] = T_PATH;
	}
	for (int y = ROAD_Y0; y <= ROAD_Y1; y++)
		for (int x = 0; x < MAP_W; x++) map[y][x] = T_ROAD;

	for (int i = 0; i < 3; i++) {
		int bx = STREET_SHOP_TX[i];
		stampBuilding(bx - 2, STREET_TY - 2, bx + 6, STREET_TY + 3, 3);
		map[STREET_DOOR_TY][bx + 3] = T_DOOR;
	}

	// waiting shed (only its back half is solid)
	fillPath(SHED_TX, SHED_TY, SHED_TX + 4, SHED_TY + 1);
	for (int x = SHED_TX; x < SHED_TX + 5; x++) map[SHED_TY + 1][x] = T_WALL;

	// electric posts along the far sidewalk
	for (int x = 4; x < MAP_W - 3; x += 9) map[SHED_TY + 1][x] = T_POLE;

	// flowers and stones in the empty grass
	for (int y = 43; y < MAP_H - 1; y++)
		for (int x = 1; x < MAP_W - 1; x++) {
			if (map[y][x] != T_GRASS || (y >= STREET_TY - 1 && y < ROAD_Y0)) continue;
			u32 q = hash2(x, y);
			if (q % 7 == 0) map[y][x] = T_FLOWER;
			else if (q % 29 == 1) map[y][x] = T_ROCK;
		}

	// keep the kids' play areas free of flowers and stones
	clearArea(8, 47, 15, 50);   // tumbang preso
	clearArea(25, 47, 29, 50);  // piko
	clearArea(33, 47, 36, 50);  // Chinese garter
	clearArea(57, 46, 62, 50);  // patintero

	// trash bins beside the streets (never on a path, so nobody's way is blocked)
	static const int BINS[BIN_COUNT][2] = {
		{ 8, 36 }, { 39, 36 }, { 55, 36 }, { 9, 39 }, { 46, 39 },
		{ 17, 50 }, { 24, 50 }, { 37, 50 }, { 44, 50 }, { 48, 50 }, { 55, 50 },
		{ 10, 59 }, { 23, 59 }, { 29, 59 }, { 36, 59 }, { 45, 59 }, { 54, 59 }
	};
	for (int i = 0; i < BIN_COUNT; i++) {
		u8 *t = &map[BINS[i][1]][BINS[i][0]];
		if (*t == T_GRASS || *t == T_FLOWER || *t == T_ROCK || *t == T_MUSH || *t == T_SUNFLOWER)
			*t = T_TRASH;
	}

	stampBeach();
}

// ---------------------------------------------------------------- drawing helpers
// Everything is drawn through rect(), which applies an optional integer scale/offset.
// That lets the same character code draw big on the customize preview.

static int gsc = 1, gox = 0, goy = 0;
static int gAlpha = 255; // 255 = opaque; the hotbar lowers this to fade out

// Building scale: while bigOn is set, every rect() is grown BLD_NUM/BLD_DEN times around the
// anchor point (bigAx, bigAy). Both edges of a rect are rounded the same way, so neighbouring
// rects still meet exactly and the building has no gaps.
static int bigOn = 0, bigAx = 0, bigAy = 0;

static int bigMap(int v, int a) {
	int d = (v - a) * BLD_NUM;
	int q = d >= 0 ? d / BLD_DEN : -((-d + BLD_DEN - 1) / BLD_DEN); // floor division
	return a + q;
}

static void bigBegin(int ax, int ay) { bigOn = 1; bigAx = ax; bigAy = ay; }
static void bigEnd(void) { bigOn = 0; }

static void rect(int x, int y, int w, int h, u32 c) {
	if (w <= 0 || h <= 0) return;
	if (bigOn) {
		int x1 = bigMap(x + w, bigAx), y1 = bigMap(y + h, bigAy);
		x = bigMap(x, bigAx);
		y = bigMap(y, bigAy);
		w = x1 - x;
		h = y1 - y;
		if (w <= 0 || h <= 0) return;
	}
	if (gAlpha < 255) c = (c & 0x00FFFFFFu) | ((u32)((int)(c >> 24) * gAlpha / 255) << 24);
	C2D_DrawRectSolid(gox + x * gsc, goy + y * gsc, 0, w * gsc, h * gsc, c);
}

// A window that lights up at night. Call it right where the window is drawn: it follows the
// building scale (bigBegin) and is painted later by drawDayNight(), after the night tint.
#define MAX_WIN_GLOWS 48
static int winN = 0;
static int winR[MAX_WIN_GLOWS][4];
static void winGlow(int x, int y, int w, int h) {
	if (winN >= MAX_WIN_GLOWS) return;
	if (bigOn) {
		int x1 = bigMap(x + w, bigAx), y1 = bigMap(y + h, bigAy);
		x = bigMap(x, bigAx);
		y = bigMap(y, bigAy);
		w = x1 - x;
		h = y1 - y;
	}
	if (w <= 0 || h <= 0 || x > SCR_W + 20 || x + w < -20 || y > SCR_H + 20 || y + h < -20) return;
	winR[winN][0] = x; winR[winN][1] = y; winR[winN][2] = w; winR[winN][3] = h;
	winN++;
}

UNUSED_FN static void card(int x, int y, int w, int h, u32 fill, u32 bevel, u32 border) {
	rect(x + 1, y, w - 2, h, border);
	rect(x, y + 1, w, h - 2, border);
	rect(x + 1, y + 1, w - 2, h - 2, fill);
	rect(x + 1, y + h - 3, w - 2, 2, bevel);
	rect(x + w - 3, y + 1, 2, h - 2, bevel);
}

static void blob(int x, int y, int w, int h, u32 fill, u32 out) {
	rect(x + 1, y, w - 2, h, out);
	rect(x, y + 1, w, h - 2, out);
	rect(x + 1, y + 1, w - 2, h - 2, fill);
}

// rounded "pill" button shape with a 2px outline
static void pill(int x, int y, int w, int h, u32 fill, u32 out) {
	rect(x + 4, y, w - 8, h, out);
	rect(x + 2, y + 1, w - 4, h - 2, out);
	rect(x + 1, y + 2, w - 2, h - 4, out);
	rect(x, y + 4, w, h - 8, out);
	x += 2; y += 2; w -= 4; h -= 4;
	rect(x + 3, y, w - 6, h, fill);
	rect(x + 1, y + 1, w - 2, h - 2, fill);
	rect(x, y + 3, w, h - 6, fill);
}

static void drawHeart(int x, int y, int s, u32 c) {
	rect(x, y, 2 * s, 2 * s, c);
	rect(x + 4 * s, y, 2 * s, 2 * s, c);
	rect(x - s, y + s, 8 * s, 2 * s, c);
	rect(x, y + 3 * s, 6 * s, s, c);
	rect(x + s, y + 4 * s, 4 * s, s, c);
	rect(x + 2 * s, y + 5 * s, 2 * s, s, c);
}

static void drawHeartO(int x, int y, int s, u32 c, u32 out) {
	drawHeart(x - 1, y, s, out);
	drawHeart(x + 1, y, s, out);
	drawHeart(x, y - 1, s, out);
	drawHeart(x, y + 1, s, out);
	drawHeart(x, y, s, c);
}

UNUSED_FN static u32 shadeColor(u32 c, int dr, int dg, int db) {
	int r = (int)(c & 0xFF) + dr;
	int g = (int)((c >> 8) & 0xFF) + dg;
	int b = (int)((c >> 16) & 0xFF) + db;
	int a = (int)((c >> 24) & 0xFF);
	if (r < 0) r = 0;
	if (r > 255) r = 255;
	if (g < 0) g = 0;
	if (g > 255) g = 255;
	if (b < 0) b = 0;
	if (b > 255) b = 255;
	return C2D_Color32((u8)r, (u8)g, (u8)b, (u8)a);
}

static u32 mixc(u32 a, u32 b, float t) {
	int ar = a & 255, ag = (a >> 8) & 255, ab = (a >> 16) & 255;
	int br = b & 255, bg = (b >> 8) & 255, bb = (b >> 16) & 255;
	return RGB((int)(ar + (br - ar) * t), (int)(ag + (bg - ag) * t), (int)(ab + (bb - ab) * t));
}

// ---- chunky 5x7 pixel font (letters + '!')
static const u8 FONT[26][7] = {
	{0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // A
	{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, // B
	{0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, // C
	{0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}, // D
	{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, // E
	{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, // F
	{0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}, // G
	{0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, // H
	{0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}, // I
	{0x07,0x02,0x02,0x02,0x02,0x12,0x0C}, // J
	{0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // K
	{0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, // L
	{0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, // M
	{0x11,0x19,0x15,0x13,0x11,0x11,0x11}, // N
	{0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, // O
	{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // P
	{0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, // Q
	{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}, // R
	{0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}, // S
	{0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, // T
	{0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // U
	{0x11,0x11,0x11,0x11,0x11,0x0A,0x04}, // V
	{0x11,0x11,0x11,0x15,0x15,0x1B,0x11}, // W
	{0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}, // X
	{0x11,0x11,0x0A,0x04,0x04,0x04,0x04}, // Y
	{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}  // Z
};
static const u8 FONT_EXCL[7] = {0x04,0x04,0x04,0x04,0x04,0x00,0x04};

static const u8 *glyph(char ch) {
	if (ch >= 'A' && ch <= 'Z') return FONT[ch - 'A'];
	if (ch >= 'a' && ch <= 'z') return FONT[ch - 'a'];
	if (ch == '!') return FONT_EXCL;
	return NULL;
}

static int pixWidth(const char *s, int sc) {
	int n = (int)strlen(s);
	return n > 0 ? n * 6 * sc - sc : 0;
}

// draws each glyph row as merged horizontal runs (few rects), optionally grown by `grow` px
static void pixRuns(int x, int y, int sc, const char *s, u32 col, int grow) {
	for (int i = 0; s[i]; i++) {
		const u8 *g = glyph(s[i]);
		if (!g) continue;
		int cx = x + i * 6 * sc;
		for (int r = 0; r < 7; r++) {
			u8 bits = g[r];
			int c = 0;
			while (c < 5) {
				if (bits & (1 << (4 - c))) {
					int c2 = c;
					while (c2 + 1 < 5 && (bits & (1 << (4 - (c2 + 1))))) c2++;
					rect(cx + c * sc - grow, y + r * sc - grow,
					     (c2 - c + 1) * sc + 2 * grow, sc + 2 * grow, col);
					c = c2 + 1;
				} else c++;
			}
		}
	}
}

static void pixText(int x, int y, int sc, const char *s, u32 fill, u32 outline, u32 shadow) {
	int og = sc / 3;
	int so = (sc + 1) / 2 + 1;
	if (shadow) pixRuns(x + so, y + so, sc, s, shadow, og);
	if (outline && og > 0) pixRuns(x, y, sc, s, outline, og);
	pixRuns(x, y, sc, s, fill, 0);
}

static void pixTextC(int cx, int y, int sc, const char *s, u32 fill, u32 outline, u32 shadow) {
	pixText(cx - pixWidth(s, sc) / 2, y, sc, s, fill, outline, shadow);
}

// ---- world tiles

static bool isPathOrBridge(int tx, int ty) {
	if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) return false;
	return map[ty][tx] == T_PATH || map[ty][tx] == T_BRIDGE;
}

static bool isWaterOrBridge(int tx, int ty) {
	if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) return false;
	return map[ty][tx] == T_WATER || map[ty][tx] == T_BRIDGE;
}

// ---- rose-vine fence (auto-connecting)
// A fence piece links to any neighbouring fence piece, so straight runs, corners and
// gate openings all join up without gaps.
static bool fenceLinks(int tx, int ty) {
	if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) return false;
	return map[ty][tx] == T_FENCE;
}

static void fenceRail(int x, int y, int w) {
	rect(x, y, w, 1, FENCE_HI);
	rect(x, y + 1, w, 1, FENCE_MID);
	rect(x, y + 2, w, 1, FENCE_DK);
}

static void fenceBeam(int x, int y, int h) { // rail receding north/south
	rect(x, y, 4, h, FENCE_MID);
	rect(x, y, 1, h, FENCE_HI);
	rect(x + 3, y, 1, h, FENCE_DK);
}

static void drawFence(int sx, int sy, int tx, int ty) {
	u32 h = hash2(tx, ty);
	bool n = fenceLinks(tx, ty - 1), e = fenceLinks(tx + 1, ty);
	bool s = fenceLinks(tx, ty + 1), w = fenceLinks(tx - 1, ty);

	// ground shadow
	if (w) rect(sx, sy + 13, 8, 2, SHADOW);
	if (e) rect(sx + 8, sy + 13, 8, 2, SHADOW);
	rect(sx + 3, sy + 13, 10, 3, SHADOW);

	// rails (behind the post)
	if (w) { fenceRail(sx, sy + 4, 9); fenceRail(sx, sy + 9, 9); }
	if (e) { fenceRail(sx + 7, sy + 4, 9); fenceRail(sx + 7, sy + 9, 9); }
	if (n) fenceBeam(sx + 6, sy, 8);
	if (s) fenceBeam(sx + 6, sy + 8, 8);

	// post with a rounded top
	rect(sx + 6, sy + 1, 4, 1, FENCE_HI);
	rect(sx + 5, sy + 2, 6, 12, FENCE_MID);
	rect(sx + 5, sy + 2, 1, 12, FENCE_HI);
	rect(sx + 10, sy + 2, 1, 12, FENCE_DK);
	rect(sx + 5, sy + 13, 6, 1, FENCE_DK);

	// rose vine on roughly two out of three pieces
	if ((h % 3) != 0) {
		int ry = 5 + (int)((h >> 5) % 3);
		rect(sx + 7, sy + 3, 1, 10, VINE_DK);
		rect(sx + 4, sy + 3, 3, 1, VINE);
		rect(sx + 9, sy + 6, 3, 1, VINE);
		rect(sx + 4, sy + 9, 3, 1, VINE);
		rect(sx + 9, sy + 12, 3, 1, VINE);
		rect(sx + 3, sy + ry, 3, 3, ROSE);
		rect(sx + 4, sy + ry + 1, 1, 1, ROSE_DK);
		rect(sx + 10, sy + ry + 4, 3, 3, ROSE);
		rect(sx + 11, sy + ry + 5, 1, 1, ROSE_DK);
		if (e) { // a bud and leaves along the rail
			rect(sx + 12, sy + 7, 2, 2, ROSE);
			rect(sx + 13, sy + 5, 2, 1, VINE);
			rect(sx + 11, sy + 11, 2, 1, VINE);
		}
	}
}

static bool isBeachTile(u8 t) {
	return t == T_SAND || t == T_SHALLOW || t == T_SEA || t == T_BOARD ||
	       t == T_PALM || t == T_PARASOL || t == T_TOWEL || t == T_SIGN;
}

static void drawSandBase(int sx, int sy, int tx, int ty) {
	u32 h = hash2(tx, ty);
	bool wet = ty >= SHALLOW_Y0 - 1;
	rect(sx, sy, TS, TS, wet ? SAND_WET : SAND_MID);
	if (!wet) {
		if (((h >> 3) & 3) == 0) rect(sx + (int)(h & 7), sy + 3 + (int)((h >> 6) & 7), 6, 2, SAND_HI);
		if ((h & 3) == 1) rect(sx + (int)((h >> 4) % 12) + 1, sy + (int)((h >> 8) % 12) + 1, 2, 1, SAND_DK);
		if (((h >> 10) % 5) == 0) rect(sx + (int)((h >> 12) % 13), sy + (int)((h >> 16) % 13), 1, 1, SAND_DK);
	} else {
		rect(sx + 2 + (int)(h & 7), sy + 5 + (int)((h >> 5) & 3), 6, 1, SAND_DK);
	}
	if (ty == BEACH_Y0 && (h % 3) == 0) { // dune grass
		rect(sx + 4, sy + 3, 1, 4, GRASS_TUFT);
		rect(sx + 6, sy + 1, 1, 6, GRASS_TUFT);
		rect(sx + 8, sy + 4, 1, 3, GRASS_TUFT);
	}
	if (!wet && ty > BEACH_Y0) {
		if ((h % 23) == 0) { // seashell
			rect(sx + 5, sy + 7, 5, 3, PAPER);
			rect(sx + 6, sy + 6, 3, 1, BLOSSOM_HI);
			rect(sx + 7, sy + 8, 1, 2, BLOSSOM_DK);
		} else if ((h % 41) == 1) { // starfish
			u32 sc = RGB(240, 150, 100);
			rect(sx + 7, sy + 4, 2, 8, sc);
			rect(sx + 4, sy + 7, 8, 2, sc);
			rect(sx + 7, sy + 7, 2, 2, RGB(255, 200, 150));
		}
	}
}

static void drawShallowBase(int sx, int sy, int tx, int ty) {
	u32 h = hash2(tx, ty);
	rect(sx, sy, TS, TS, SHALLOW_C);
	int wave = ((frameCount / 14 + tx * 2 + ty) % 8) - 2;
	rect(sx + 3 + wave, sy + 7, 5, 1, SHALLOW_HI);
	rect(sx + 8 - wave, sy + 12, 4, 1, SHALLOW_HI);
	if (ty == SHALLOW_Y0) { // surf creeping up the sand
		int surge = (frameCount / 24 + tx) % 4;
		rect(sx, sy, TS, 1 + surge / 2, FOAM);
		rect(sx + surge * 3, sy + 2 + surge / 2, 7, 1, FOAM);
		if (((h >> 4) & 3) == 0) rect(sx + 9, sy + 4, 4, 1, FOAM);
	}
	if (ty == SEA_Y0 - 1) rect(sx, sy + TS - 2, TS, 2, SEA_LT);
}

static void drawSeaBase(int sx, int sy, int tx, int ty) {
	u32 h = hash2(tx, ty);
	rect(sx, sy, TS, TS, ty >= SEA_Y0 + 2 ? SEA_DK : SEA_MID);
	int wave = ((frameCount / 10 + tx * 3 + ty * 2) % 10) - 3;
	rect(sx + 2 + wave, sy + 4, 6, 1, SEA_LT);
	rect(sx + 8 - wave, sy + 11, 5, 1, SEA_LT);
	if ((h % 9) == 0) rect(sx + 3 + (int)((h >> 5) % 8), sy + 7, 4, 1, FOAM); // whitecap
	if (ty == SEA_Y0) rect(sx, sy, TS, 2, SEA_LT);
}

static void drawTile(u8 t, int sx, int sy, int tx, int ty) {
	u32 h = hash2(tx, ty);
	int swi = ((frameCount / 18 + tx + ty) % 3) - 1;

	// flat sage-green grass with soft patches, tufts and tiny daisies
	if (t != T_PATH && t != T_WATER && t != T_BLANKET && t != T_CROP && !isBeachTile(t)) {
		rect(sx, sy, TS, TS, GRASS_A);
		if (((h >> 3) & 7) == 0) rect(sx + (h & 7), sy + 4 + ((h >> 6) & 7), 7, 3, GRASS_B);
		if ((h & 3) == 0) {
			int ox = ((h >> 4) % 9) + 3;
			int oy = ((h >> 8) % 9) + 3;
			rect(sx + ox, sy + oy + 1, 3, 1, GRASS_TUFT);
			rect(sx + ox + 1, sy + oy, 1, 2, GRASS_TUFT);
		}
		if (((h >> 12) % 13) == 0)
			rect(sx + 3 + (int)((h >> 16) % 10), sy + 3 + (int)((h >> 20) % 10), 2, 2, PAPER);
	}

	switch (t) {
	case T_ROAD: { // dark asphalt with Philippine road markings
		rect(sx, sy, TS, TS, RGB(88, 88, 94));
		bool cross = tx >= CROSS_X0 && tx <= CROSS_X1;
		if (cross) { // pedestrian lane
			rect(sx + 1, sy + 2, 14, 5, RGB(236, 236, 228));
			rect(sx + 1, sy + 10, 14, 5, RGB(236, 236, 228));
		} else {
			if (ty == ROAD_Y0) rect(sx, sy + 3, TS, 2, RGB(226, 226, 218));
			if (ty == ROAD_Y1) rect(sx, sy + 11, TS, 2, RGB(226, 226, 218));
			if (ty == ROAD_Y0 + 2) { // double yellow centre line
				rect(sx, sy + 5, TS, 2, RGB(240, 190, 56));
				rect(sx, sy + 9, TS, 2, RGB(240, 190, 56));
			}
		}
		// yellow-and-black painted curbs
		for (int k = 0; k < 2; k++) {
			u32 c = k ? INK : GOLD_UI;
			if (ty == ROAD_Y0) rect(sx + k * 8, sy, 8, 2, c);
			if (ty == ROAD_Y1) rect(sx + k * 8, sy + TS - 2, 8, 2, c);
		}
		break;
	}
	case T_PATH: { // paved stone sidewalk with slab seams and a curb
		rect(sx, sy, TS, TS, PATH_MID);
		// two rows of 16 x 8 slabs per tile; every other row is shifted by half a slab,
		// so the seams line up across neighbouring tiles instead of wandering around
		for (int r = 0; r < 2; r++) {
			int y = sy + r * 8;
			int vx = (((ty * 2 + r) & 1) ? 7 : 15);
			rect(sx, y, TS, 1, PATH_HI);           // lit top edge of the slab
			rect(sx, y + 7, TS, 1, PATH_SEAM);     // seam below it
			rect(sx + vx, y + 1, 1, 6, PATH_SEAM); // vertical seam
		}
		if (!isPathOrBridge(tx, ty - 1)) rect(sx, sy, TS, 1, PATH_EDGE);
		if (!isPathOrBridge(tx, ty + 1)) rect(sx, sy + TS - 2, TS, 2, PATH_EDGE);
		if (!isPathOrBridge(tx - 1, ty)) rect(sx, sy, 1, TS, PATH_EDGE);
		if (!isPathOrBridge(tx + 1, ty)) rect(sx + TS - 1, sy, 1, TS, PATH_EDGE);
		break;
	}
	case T_WATER: {
		rect(sx, sy, TS, TS, WATER_MID);
		if (!isWaterOrBridge(tx, ty - 1)) rect(sx, sy, TS, 2, WATER_HI);
		if (!isWaterOrBridge(tx, ty + 1)) rect(sx, sy + TS - 2, TS, 2, WATER_DK);
		int wave = ((frameCount / 12 + tx * 2 + ty) % 8) - 2;
		rect(sx + 3 + wave, sy + 6, 5, 1, WATER_HI);
		rect(sx + 7 - wave, sy + 11, 4, 1, WATER_HI);
		if (ty < 40 && (h % 5) == 0) { // lily pad (only on the pond, not the stream)
			int lx = sx + 3 + (int)((h >> 5) % 8), ly = sy + 4 + (int)((h >> 9) % 6);
			rect(lx, ly, 6, 4, LEAF_MID);
			rect(lx + 1, ly - 1, 4, 1, LEAF_MID);
			rect(lx + 3, ly + 1, 3, 1, WATER_MID);
			if ((h >> 13) & 1) rect(lx + 2, ly - 2, 2, 2, BLOSSOM_PALE);
		}
		break;
	}
	case T_CROP: { // tilled soil with little vegetable plants
		rect(sx, sy, TS, TS, SOIL);
		rect(sx, sy + 4, TS, 1, SOIL_DK);
		rect(sx, sy + 11, TS, 1, SOIL_DK);
		int stage = (int)((h >> 3) % 3);
		for (int k = 0; k < 2; k++) {
			int cx = sx + 3 + k * 6, cy = sy + k * 7;
			rect(cx + 2, cy + 2, 1, 4, LEAF_DK);
			rect(cx, cy + 1, 2, 2, LEAF_HI);
			if (stage >= 1) rect(cx + 3, cy, 2, 2, LEAF_MID);
			if (stage == 2) rect(cx + 1, cy + 4, 2, 2, ((h >> (7 + k)) & 1) ? ROSE : GOLD_UI);
		}
		break;
	}
	case T_BRIDGE: { // wooden plank bridge with side rails
		rect(sx, sy, TS, TS, WATER_MID);
		rect(sx, sy + 1, TS, TS - 2, DECK_MID);
		for (int k = 0; k < 4; k++) {
			rect(sx, sy + 3 + k * 4, TS, 1, DECK_DK);
			rect(sx, sy + 1 + k * 4, TS, 1, DECK_HI);
		}
		if (tx > 0 && map[ty][tx - 1] != T_BRIDGE) rect(sx, sy, 2, TS, BENCH_DK);
		if (tx < MAP_W - 1 && map[ty][tx + 1] != T_BRIDGE) rect(sx + TS - 2, sy, 2, TS, BENCH_DK);
		break;
	}
	case T_TABLE: { // wooden bench with an occasional tea cup
		rect(sx, sy + 13, TS, 2, SHADOW);
		rect(sx, sy + 3, TS, 9, BENCH_DK);
		rect(sx, sy + 3, TS, 3, BENCH_HI);
		rect(sx, sy + 6, TS, 3, BENCH_MID);
		rect(sx, sy + 9, TS, 1, BENCH_DK);
		rect(sx + 2, sy + 11, 2, 4, BENCH_DK);
		rect(sx + 12, sy + 11, 2, 4, BENCH_DK);
		if ((tx & 1) == 0) {
			rect(sx + 6, sy + 1, 4, 3, PAPER);
			rect(sx + 7, sy + 1, 2, 1, SHOP_WOOD);
		}
		break;
	}
	case T_BLANKET: { // wooden deck, with a pink floor cushion on every other plank tile
		rect(sx, sy, TS, TS, DECK_MID);
		for (int k = 0; k < 4; k++) {
			rect(sx, sy + k * 4, TS, 1, DECK_HI);
			rect(sx, sy + k * 4 + 3, TS, 1, DECK_DK);
			rect(sx + 1 + (int)(hash2(tx, ty + k * 7) % 14), sy + k * 4 + 1, 1, 2, DECK_DK);
		}
		if ((tx & 1) == 0) {
			blob(sx + 3, sy + 4, 10, 8, NOREN, NOREN_DK);
			rect(sx + 5, sy + 6, 6, 1, BLOSSOM_HI);
		}
		break;
	}
	case T_SUNFLOWER: { // low hydrangea shrub
		rect(sx + 3, sy + 12, 10, 2, SHADOW);
		blob(sx + 3, sy + 5, 10, 8, LEAF_MID, LEAF_DK);
		rect(sx + 5, sy + 6, 6, 2, LEAF_HI);
		rect(sx + 4 + swi / 2, sy + 4, 3, 3, flowerCols[(h >> 2) % 5]);
		rect(sx + 9 + swi / 2, sy + 6, 3, 3, flowerCols[(h >> 5) % 5]);
		rect(sx + 6, sy + 9, 3, 3, flowerCols[(h >> 8) % 5]);
		break;
	}
	case T_SAND:
		drawSandBase(sx, sy, tx, ty);
		break;
	case T_SHALLOW:
		drawShallowBase(sx, sy, tx, ty);
		break;
	case T_SEA:
		drawSeaBase(sx, sy, tx, ty);
		break;
	case T_PALM:
	case T_PARASOL:
	case T_SIGN: // drawn in their own pass, standing on sand
		drawSandBase(sx, sy, tx, ty);
		break;
	case T_TOWEL: { // beach towel with white stripes
		drawSandBase(sx, sy, tx, ty);
		u32 tc = ((tx / 3) & 1) ? ROSE : SKY_UI;
		rect(sx, sy + 3, TS, 11, tc);
		rect(sx, sy + 3, TS, 1, INK_SOFT);
		rect(sx, sy + 13, TS, 1, INK_SOFT);
		rect(sx, sy + 6, TS, 2, WHITE);
		rect(sx, sy + 10, TS, 1, WHITE);
		break;
	}
	case T_BOARD: { // wooden boardwalk; out over the water it gets side rails
		rect(sx, sy, TS, TS, DECK_MID);
		for (int k = 0; k < 4; k++) {
			rect(sx, sy + k * 4, TS, 1, DECK_HI);
			rect(sx, sy + k * 4 + 3, TS, 1, DECK_DK);
			rect(sx + 1 + (int)(hash2(tx, ty + k * 7) % 14), sy + k * 4 + 1, 1, 2, DECK_DK);
		}
		int rail = ty >= SHALLOW_Y0 ? 2 : 1;
		if (map[ty][tx - 1] != T_BOARD) rect(sx, sy, rail, TS, BENCH_DK);
		if (map[ty][tx + 1] != T_BOARD) rect(sx + TS - rail, sy, rail, TS, BENCH_DK);
		if (map[ty + 1][tx] != T_BOARD) rect(sx, sy + TS - 3, TS, 3, BENCH_DK); // end of the pier
		break;
	}
	case T_TREE:
	case T_APPLE: // trees are drawn in their own pass by drawTree()
		break;
	case T_ROCK:
		rect(sx + 2, sy + 12, 12, 3, SHADOW);
		if ((h % 3) == 0) { // stone garden lantern
			rect(sx + 4, sy + 12, 8, 3, STONE_DK);
			rect(sx + 7, sy + 8, 2, 5, STONE_MID);
			rect(sx + 4, sy + 4, 8, 5, STONE_MID);
			rect(sx + 6, sy + 5, 4, 3, SHOP_GLOW);
			rect(sx + 2, sy + 2, 12, 3, STONE_HI);
			rect(sx + 4, sy, 8, 2, STONE_MID);
			rect(sx + 7, sy - 1, 2, 1, STONE_HI);
		} else { // mossy boulder
			blob(sx + 2, sy + 5, 12, 9, STONE_MID, STONE_DK);
			rect(sx + 4, sy + 6, 5, 2, STONE_HI);
			rect(sx + 9, sy + 5, 4, 2, LEAF_MID);
		}
		break;
	case T_ROOF:
	case T_WALL:
	case T_DOOR: // the whole cottage is drawn by drawCottage()
		break;
	case T_FENCE: // brown post-and-rail fence with rose vines
		drawFence(sx, sy, tx, ty);
		break;
	case T_MUSH: // flat stepping stones
		blob(sx + 1 + (h & 3), sy + 3, 9, 6, STONE_MID, STONE_DK);
		rect(sx + 3 + (h & 3), sy + 4, 4, 1, STONE_HI);
		blob(sx + 8, sy + 9, 7, 5, STONE_MID, STONE_DK);
		rect(sx + 9, sy + 10, 3, 1, STONE_HI);
		break;
	case T_FLOWER:
		for (int k = 0; k < 2; k++) {
			int fx = sx + 4 + k * 7 + ((h >> (k * 4)) & 1);
			int fy = sy + 5 + ((h >> (k * 3 + 2)) & 2);
			u32 c = flowerCols[(h >> (k * 5 + 1)) % 5];
			rect(fx, fy + 2, 1, 5, LEAF_MID);
			rect(fx - 1 + swi, fy - 2, 3, 5, c);
			rect(fx - 2 + swi, fy - 1, 5, 3, c);
			rect(fx + swi, fy, 1, 1, SHOP_GLOW);
		}
		break;
	default:
		break;
	}
}

// ---------------------------------------------------------------- flower cottage
// A small pot or bucket with three blooms. (x, y) is the top-left of the pot.
static void drawPot(int x, int y, u32 pot, u32 bloom) {
	rect(x + 1, y - 4, 1, 4, VINE_DK);
	rect(x + 4, y - 6, 1, 6, VINE_DK);
	rect(x + 6, y - 3, 1, 3, VINE_DK);
	rect(x + 2, y - 2, 2, 1, VINE);
	rect(x, y - 6, 3, 3, bloom);
	rect(x + 3, y - 8, 3, 3, bloom);
	rect(x + 5, y - 5, 3, 3, bloom);
	rect(x + 1, y - 5, 1, 1, GOLD_UI);
	rect(x + 4, y - 7, 1, 1, GOLD_UI);
	rect(x + 6, y - 4, 1, 1, GOLD_UI);
	rect(x, y, 8, 2, INK_SOFT);
	rect(x + 1, y + 1, 6, 4, pot);
}

// Drawn in one piece over its 6x4 tiles; (x, y) is the top-left of the footprint.
static void drawCottage(int x, int y) {
	u32 roofHi = RGB(236, 150, 124), roofMid = RGB(210, 112, 98), roofDk = RGB(150, 72, 74);
	u32 wall = RGB(250, 232, 184), wallShd = RGB(226, 200, 150);
	u32 mintHi = RGB(166, 212, 190), mint = RGB(116, 176, 156), mintDk = RGB(66, 112, 106);
	u32 glass = RGB(184, 220, 222);

	rect(x + 2, y + 62, 94, 4, SHADOW);

	// chimney
	rect(x + 72, y - 6, 12, 16, INK);
	rect(x + 73, y - 5, 10, 15, STONE_MID);
	rect(x + 71, y - 7, 14, 3, STONE_DK);

	// rounded hip roof with scalloped shingles
	for (int r = 0; r < 6; r++) {
		int inset = (5 - r) * 3;
		int ry = y + r * 4;
		rect(x - 2 + inset, ry, 100 - inset * 2, 4, roofDk);
		for (int sx = x - 1 + inset; sx < x + 97 - inset; sx += 7) {
			int w = x + 97 - inset - sx;
			if (w > 6) w = 6;
			rect(sx, ry, w, 3, roofMid);
			rect(sx, ry, w, 1, roofHi);
		}
	}
	rect(x + 13, y - 2, 70, 2, roofDk);

	// butter-yellow walls with timber frame and stone footing
	rect(x + 1, y + 24, 94, 40, INK);
	rect(x + 2, y + 24, 92, 39, wall);
	rect(x + 2, y + 24, 92, 3, wallShd);
	rect(x + 2, y + 24, 3, 39, SHOP_WOOD);
	rect(x + 91, y + 24, 3, 39, SHOP_WOOD);
	rect(x + 2, y + 59, 92, 4, STONE_MID);
	for (int k = 0; k < 11; k++) rect(x + 5 + k * 8, y + 59, 1, 4, STONE_DK);

	// shop sign: BULAKLAK means "flower"
	blob(x + 20, y + 20, 56, 13, PAPER, INK);
	rect(x + 22, y + 30, 52, 1, PAPER_DK);
	pixText(x + 25, y + 23, 1, "BULAKLAK", ROSE_DK, 0, 0);

	// display window on the left
	rect(x + 7, y + 40, 36, 19, INK);
	rect(x + 8, y + 41, 34, 17, SHOP_WOOD);
	rect(x + 10, y + 43, 30, 13, SHOP_GLOW);
	winGlow(x + 10, y + 43, 30, 13);
	rect(x + 24, y + 43, 2, 13, SHOP_WOOD);
	drawPot(x + 12, y + 51, RGB(196, 118, 90), flowerCols[2]);
	drawPot(x + 29, y + 51, RGB(196, 118, 90), flowerCols[3]);

	// striped awning
	for (int k = 0; k < 8; k++) {
		u32 c = (k & 1) ? PAPER : NOREN;
		rect(x + 5 + k * 5, y + 35, 5, 7, c);
		rect(x + 6 + k * 5, y + 42, 3, 2, c);
	}
	rect(x + 5, y + 35, 40, 1, NOREN_DK);

	// arched mint door (matches the door tile)
	rect(x + 52, y + 36, 9, 2, INK);
	rect(x + 50, y + 38, 13, 2, INK);
	rect(x + 49, y + 40, 15, 24, INK);
	rect(x + 52, y + 38, 9, 2, mint);
	rect(x + 50, y + 40, 13, 23, mint);
	rect(x + 51, y + 40, 1, 23, mintHi);
	rect(x + 56, y + 49, 1, 14, mintDk);
	rect(x + 53, y + 41, 7, 7, mintDk);
	rect(x + 54, y + 42, 5, 5, SHOP_GLOW);
	winGlow(x + 54, y + 42, 5, 5);
	rect(x + 59, y + 52, 2, 2, GOLD_UI);
	rect(x + 47, y + 63, 19, 2, STONE_HI);

	// window with flower box on the right
	blob(x + 70, y + 37, 18, 17, SHOP_WOOD, INK);
	rect(x + 73, y + 40, 12, 11, glass);
	rect(x + 78, y + 40, 2, 11, SHOP_WOOD);
	rect(x + 73, y + 45, 12, 2, SHOP_WOOD);
	rect(x + 68, y + 53, 22, 5, SHOP_WOOD_DK);
	for (int k = 0; k < 6; k++) {
		rect(x + 69 + k * 4, y + 51, 3, 3, flowerCols[(k + 1) % 5]);
		rect(x + 71 + k * 4, y + 53, 2, 2, VINE);
	}

	// flower buckets by the door
	drawPot(x + 38, y + 59, STONE_HI, flowerCols[0]);
	drawPot(x + 80, y + 59, STONE_HI, flowerCols[1]);
}

// ---------------------------------------------------------------- bakery
// a plump little bread roll / loaf
static void bun(int x, int y, int w, int h, u32 c, u32 d) {
	blob(x, y, w, h, c, d);
	rect(x + 2, y + 1, w / 2, 1, mixc(c, WHITE, 0.5f));
}

// Drawn in one piece over its 6x4 tiles; (x, y) is the top-left of the footprint.
static void drawBakery(int x, int y) {
	u32 cream = RGB(246, 232, 208), creamDk = RGB(214, 190, 164);
	u32 brick = RGB(200, 142, 122), brickDk = RGB(158, 100, 94), brickHi = RGB(224, 172, 148);
	u32 wood = RGB(148, 72, 70), woodDk = RGB(104, 48, 52), woodHi = RGB(186, 100, 92);
	u32 awn = RGB(238, 150, 164), glow = RGB(252, 220, 160);

	rect(x + 2, y + 62, 94, 4, SHADOW);

	// chimney with drifting oven steam
	rect(x + 76, y - 8, 10, 12, INK);
	rect(x + 77, y - 7, 8, 11, STONE_MID);
	rect(x + 75, y - 9, 12, 3, STONE_DK);
	for (int k = 0; k < 3; k++) {
		int t = (frameCount / 2 + k * 20) % 60;
		int sway = (int)(sinf((frameCount + k * 37) * 0.06f) * 3.0f);
		int s = 3 + t / 20;
		rect(x + 79 + sway, y - 12 - t / 2, s, s, RGBA(255, 250, 250, 190 - t * 3));
	}

	// cream stone parapet and brick band with little shuttered windows
	rect(x - 2, y, 100, 6, INK);
	rect(x - 1, y + 1, 98, 4, cream);
	for (int k = 0; k < 12; k++) rect(x + 2 + k * 8, y + 1, 1, 4, creamDk);
	rect(x, y + 6, 96, 17, brick);
	rect(x, y + 6, 96, 1, brickHi);
	for (int r = 0; r < 3; r++) {
		rect(x, y + 10 + r * 5, 96, 1, brickDk);
		for (int k = 0; k < 8; k++) rect(x + 4 + k * 12 + (r & 1) * 6, y + 6 + r * 5, 1, 5, brickDk);
	}
	for (int k = 0; k < 3; k++) {
		int wx = x + 10 + k * 31;
		rect(wx, y + 8, 16, 14, INK);
		rect(wx + 1, y + 9, 14, 12, awn);
		rect(wx + 4, y + 10, 8, 10, glow);
		rect(wx + 7, y + 10, 2, 10, woodDk);
	}

	// dark red timber front with a stone footing
	rect(x, y + 23, 96, 41, INK);
	rect(x + 1, y + 23, 94, 40, wood);
	rect(x + 1, y + 23, 94, 2, woodHi);
	for (int k = 0; k < 5; k++) rect(x + 16 + k * 16, y + 25, 1, 34, woodDk);
	rect(x + 1, y + 59, 94, 4, STONE_MID);
	for (int k = 0; k < 7; k++) rect(x + 8 + k * 13, y + 59, 1, 4, STONE_DK);

	// BAKERY sign
	blob(x + 14, y + 28, 68, 12, woodDk, INK);
	rect(x + 16, y + 29, 64, 1, woodHi);
	pixTextC(x + 48, y + 31, 1, "BAKERY", NOREN, 0, 0);

	// rose ivy draped across the front
	rect(x + 4, y + 22, 88, 3, VINE);
	rect(x + 4, y + 24, 88, 1, VINE_DK);
	for (int k = 0; k < 11; k++) {
		u32 q = hash2(k, 99);
		int hx = x + 6 + k * 8;
		int hl = 3 + (int)(q % 7);
		rect(hx, y + 25, 2, hl, VINE);
		rect(hx + 1, y + 25 + hl, 2, 2, LEAF_HI);
		if ((q >> 4) & 1) rect(hx - 1, y + 22 + (int)((q >> 8) % 3), 2, 2, BLOSSOM_PALE);
	}

	// bread display window
	rect(x + 6, y + 41, 40, 19, INK);
	rect(x + 7, y + 42, 38, 17, woodHi);
	rect(x + 9, y + 44, 34, 14, glow);
	winGlow(x + 9, y + 44, 34, 14);
	rect(x + 10, y + 44, 6, 1, WHITE);
	rect(x + 9, y + 51, 34, 1, BREAD_DK);
	bun(x + 11, y + 46, 10, 5, BREAD_MID, BREAD_DK);
	bun(x + 24, y + 45, 12, 6, BREAD_HI, BREAD_DK);
	bun(x + 37, y + 46, 5, 5, BREAD_MID, BREAD_DK);
	for (int k = 0; k < 4; k++) {
		rect(x + 11 + k * 8, y + 53, 6, 4, BREAD_MID);
		rect(x + 11 + k * 8, y + 53, 6, 1, BREAD_HI);
	}
	rect(x + 4, y + 60, 44, 2, creamDk);

	// door
	rect(x + 49, y + 46, 15, 18, INK);
	rect(x + 50, y + 47, 13, 17, woodDk);
	rect(x + 51, y + 47, 1, 17, woodHi);
	rect(x + 53, y + 48, 7, 7, INK);
	rect(x + 54, y + 49, 5, 5, glow);
	rect(x + 56, y + 49, 1, 5, woodDk);
	rect(x + 61, y + 56, 2, 2, GOLD_UI);
	rect(x + 47, y + 63, 19, 2, STONE_HI);

	// small window to the right of the door
	rect(x + 68, y + 49, 22, 11, INK);
	rect(x + 69, y + 50, 20, 9, glow);
	winGlow(x + 69, y + 50, 20, 9);
	rect(x + 78, y + 50, 2, 9, woodDk);

	// pink striped awning over the door and side window
	for (int k = 0; k < 9; k++) {
		u32 c = (k & 1) ? cream : awn;
		rect(x + 47 + k * 5, y + 38, 5, 7, c);
		rect(x + 48 + k * 5, y + 45, 3, 2, c);
	}
	rect(x + 47, y + 38, 45, 1, NOREN_DK);

	// flower pots
	drawPot(x + 38, y + 59, RGB(196, 118, 90), BLOSSOM_MID);
	drawPot(x + 80, y + 59, STONE_HI, flowerCols[0]);
}

// ---------------------------------------------------------------- village landmarks
// Each of these is drawn in one piece over its tiles; (x, y) is the top-left of the footprint.

// stone fountain with a spraying centre column (4 x 3 tiles)
static void drawFountain(int x, int y) {
	rect(x + 2, y + 44, 62, 4, SHADOW);

	// stone basin
	blob(x + 2, y + 16, 60, 30, STONE_MID, INK);
	rect(x + 6, y + 18, 52, 2, STONE_HI);
	rect(x + 4, y + 38, 56, 6, STONE_DK);
	for (int k = 0; k < 6; k++) rect(x + 8 + k * 9, y + 38, 1, 6, STONE_MID);

	// water with drifting ripples and a few wishing coins
	blob(x + 8, y + 21, 48, 18, WATER_MID, STONE_DK);
	rect(x + 10, y + 22, 44, 2, WATER_HI);
	int w = (frameCount / 10) % 6;
	rect(x + 14 + w, y + 27, 8, 1, WATER_HI);
	rect(x + 38 - w, y + 31, 10, 1, WATER_HI);
	rect(x + 22 + w, y + 35, 6, 1, WATER_HI);
	rect(x + 16, y + 32, 2, 2, GOLD_UI);
	rect(x + 46, y + 28, 2, 2, GOLD_UI);

	// centre column and upper bowl
	rect(x + 29, y + 10, 6, 26, STONE_MID);
	rect(x + 29, y + 10, 2, 26, STONE_HI);
	blob(x + 22, y + 6, 20, 7, STONE_HI, STONE_DK);
	rect(x + 30, y + 1, 4, 5, STONE_HI);

	// spray: droplets rise, curve outward and fall back into the basin
	rect(x + 31, y + 1 + (frameCount / 3) % 3, 2, 5, WATER_HI);
	for (int side = -1; side <= 1; side += 2)
		for (int k = 0; k < 3; k++) {
			int t = (frameCount / 2 + k * 8) % 24;
			int dx = 2 + t / 2;
			int dy = (t - 8) * (t - 8) / 12 - 4;
			rect(x + 31 + side * dx, y + 4 + dy, 2, 2, WATER_HI);
		}
}

// market stall with a striped awning: 0 = mangoes, 1 = vegetables, 2 = fish (2 x 2 tiles)
static void drawStall(int x, int y, int v) {
	u32 awn = (v == 0) ? NOREN : (v == 1) ? MINT_UI : SKY_UI;
	rect(x + 2, y + 29, 30, 3, SHADOW);
	rect(x + 1, y + 8, 2, 22, SHOP_WOOD_DK);
	rect(x + 29, y + 8, 2, 22, SHOP_WOOD_DK);

	// counter
	rect(x + 2, y + 18, 28, 11, SHOP_WOOD);
	rect(x + 2, y + 18, 28, 3, SHOP_WOOD_HI);
	rect(x + 2, y + 27, 28, 2, SHOP_WOOD_DK);
	rect(x + 10, y + 22, 1, 5, SHOP_WOOD_DK);
	rect(x + 21, y + 22, 1, 5, SHOP_WOOD_DK);

	// goods on the counter
	for (int k = 0; k < 5; k++) {
		int gx = x + 4 + k * 5;
		if (v == 0) {          // mangoes
			blob(gx, y + 13, 5, 5, GOLD_UI, BREAD_MID);
			rect(gx + 3, y + 12, 2, 1, LEAF_MID);
		} else if (v == 1) {   // vegetables
			rect(gx, y + 14, 4, 4, (k & 1) ? ROSE_DK : LEAF_HI);
			rect(gx + 1, y + 12, 2, 2, LEAF_DK);
		} else {               // fresh fish on ice
			rect(gx, y + 15, 5, 3, STONE_HI);
			rect(gx + 4, y + 16, 2, 1, STONE_MID);
			rect(gx + 1, y + 15, 1, 1, INK);
		}
	}
	if (v == 2) rect(x + 3, y + 18, 26, 1, WATER_HI);

	// striped awning
	for (int k = 0; k < 8; k++) {
		rect(x + k * 4, y + 2, 4, 8, (k & 1) ? PAPER : awn);
		rect(x + k * 4 + 1, y + 10, 2, 2, (k & 1) ? PAPER : awn);
	}
	rect(x, y + 2, 32, 1, NOREN_DK);
}

// bahay kubo: a nipa hut on stilts with a thatched roof (4 x 3 tiles)
static void drawHut(int x, int y) {
	rect(x + 2, y + 44, 62, 4, SHADOW);

	// stilts and floor
	for (int k = 0; k < 4; k++) {
		rect(x + 6 + k * 16, y + 30, 3, 14, BARK);
		rect(x + 8 + k * 16, y + 30, 1, 14, BARK_DK);
	}
	rect(x + 4, y + 28, 56, 3, BARK_DK);

	// woven bamboo walls
	rect(x + 6, y + 14, 52, 14, BAMBOO);
	for (int k = 0; k < 13; k++) rect(x + 6 + k * 4, y + 14, 1, 14, BAMBOO_DK);
	rect(x + 6, y + 20, 52, 1, BAMBOO_DK);

	// window with a propped-open shutter, and the doorway
	rect(x + 12, y + 17, 12, 9, SHOP_DARK);
	rect(x + 13, y + 18, 10, 7, SHOP_GLOW);
	winGlow(x + 13, y + 18, 10, 7);
	rect(x + 9, y + 13, 18, 3, THATCH_DK);
	rect(x + 34, y + 16, 12, 12, SHOP_DARK);
	rect(x + 36, y + 18, 8, 10, SHOP_WOOD_DK);

	// ladder
	rect(x + 36, y + 28, 1, 16, BARK);
	rect(x + 43, y + 28, 1, 16, BARK);
	for (int k = 0; k < 4; k++) rect(x + 36, y + 31 + k * 4, 8, 1, BARK);

	// thatched roof
	for (int r = 0; r < 5; r++) {
		int inset = (4 - r) * 7;
		int w = 68 - inset * 2;
		int ry = y - 6 + r * 4;
		rect(x - 2 + inset, ry, w, 4, THATCH_DK);
		rect(x - 2 + inset, ry, w, 3, THATCH);
		for (int sxx = x + inset + 1; sxx < x - 2 + inset + w - 2; sxx += 6) rect(sxx, ry + 1, 1, 3, THATCH_DK);
	}
	rect(x + 30, y - 9, 4, 3, THATCH);

	drawPot(x + 50, y + 40, RGB(196, 118, 90), flowerCols[2]);
	drawPot(x + 12, y + 40, STONE_HI, flowerCols[1]);
}

// neighbour's cosy house with shuttered windows and flower boxes (5 x 3 tiles)
static void drawHouse(int x, int y) {
	u32 wall = RGB(250, 226, 208), wallShd = RGB(226, 196, 174);
	u32 glass = RGB(184, 220, 222), mint = RGB(116, 176, 156), mintDk = RGB(66, 112, 106);

	rect(x + 2, y + 44, 80, 4, SHADOW);

	// chimney (drawn first so the roof covers its foot)
	rect(x + 58, y - 14, 10, 14, INK);
	rect(x + 59, y - 13, 8, 13, STONE_MID);
	rect(x + 57, y - 15, 12, 3, STONE_DK);

	// hip roof with shingle seams
	for (int r = 0; r < 5; r++) {
		int inset = (4 - r) * 6;
		int w = 84 - inset * 2;
		int ry = y - 8 + r * 5;
		rect(x - 2 + inset, ry, w, 5, ROOF_DK);
		rect(x - 2 + inset, ry, w, 4, ROOF_MID);
		rect(x - 2 + inset, ry, w, 1, ROOF_HI);
		for (int sxx = x + inset + 4; sxx < x - 2 + inset + w - 2; sxx += 8) rect(sxx, ry + 1, 1, 3, ROOF_DK);
	}

	// walls and stone footing
	rect(x, y + 17, 80, 28, INK);
	rect(x + 1, y + 17, 78, 27, wall);
	rect(x + 1, y + 17, 78, 2, wallShd);
	rect(x + 1, y + 41, 78, 4, STONE_MID);
	for (int k = 0; k < 9; k++) rect(x + 4 + k * 9, y + 41, 1, 4, STONE_DK);

	// two shuttered windows with flower boxes
	for (int k = 0; k < 2; k++) {
		int wx = x + 8 + k * 46;
		rect(wx, y + 22, 18, 14, INK);
		rect(wx + 1, y + 23, 16, 12, glass);
	winGlow(wx + 1, y + 23, 16, 12);
		rect(wx + 8, y + 23, 2, 12, SHOP_WOOD);
		rect(wx + 1, y + 28, 16, 2, SHOP_WOOD);
		rect(wx - 4, y + 22, 4, 14, NOREN);
		rect(wx + 18, y + 22, 4, 14, NOREN);
		rect(wx - 1, y + 36, 20, 3, SHOP_WOOD_DK);
		for (int j = 0; j < 4; j++) rect(wx + 1 + j * 5, y + 34, 3, 3, flowerCols[(j + k) % 5]);
	}

	// mint front door
	rect(x + 33, y + 24, 14, 21, INK);
	rect(x + 34, y + 25, 12, 20, mint);
	rect(x + 39, y + 33, 1, 12, mintDk);
	rect(x + 36, y + 27, 8, 6, mintDk);
	rect(x + 37, y + 28, 6, 4, SHOP_GLOW);
	winGlow(x + 37, y + 28, 6, 4);
	rect(x + 43, y + 37, 2, 2, GOLD_UI);
	rect(x + 31, y + 44, 18, 2, STONE_HI);
}

// sari-sari store: corrugated tin roof, snack window and drink crates (6 x 4 tiles)
static void drawSariStore(int x, int y) {
	u32 wall = RGB(124, 188, 176), wallDk = RGB(84, 144, 138);
	u32 pal[4] = { ROSE, GOLD_UI, MINT_UI, SKY_UI };

	rect(x + 2, y + 62, 96, 4, SHADOW);

	// corrugated tin roof, three overlapping sheets
	for (int s = 0; s < 3; s++) {
		int rx = x + 6 - s * 5, ry = y - 2 + s * 6, rw = 84 + s * 10;
		rect(rx, ry, rw, 8, TIN_MID);
		rect(rx, ry, rw, 1, TIN_HI);
		rect(rx, ry + 7, rw, 1, TIN_DK);
		for (int k = 3; k < rw - 2; k += 6) rect(rx + k, ry + 1, 1, 6, TIN_DK);
	}
	rect(x + 70, y + 12, 6, 3, RGB(168, 110, 84)); // a rusty patch

	// planked front wall with a stone footing
	rect(x + 1, y + 18, 94, 46, INK);
	rect(x + 2, y + 18, 92, 45, wall);
	for (int k = 0; k < 5; k++) rect(x + 2, y + 25 + k * 8, 92, 1, wallDk);
	rect(x + 2, y + 59, 92, 4, STONE_MID);

	// sign board
	blob(x + 12, y + 20, 72, 14, GOLD_UI, INK);
	pixTextC(x + 48, y + 24, 1, "SARI SARI", ROSE_DK, 0, 0);

	// open window: hanging chichirya packs above, shelves of goodies inside
	rect(x + 6, y + 35, 40, 1, INK);
	for (int k = 0; k < 6; k++) rect(x + 8 + k * 6, y + 36, 4, 5, pal[(k + 1) % 4]);
	rect(x + 6, y + 42, 40, 18, INK);
	rect(x + 7, y + 43, 38, 16, SHOP_DARK);
	winGlow(x + 7, y + 43, 38, 16);
	rect(x + 7, y + 51, 38, 1, SHOP_WOOD_DK);
	for (int k = 0; k < 7; k++) {
		rect(x + 9 + k * 5, y + 46, 3, 5, pal[k % 4]);
		rect(x + 9 + k * 5, y + 53, 3, 5, pal[(k + 2) % 4]);
	}
	rect(x + 4, y + 60, 44, 3, SHOP_WOOD);
	rect(x + 4, y + 60, 44, 1, SHOP_WOOD_HI);

	// door
	rect(x + 49, y + 36, 15, 28, INK);
	rect(x + 50, y + 37, 13, 27, SHOP_WOOD);
	rect(x + 51, y + 37, 1, 27, SHOP_WOOD_HI);
	rect(x + 52, y + 40, 9, 10, INK);
	rect(x + 53, y + 41, 7, 8, SHOP_GLOW);
	winGlow(x + 53, y + 41, 7, 8);
	rect(x + 56, y + 41, 1, 8, SHOP_WOOD_DK);
	rect(x + 60, y + 53, 2, 2, GOLD_UI);
	rect(x + 47, y + 63, 19, 2, STONE_HI);

	// drink crates outside on the right
	rect(x + 70, y + 50, 22, 12, SHOP_WOOD_DK);
	rect(x + 71, y + 51, 20, 10, SHOP_WOOD);
	rect(x + 71, y + 55, 20, 1, SHOP_WOOD_DK);
	for (int k = 0; k < 5; k++) {
		rect(x + 72 + k * 4, y + 43, 3, 8, pal[k % 4]);
		rect(x + 73 + k * 4, y + 41, 1, 2, STONE_HI);
	}
}

// ---------------------------------------------------------------- the school (outside)
// 7 x 4 tiles. Cream walls, a coral tile roof, a clock gable and a Philippine flag on top.
// The door is the tile at SCH_DOOR_TX (x + 48 .. x + 64).
static void drawSchool(int x, int y) {
	u32 wall = RGB(250, 228, 176), wallDk = RGB(214, 170, 112);
	u32 roof = RGB(204, 98, 88), roofHi = RGB(230, 134, 118), roofDk = RGB(146, 62, 64);
	u32 glass = RGB(176, 218, 236), curtain = RGB(212, 76, 84);

	rect(x + 2, y + 62, 108, 4, SHADOW);

	// tiled roof
	rect(x - 4, y + 4, 120, 22, roofDk);
	rect(x - 3, y + 5, 118, 19, roof);
	rect(x - 3, y + 5, 118, 2, roofHi);
	for (int r = 1; r < 4; r++) rect(x - 3, y + 5 + r * 5, 118, 1, roofDk);
	for (int r = 0; r < 4; r++)
		for (int k = (r & 1) * 5; k < 116; k += 10) rect(x - 3 + k, y + 6 + r * 5, 1, 4, roofDk);

	// clock gable in the middle of the roof, built from stepped rows
	rect(x + 50, y - 10, 12, 1, INK);
	for (int r = 0; r < 8; r++) {
		int half = 6 + r * 2;
		rect(x + 56 - half - 1, y - 9 + r * 3, half * 2 + 2, 3, INK);
		rect(x + 56 - half, y - 9 + r * 3, half * 2, 3, wall);
	}
	blob(x + 49, y - 1, 14, 14, PAPER, INK);       // clock face
	rect(x + 56, y + 2, 1, 5, INK);
	rect(x + 56, y + 6, 4, 1, INK);

	// flag pole with the Philippine flag
	rect(x + 56, y - 31, 2, 22, STONE_DK);
	rect(x + 55, y - 34, 4, 3, GOLD_UI);
	rect(x + 58, y - 31, 16, 10, INK);
	rect(x + 59, y - 30, 14, 4, RGB(0, 56, 168));
	rect(x + 59, y - 26, 14, 4, RGB(206, 17, 38));
	rect(x + 59, y - 30, 3, 8, WHITE);
	rect(x + 62, y - 29, 2, 6, WHITE);
	rect(x + 64, y - 28, 1, 4, WHITE);
	rect(x + 60, y - 27, 1, 2, GOLD_UI);

	// eave, then the front wall
	rect(x - 4, y + 22, 120, 4, roofDk);
	rect(x - 4, y + 22, 120, 1, roofHi);
	rect(x + 1, y + 26, 110, 38, INK);
	rect(x + 2, y + 26, 108, 37, wall);
	rect(x + 2, y + 27, 108, 2, SHADOW);
	rect(x + 2, y + 57, 108, 6, wallDk);

	// sign board
	blob(x + 36, y + 29, 40, 11, PAPER, INK);
	pixTextC(x + 56, y + 32, 1, "SCHOOL", ROSE_DK, 0, 0);

	// double door (the tile you stand in front of), with a step
	rect(x + 47, y + 41, 18, 23, INK);
	rect(x + 48, y + 42, 16, 22, SHOP_WOOD);
	rect(x + 55, y + 42, 2, 22, SHOP_WOOD_DK);
	rect(x + 50, y + 45, 4, 8, SHOP_GLOW);
	rect(x + 58, y + 45, 4, 8, SHOP_GLOW);
	winGlow(x + 50, y + 45, 4, 8);
	winGlow(x + 58, y + 45, 4, 8);
	rect(x + 53, y + 56, 2, 2, GOLD_UI);
	rect(x + 57, y + 56, 2, 2, GOLD_UI);
	rect(x + 44, y + 63, 24, 3, STONE_HI);
	rect(x + 46, y + 61, 20, 2, STONE_MID);

	// four classroom windows with red curtains
	static const int WIN_X[4] = { 8, 26, 72, 90 };
	for (int k = 0; k < 4; k++) {
		int wx = x + WIN_X[k];
		rect(wx - 1, y + 41, 16, 19, INK);
		rect(wx, y + 42, 14, 17, glass);
	winGlow(wx, y + 42, 14, 17);
		rect(wx + 6, y + 42, 2, 17, SHOP_WOOD);
		rect(wx, y + 49, 14, 2, SHOP_WOOD);
		rect(wx, y + 42, 3, 8, curtain);
		rect(wx + 11, y + 42, 3, 8, curtain);
		rect(wx - 2, y + 59, 18, 2, STONE_HI);
	}

	// little bushes with flowers at both ends
	blob(x - 3, y + 52, 11, 12, LEAF_MID, LEAF_DK);
	rect(x + 1, y + 55, 2, 2, ROSE);
	blob(x + 105, y + 52, 11, 12, LEAF_MID, LEAF_DK);
	rect(x + 108, y + 55, 2, 2, GOLD_UI);
}

// ---------------------------------------------------------------- game state

typedef struct {
	float x, y;
	float fx, fy;
	int wp;
	int wait;
} Npc;

// Jay strolls a loop around the (bigger) cottage, crossing the front path below the door.
static const int WP[4][2] = { {24, 10}, {35, 10}, {35, 17}, {24, 17} };

static float px, py;
static float pfx = 0, pfy = 1;
static bool playerMoving = false;
static Npc jay;
static bool insideHouse = false;

// ---- bakery shop state
static int shopId = 0;                 // which shop we are inside: 0 = flower shop, 1 = bakery
static bool shopMenu = false;          // the bakery buy menu is open
static bool tahoMenu = false;          // Mang Taho's buy menu is open
static const char *tahoMsg = NULL;
static int shopSel = 0;
static const char *shopMsg = NULL;
static int coins = 100;
// every item you can buy: 0-3 are sold at the bakery, 4-7 at the sari-sari store
static const char *ITEM_NAMES[ITEM_COUNT] = {
	"Pandesal", "Ensaymada", "Pan de Coco", "Hopia",
	"Sago't Gulaman", "Turon", "Chichirya", "Pastillas",
	"Pink Blouse", "Blue Shirt", "Navy Bottoms", "Red Shoes",
	"Dog Food", "Cat Food", "Bola", "Collar",
	"Adobo Rice", "Sinigang", "Lugaw", "Halo-Halo",
	"Taho"
};
static const int ITEM_PRICE[ITEM_COUNT] = {
	5, 15, 10, 8,     7, 6, 10, 5,
	25, 25, 20, 30,   8, 8, 12, 15,
	20, 22, 10, 15,
	10
};
#define CLOTHES_BASE 8    // clothes are worn right away instead of going in the bag
#define ITEM_TAHO 20      // sold by Mang Taho, who walks around the map
#define ITEM_DOG_FOOD 12
#define ITEM_CAT_FOOD 13
static int shopBase(void) { return (shopId - 1) * BAKE_COUNT; } // first item of the shop you are in

// ---- inventory hotbar (Minecraft style): 9 slots, each holds one kind of item
#define HOTBAR_SLOTS 9
#define STACK_MAX 99
static int invType[HOTBAR_SLOTS];     // -1 = empty slot, otherwise a bakery item (0..BAKE_COUNT-1)
static int invCount[HOTBAR_SLOTS];
static int hotbarSel = 0;             // the slot you are holding
static int hbNameTimer = 0;           // frames left to show the held item's name
static int hbFade = 255;              // hotbar opacity (fades when the player walks behind it)

// Puts one item into the hotbar: tops up a matching stack first, then uses the first empty slot.
static bool addItem(int type) {
	for (int i = 0; i < HOTBAR_SLOTS; i++)
		if (invType[i] == type && invCount[i] < STACK_MAX) { invCount[i]++; return true; }
	for (int i = 0; i < HOTBAR_SLOTS; i++)
		if (invType[i] < 0) { invType[i] = type; invCount[i] = 1; return true; }
	return false; // every slot is full
}

static int itemTotal(int type) {
	int n = 0;
	for (int i = 0; i < HOTBAR_SLOTS; i++)
		if (invType[i] == type) n += invCount[i];
	return n;
}

// ---- eating: hold food in the hotbar and press Y
#define FULL_MAX 100        // the busog (fullness) bar goes from 0 to 100
#define EAT_TIME 60         // frames an eating animation lasts
static int fullness = 60;           // 0 = gutom na gutom, 100 = busog na busog
static int hungerTick = 0;          // counts frames until the bar drops by 1
static int eatTimer = 0;            // > 0 while the player is eating
static int eatItem = -1;            // the food being eaten
static const char *eatMsg = NULL;   // speech bubble over the player's head
static int eatMsgTimer = 0;

// how much each item fills you up; -1 = not food (clothes, pet food, accessories)
static int foodValue(int item) {
	static const int V[ITEM_COUNT] = {
		12, 22, 18, 15,    // pandesal, ensaymada, pan de coco, hopia
		10, 16, 12, 8,     // sago't gulaman, turon, chichirya, pastillas
		-1, -1, -1, -1,    // clothes
		-1, -1, -1, -1,    // pet shop items
		40, 35, 28, 25,    // adobo rice, sinigang, lugaw, halo-halo
		15                 // taho
	};
	return (item >= 0 && item < ITEM_COUNT) ? V[item] : -1;
}
static bool isDrink(int item) { return item == 4 || item == 20; } // sago't gulaman, taho

static void eatSay(const char *msg) { eatMsg = msg; eatMsgTimer = 110; }

// Y: start eating the item in the selected hotbar slot
static void tryEat(void) {
	int it = invType[hotbarSel];
	if (it < 0) { eatSay("Walang hawak na pagkain!"); return; }
	if (it == ITEM_DOG_FOOD || it == ITEM_CAT_FOOD) { eatSay("Pagkain 'yan ng alaga!"); return; }
	if (foodValue(it) < 0) { eatSay("Hindi 'yan pagkain!"); return; }
	if (fullness >= FULL_MAX) { eatSay("Busog na busog na ako!"); return; }
	eatItem = it;
	eatTimer = EAT_TIME;
	if (--invCount[hotbarSel] == 0) invType[hotbarSel] = -1;
	hbNameTimer = 0;
}

// runs every frame while eating; the food only fills you up once you finish it
static void updateEating(void) {
	if (eatTimer <= 0) return;
	if (--eatTimer > 0) return;
	fullness += foodValue(eatItem);
	if (fullness > FULL_MAX) fullness = FULL_MAX;
	if (fullness >= FULL_MAX) eatSay("Busog na busog na ako!");
	else eatSay(isDrink(eatItem) ? "Ahh, refreshing!" : "Yum! Ang sarap!");
	eatItem = -1;
}

// ---- NPC dialogue
static bool dialogActive = false;
static int dialogLine = 0;
#define JAY_LINES 4
static const char *dialogJay[JAY_LINES] = {
	"Uy, kumusta ka?",
	"Ako si Jay. Dito ako madalas maglakad.",
	"Ang ganda ng mga bulaklak ni Aling Rosa, 'no?",
	"Sige, kita tayo ulit mamaya!"
};
static bool talkingToOwner = false;

// Aling Rosa, the flower shop owner (Tagalog)
#define OWNER_X 104     // screen position inside the shop (behind the counter)
#define OWNER_FEET 102
#define OWNER_LINES 5
static const char *dialogOwner[OWNER_LINES] = {
	"Magandang araw! Tuloy ka, anak.",
	"Ako si Aling Rosa. Akin ang tindahang ito.",
	"Aba, mahilig ka ba sa mga bulaklak?",
	"Huwag kang mahiya. Sabihin mo ang nararamdaman mo.",
	"Ingat ka, at bumalik ka ulit ha!"
};

// Ate Mila, the baker (Tagalog)
#define BAKER_LINES 4
static const char *dialogBaker[BAKER_LINES] = {
	"Magandang umaga! Bagong luto ang pandesal!",
	"Ako si Ate Mila. Akin ang panaderyang ito.",
	"Ang bango, 'no? Mainit pa ang ensaymada.",
	"Ano'ng gusto mo ngayon?"
};

// Kuya Ben, the sari-sari store owner (Tagalog)
#define SARI_LINES 4
static const char *dialogSari[SARI_LINES] = {
	"Uy, suki! Tuloy ka sa tindahan ko.",
	"Ako si Kuya Ben. Sari-sari ang paninda ko dito.",
	"May turon, chichirya, at malamig na gulaman!",
	"Ano'ng bibilhin mo ngayon?"
};

// The school: Miss Luz and Sir Dante (teachers), Angel, Miko and Renz (students)
#define SCHOOL_LINES 3
static int talkingSchool = -1;   // school character you are chatting with, or -1
static const char *dialogSchool[SCHOOL_NPC_COUNT][SCHOOL_LINES] = {
	{
		"Magandang umaga! Tuloy ka sa klase namin.",
		"Ako si Miss Luz. Ako ang guro nila dito.",
		"Mag-aral nang mabuti at huwag kalimutang ngumiti!"
	}, {
		"Uy, magandang araw! Ako si Sir Dante.",
		"Math at Science ang tinuturo ko dito.",
		"Basta masipag ka, kayang-kaya mo 'yan!"
	}, {
		"Hi po! Ako si Angel. Grade 3 po ako.",
		"Gumagawa po ako ng assignment sa Filipino.",
		"Gusto ko pong maging guro paglaki ko!"
	}, {
		"Hello po! Ako si Miko.",
		"Paborito ko po ang recess, lalo na ang turon!",
		"Shh, tahimik lang po tayo sa loob ng klase."
	}, {
		"Shh... nagbabasa po ako ng libro.",
		"Ako po si Renz. Mahilig po ako sa mga kuwento.",
		"Gusto mo rin bang magbasa? Masaya po 'to!"
	}
};

// Ate Joy (ukay-ukay), Mang Tonyo (pet shop) and Aling Nena (karenderya)
#define STREET_LINES 4
static const char *dialogStreet[3][STREET_LINES] = {
	{
		"Hi, ganda! Pasok ka sa ukay-ukay ko.",
		"Ako si Ate Joy. Mura lang ang mga damit dito.",
		"Bagay sa'yo ang kahit anong kulay!",
		"Pili ka na. Isusuot mo agad!"
	}, {
		"Uy! Mahilig ka rin ba sa alagang hayop?",
		"Ako si Mang Tonyo. Akin ang pet shop na ito.",
		"Hawakan ang pagkain, tapos lapitan ang aso o pusa.",
		"Ano'ng kailangan ng alaga mo?"
	}, {
		"Kain na! Bagong luto ang ulam!",
		"Ako si Aling Nena. Akin ang karenderyang ito.",
		"Masarap ang sinigang ko, maasim-asim pa!",
		"Ano'ng order mo?"
	}
};
static const char *SHOP_KEEPER[SHOP_COUNT] = {
	"ALING ROSA", "ATE MILA", "KUYA BEN", "ATE JOY", "MANG TONYO", "ALING NENA", ""
};

// the clothes sold at the ukay-ukay: which outfit setting each one changes
static int *wearSlot(int item) {
	if (item < CLOTHES_BASE + 2) return &outfitIndex;
	return item == CLOTHES_BASE + 2 ? &skirtIndex : &shoeIndex;
}
static int wearValue(int item) {
	static const int V[BAKE_COUNT] = { 3, 1, 1, 2 }; // pink top, sky top, navy bottoms, red shoes
	return V[item - CLOTHES_BASE];
}
static bool isClothes(int item) { return item >= CLOTHES_BASE && item < CLOTHES_BASE + BAKE_COUNT; }

static C2D_TextBuf textBuf;

static void drawText(float x, float y, float scale, u32 clr, const char *s) {
	C2D_Text t;
	C2D_TextParse(&t, textBuf, s);
	C2D_TextOptimize(&t);
	C2D_DrawText(&t, C2D_WithColor, x, y, 0.5f, scale, scale, clr);
}

// ---------------------------------------------------------------- seasonal events
// Halloween and Pasko. In AUTO the 3DS clock decides which one is on; the bottom-screen button
// can also force one, so you can play either event on any day.
enum { EV_NONE, EV_HALLOWEEN, EV_PASKO };
enum { EVM_AUTO, EVM_HALLOWEEN, EVM_PASKO, EVM_OFF, EVM_COUNT };
static int eventMode = EVM_AUTO;
static int activeEvent = EV_NONE;
// who has already given you a treat: one bit per grown-up, kept apart for each event
static u32 giftMask[2];
static int giftYear[2];        // the season (year) each mask belongs to; a new year starts over

// gift givers: 0 = Jay, then the villagers, the shopkeepers, and the two teachers
#define GIVER_WALKER0 1
#define GIVER_SHOP0   (GIVER_WALKER0 + WALKER_COUNT)
#define GIVER_TEACH0  (GIVER_SHOP0 + SCHOOL_ID)
#define GIVER_COUNT   (GIVER_TEACH0 + 2)

#define TREAT_COINS     5     // Halloween: coins that come with the candy
#define TREAT_CANDY     7     // Halloween: the candy is Pastillas (goes into the hotbar)
#define AGUINALDO_COINS 20    // Pasko
#define ALL_GIFTS_BONUS 100   // extra coins when every grown-up has given you one

static int calendarEvent(int month, int day) { // month: 0 = January
	if (month == 9 || (month == 10 && day <= 2)) return EV_HALLOWEEN;
	if (month == 10 || month == 11 || (month == 0 && day <= 6)) return EV_PASKO;
	return EV_NONE;
}

static void refreshEvent(void) {
	time_t now = time(NULL);
	struct tm *tmv = localtime(&now);
	int month = tmv ? tmv->tm_mon : 5, day = tmv ? tmv->tm_mday : 1;
	int year = tmv ? tmv->tm_year + 1900 : 2000;
	if (month == 0) year--; // early January still belongs to last year's Pasko
	if (eventMode == EVM_HALLOWEEN) activeEvent = EV_HALLOWEEN;
	else if (eventMode == EVM_PASKO) activeEvent = EV_PASKO;
	else if (eventMode == EVM_OFF) activeEvent = EV_NONE;
	else activeEvent = calendarEvent(month, day);
	if (activeEvent != EV_NONE && giftYear[activeEvent - 1] != year) {
		giftYear[activeEvent - 1] = year;
		giftMask[activeEvent - 1] = 0;
	}
}

static int giftsGot(void) {
	if (activeEvent == EV_NONE) return 0;
	int n = 0;
	for (int i = 0; i < GIVER_COUNT; i++)
		if (giftMask[activeEvent - 1] & (1u << i)) n++;
	return n;
}

// During an event every chat gets one extra greeting line at the start.
static int eventLines(void) { return activeEvent != EV_NONE ? 1 : 0; }

// ---------------------------------------------------------------- weather
// Two kinds of weather: sunny and rainy. It changes by itself every minute or two,
// and you can tap the weather button on the bottom screen to change it yourself.
enum { W_SUNNY, W_RAINY };
static int weather = W_SUNNY;
static int weatherTimer = 0;    // frames until the weather changes by itself
static float rainLevel = 0.0f;  // 0 = completely dry, 1 = full rain (fades in and out)

static void setWeather(int w) {
	weather = w;
	weatherTimer = (w == W_SUNNY) ? 3600 + rand() % 3600 : 1800 + rand() % 2400;
}

static void updateWeather(void) {
	if (--weatherTimer <= 0) setWeather(weather == W_SUNNY ? W_RAINY : W_SUNNY);
	const float step = 1.0f / 180.0f; // roughly 3 seconds to fade
	if (weather == W_RAINY) {
		rainLevel += step;
		if (rainLevel > 1.0f) rainLevel = 1.0f;
	} else {
		rainLevel -= step;
		if (rainLevel < 0.0f) rainLevel = 0.0f;
	}
}

// ---------------------------------------------------------------- day and night
// One full day lasts DAY_FRAMES frames. dayTime goes 0..1: 0 = midnight, 0.25 = 6:00 (sunrise),
// 0.5 = noon, 0.75 = 18:00 (sunset). The whole village is tinted by a colour that is blended
// between the keyframes below, so sunrise and sunset fade in and out smoothly.
#define DAY_FRAMES 25200            // 7 minutes at 60 fps
#define DAY_START  0.33f            // a new game starts on a bright morning
typedef struct { float t; int r, g, b, a; } SkyKey;
static const SkyKey SKY_KEYS[] = {
	{ 0.00f,  18,  26,  70, 120 },  // midnight
	{ 0.20f,  34,  40, 100, 104 },  // just before dawn
	{ 0.26f, 255, 140, 110,  64 },  // sunrise
	{ 0.33f, 255, 210, 160,  22 },  // early morning
	{ 0.41f, 255, 240, 200,   0 },  // full day
	{ 0.62f, 255, 240, 200,   0 },
	{ 0.68f, 255, 205, 120,  22 },  // late afternoon
	{ 0.74f, 255, 110,  70,  72 },  // sunset
	{ 0.80f, 110,  56, 120, 100 },  // dusk
	{ 0.88f,  24,  30,  76, 118 },  // night
	{ 1.00f,  18,  26,  70, 120 },  // midnight again
};
#define SKY_KEY_COUNT ((int)(sizeof(SKY_KEYS) / sizeof(SKY_KEYS[0])))
enum { PH_NIGHT, PH_SUNRISE, PH_DAY, PH_SUNSET };
static const float PHASE_START[4] = { 0.80f, 0.22f, 0.31f, 0.70f }; // indexed by PH_*
static float dayTime = DAY_START;

static void updateDayTime(void) {
	dayTime += 1.0f / DAY_FRAMES;
	if (dayTime >= 1.0f) dayTime -= 1.0f;
}

static void skyTint(float t, int *r, int *g, int *b, int *a) {
	int i = 0;
	while (i < SKY_KEY_COUNT - 2 && t > SKY_KEYS[i + 1].t) i++;
	const SkyKey *k0 = &SKY_KEYS[i], *k1 = &SKY_KEYS[i + 1];
	float f = (t - k0->t) / (k1->t - k0->t);
	if (f < 0.0f) f = 0.0f;
	if (f > 1.0f) f = 1.0f;
	*r = k0->r + (int)((k1->r - k0->r) * f);
	*g = k0->g + (int)((k1->g - k0->g) * f);
	*b = k0->b + (int)((k1->b - k0->b) * f);
	*a = k0->a + (int)((k1->a - k0->a) * f);
}

// 0 = bright day, 1 = deep night (ramps through sunrise and sunset)
static float nightLevel(void) {
	float t = dayTime;
	if (t <= 0.20f) return 1.0f;
	if (t < 0.30f) return 1.0f - (t - 0.20f) / 0.10f;
	if (t <= 0.74f) return 0.0f;
	if (t < 0.86f) return (t - 0.74f) / 0.12f;
	return 1.0f;
}

static int dayPhase(void) {
	if (dayTime >= 0.22f && dayTime < 0.31f) return PH_SUNRISE;
	if (dayTime >= 0.31f && dayTime < 0.70f) return PH_DAY;
	if (dayTime >= 0.70f && dayTime < 0.80f) return PH_SUNSET;
	return PH_NIGHT;
}

static void skipToNextPhase(void) {
	float best = 2.0f;
	for (int i = 0; i < 4; i++) { // the nearest phase start after now (wrapping past midnight)
		float d = PHASE_START[i] - dayTime;
		if (d <= 0.001f) d += 1.0f;
		if (d < best) best = d;
	}
	dayTime += best;
	if (dayTime >= 1.0f) dayTime -= 1.0f;
}

// ---------------------------------------------------------------- village pets
// Two askal (Philippine street dogs) and two puspin (Philippine street cats)
// wander near their favourite spots. Press A next to one to pet it.
#define PET_COUNT 5
enum { PET_DOG, PET_CAT, PET_KALABAW };
enum { PS_IDLE, PS_WALK, PS_SLEEP };

typedef struct {
	float x, y;          // feet position in world pixels
	float tx, ty;        // where it is walking to
	int kind;            // PET_DOG, PET_CAT or PET_KALABAW
	int pattern;         // coat pattern (0 or 1)
	int dir;             // 1 = facing right, -1 = facing left
	int state;           // PS_IDLE / PS_WALK / PS_SLEEP
	int timer;           // frames left in the current state
	int homeX, homeY;    // favourite tile
	int range;           // how many tiles from home it may roam
	int happy;           // frames left showing a heart while being petted
	const char *name;
	const char *sound;
	u32 coat, coatDk, patch, ear, collar; // collar == 0 means no collar
} Pet;

static Pet pets[PET_COUNT];

// feet box of a pet; pets stop at trees, rocks, fences, water and buildings
static bool petBlocked(float x, float y) {
	return solid(tileAt(x - 4, y - 3)) || solid(tileAt(x + 4, y - 3)) ||
		   solid(tileAt(x - 4, y)) || solid(tileAt(x + 4, y));
}

static void initPet(Pet *p, const char *name, int kind, int pattern, int hx, int hy, int range) {
	memset(p, 0, sizeof(*p));
	p->name = name;
	p->kind = kind;
	p->pattern = pattern;
	p->homeX = hx;
	p->homeY = hy;
	p->range = range;
	p->sound = kind == PET_DOG ? "Aw aw!" : kind == PET_CAT ? "Ngiyaw!" : "Unga~!";
	p->dir = 1;
	p->state = PS_IDLE;
	p->timer = 30 + rand() % 120;
	p->x = hx * TS + TS / 2;
	p->y = hy * TS + TS - 3;
	// start on the nearest free spot around the home tile
	bool found = !petBlocked(p->x, p->y);
	for (int r = 1; r <= 6 && !found; r++)
		for (int dy = -r; dy <= r && !found; dy++)
			for (int dx = -r; dx <= r && !found; dx++) {
				float fx = (hx + dx) * TS + TS / 2;
				float fy = (hy + dy) * TS + TS - 3;
				if (!petBlocked(fx, fy)) { p->x = fx; p->y = fy; found = true; }
			}
}

static void initPets(void) {
	// Bantay: brown askal with a pink collar
	initPet(&pets[0], "Bantay", PET_DOG, 0, 36, 36, 7);
	pets[0].coat = RGB(186, 134, 82);
	pets[0].coatDk = RGB(138, 96, 58);
	pets[0].patch = RGB(250, 240, 222);
	pets[0].ear = RGB(120, 80, 50);
	pets[0].collar = ROSE_DK;

	// Tagpi: black-and-white askal ("tagpi" means patch)
	initPet(&pets[1], "Tagpi", PET_DOG, 1, 19, 35, 7);
	pets[1].coat = RGB(70, 62, 70);
	pets[1].coatDk = RGB(44, 38, 48);
	pets[1].patch = WHITE;
	pets[1].ear = RGB(44, 38, 48);
	pets[1].collar = 0;

	// Muning: orange tabby puspin who likes the cottage garden
	initPet(&pets[2], "Muning", PET_CAT, 0, 29, 20, 3);
	pets[2].coat = RGB(238, 166, 94);
	pets[2].coatDk = RGB(198, 114, 62);
	pets[2].patch = RGB(255, 238, 208);
	pets[2].ear = RGB(238, 166, 94);
	pets[2].collar = 0;

	// Mingming: black-and-white puspin who stays near the bakery
	initPet(&pets[3], "Mingming", PET_CAT, 1, 9, 35, 5);
	pets[3].coat = RGB(64, 58, 72);
	pets[3].coatDk = RGB(40, 36, 48);
	pets[3].patch = WHITE;
	pets[3].ear = RGB(64, 58, 72);
	pets[3].collar = 0;

	// Kiko: the baby kalabaw who grazes between the bahay kubo and the lily pond
	initPet(&pets[4], "Kiko", PET_KALABAW, 0, 9, 18, 3);
	pets[4].coat = RGB(132, 128, 142);
	pets[4].coatDk = RGB(86, 80, 98);
	pets[4].patch = RGB(190, 178, 188);  // soft muzzle and belly
	pets[4].ear = RGB(236, 160, 172);    // pink inside the ears
	pets[4].collar = 0;                  // he wears sampaguita instead
}

static void petPickTarget(Pet *p) {
	for (int tries = 0; tries < 10; tries++) {
		int tx = p->homeX + rand() % (2 * p->range + 1) - p->range;
		int ty = p->homeY + rand() % (2 * p->range + 1) - p->range;
		float fx = tx * TS + TS / 2;
		float fy = ty * TS + TS - 3;
		if (!petBlocked(fx, fy)) {
			p->tx = fx;
			p->ty = fy;
			p->state = PS_WALK;
			p->timer = 600; // give up after ten seconds if something is in the way
			return;
		}
	}
	p->state = PS_IDLE;
	p->timer = 60;
}

static void updatePets(void) {
	bool rain = rainLevel > 0.5f;
	for (int i = 0; i < PET_COUNT; i++) {
		Pet *p = &pets[i];
		if (p->happy > 0) { p->happy--; continue; } // stands still while being petted

		if (p->state == PS_SLEEP) {
			if (--p->timer <= 0) { p->state = PS_IDLE; p->timer = 60 + rand() % 120; }
			continue;
		}
		if (p->state == PS_IDLE) {
			if (--p->timer <= 0) {
				// in the rain and at night they nap more
				if (rand() % 100 < (rain ? 45 : nightLevel() > 0.6f ? 40 : 15)) {
					p->state = PS_SLEEP;
					p->timer = 300 + rand() % 600;
				} else {
					petPickTarget(p);
				}
			}
			continue;
		}

		// walking
		float dx = p->tx - p->x, dy = p->ty - p->y;
		float d = sqrtf(dx * dx + dy * dy);
		if (d < 1.5f || --p->timer <= 0) {
			p->state = PS_IDLE;
			p->timer = 60 + rand() % 180;
			continue;
		}
		float sp = (p->kind == PET_DOG ? 0.7f : p->kind == PET_CAT ? 0.55f : 0.32f) * (rain ? 0.75f : 1.0f);
		float nx = p->x + dx / d * sp;
		float ny = p->y + dy / d * sp;
		bool moved = false;
		if (!petBlocked(nx, p->y)) { p->x = nx; moved = true; }
		if (!petBlocked(p->x, ny)) { p->y = ny; moved = true; }
		if (fabsf(dx) > 0.5f) p->dir = dx > 0 ? 1 : -1;
		if (!moved) { p->state = PS_IDLE; p->timer = 40; }
	}
}

// index of the pet close enough to pet, or -1
static int nearPet(void) {
	float cx = px + PLAYER_SIZE / 2.0f, cy = py + PLAYER_SIZE / 2.0f;
	int best = -1;
	float bestD = 28.0f * 28.0f;
	for (int i = 0; i < PET_COUNT; i++) {
		float dx = pets[i].x - cx, dy = (pets[i].y - 4.0f) - cy;
		float d2 = dx * dx + dy * dy;
		if (pets[i].kind == PET_KALABAW) d2 *= 0.7f; // he is bigger than the others
		if (d2 < bestD) { bestD = d2; best = i; }
	}
	return best;
}

static void petPet(int i) {
	Pet *p = &pets[i];
	p->state = PS_IDLE; // wakes up if it was sleeping
	p->timer = 90;
	p->happy = 110;
	float dx = (px + PLAYER_SIZE / 2.0f) - p->x;
	if (dx > 1.0f) p->dir = 1;
	else if (dx < -1.0f) p->dir = -1;
}

// ---------------------------------------------------------------- walking villagers
// Each one walks a loop of waypoints (in tiles). They stop politely when you are close,
// and they only step onto the road when the pedestrian lane is clear.
#define WALKER_POINTS 8
#define WALKER_LINES 3

typedef struct {
	const char *name;
	int jayLook;                  // 0 = girl sprite, 2 or 3 = Jay sprite in those colors
	int style, acc, skin, hair, top, skirt, shoe; // girl sprite look (journal choices)
	float speed;
	int points;
	int wp[WALKER_POINTS][2];
	const char *lines[WALKER_LINES];
} WalkerInfo;

static const WalkerInfo WALKER_INFO[WALKER_COUNT] = {
	{ "BEA", 0, 1, 1, 1, 1, 3, 2, 4, 0.50f, 2, { {4, 37}, {24, 37} }, {
		"Hello! Namamasyal lang ako.",
		"Natikman mo na ba ang pandesal ni Ate Mila?",
		"Sige, mauna na ako!" } },
	{ "MANG KARDO", 3, 0, 0, 0, 0, 0, 0, 0, 0.40f, 2, { {60, 38}, {39, 38} }, {
		"Magandang araw sa'yo!",
		"Galing ako sa bukid. Ang init, 'no?",
		"Ingat ka sa daan, ha." } },
	{ "LIZA", 0, 3, 3, 2, 1, 4, 3, 1, 0.45f, 4, { {28, 30}, {35, 30}, {35, 35}, {28, 35} }, {
		"Ang ganda ng fountain, 'no?",
		"Dito ako nagpapahangin tuwing hapon.",
		"Kita-kits!" } },
	{ "TOTOY", 2, 0, 0, 0, 0, 0, 0, 0, 0.70f, 2, { {4, 52}, {60, 52} }, {
		"Uy! Nakita mo ba ang jeep na MAHAL?",
		"Pupunta ako sa pet shop mamaya.",
		"Bye! Takbo na ako!" } },
	{ "LOLA NENE", 0, 2, 0, 3, 5, 2, 4, 0, 0.28f, 2, { {58, 51}, {14, 51} }, {
		"Kumain ka na ba, apo?",
		"Masarap ang lugaw sa karenderya ni Nena.",
		"O siya, mag-ingat ka." } },
	{ "CARLA", 0, 0, 4, 2, 0, 5, 1, 2, 0.55f, 6,
	  { {37, 52}, {31, 52}, {31, 58}, {22, 58}, {31, 58}, {31, 52} }, {
		"Tumawid ka sa pedestrian lane, ha!",
		"Naghihintay ako ng jeep papuntang bayan.",
		"Sige, ingat!" } },
	// Mang Taho walks the main road and the street, calling out "Taho!" (see drawTahoVendor)
	{ "MANG TAHO", 3, 0, 0, 0, 0, 0, 0, 0, 0.40f, 8,
	  { {5, 38}, {31, 38}, {31, 52}, {58, 52}, {31, 52}, {31, 38}, {58, 38}, {31, 38} }, {
		"Taho! Taho! Mainit pa ang taho!",
		"May sago at arnibal, masarap 'yan!",
		"Bili ka na, iha! Taho!" } }
};

typedef struct {
	float x, y;       // same convention as Jay: the feet are 5 pixels below y
	float fx, fy;
	int wp;           // waypoint it is walking to
	int wait;
	bool walking;
} Walker;

static Walker walkers[WALKER_COUNT];
static int talkingWalker = -1; // villager you are chatting with, or -1

static void initWalkers(void) {
	for (int i = 0; i < WALKER_COUNT; i++) {
		Walker *w = &walkers[i];
		w->x = WALKER_INFO[i].wp[0][0] * TS + TS / 2;
		w->y = WALKER_INFO[i].wp[0][1] * TS + TS / 2;
		w->fx = 0; w->fy = 1;
		w->wp = 1;
		w->wait = 0;
		w->walking = false;
	}
	talkingWalker = -1;
}

// index of the villager close enough to chat with, or -1
static int nearWalker(void) {
	float cx = px + PLAYER_SIZE / 2.0f, cy = py + PLAYER_SIZE / 2.0f;
	int best = -1;
	float bestD = 30.0f * 30.0f;
	for (int i = 0; i < WALKER_COUNT; i++) {
		float dx = walkers[i].x - cx, dy = walkers[i].y - cy;
		float d2 = dx * dx + dy * dy;
		if (d2 < bestD) { bestD = d2; best = i; }
	}
	return best;
}

static bool onRoad(float y) { return y > ROAD_Y0 * TS - 6 && y < (ROAD_Y1 + 1) * TS + 4; }

// ---------------------------------------------------------------- Filipino street games
// All the kids' movements come from one clock, so each game stays in step. The clock
// stops while you are chatting, just like everything else.
enum { G_PRESO, G_PIKO, G_GARTER, G_PATINTERO, GAME_COUNT };
#define GAME_LINES 3

static int playClock = 0;
static int talkingGame = -1; // the game you are asking about, or -1

// ---- mga batang namamasko: during Pasko, three children sing beside the doors of the shops and houses
// (not at the school)
#define CAROL_COUNT 8
#define CAROL_SIZE  3              // children in each group
#define CAROL_GAP   15             // pixels between them
#define GAME_CAROL  GAME_COUNT     // talkingGame value while chatting with carolers
#define CAROL_SETS  3
static int talkingCarol = 0;       // the group you are chatting with
static const char *CAROL_TALK[CAROL_SETS][GAME_LINES] = {
	{ "May tambourine po kami na gawa sa tansan!",
	  "Nag-practice po kami buong hapon.",
	  "Maligayang Pasko po!" },
	{ "Lata po ng gatas ang tambol namin.",
	  "Medyo sintunado po, pero masaya kami!",
	  "Salamat po sa pakikinig!" },
	{ "Lilibutin po namin ang buong barangay!",
	  "Sa susunod na bahay naman po kami.",
	  "Merry Christmas po!" }
};

// world position of the middle child's feet: to the left of each door, so the door stays clear
static void carolSpot(int i, int *x, int *y) {
	int tx, ty;
	switch (i) {
	case 0:  tx = DOOR_TX; ty = DOOR_TY; break;             // flower shop
	case 1:  tx = BDOOR_TX; ty = BDOOR_TY; break;           // bakery
	case 2:  tx = SDOOR_TX; ty = SDOOR_TY; break;           // sari-sari store
	case 3: case 4: case 5:                                 // the three street shops
		tx = STREET_SHOP_TX[i - 3] + 3; ty = STREET_DOOR_TY; break;
	case 6:  // bahay kubo
		*x = HUT_TX * TS + 43 - 40; *y = HUT_TY * TS + 64; return;
	default: // neighbour's house
		*x = HOUSE_TX * TS + 43 - 40; *y = HOUSE_TY * TS + 64; return;
	}
	*x = tx * TS + TS / 2 - 48;
	*y = (ty + 1) * TS + 14;
}


static const char *KID_NAMES[GAME_COUNT] = { "JUNJUN", "NENENG", "TINTIN", "BUBOY" };
static const char *GAME_TALK[GAME_COUNT][GAME_LINES] = {
	{ "Tumbang preso ang laro namin!",
	  "Patumbahin ang lata gamit ang tsinelas.",
	  "Takbo agad bago ka mahuli ng taya!" },
	{ "Piko ang tawag dito!",
	  "Lumundag sa mga kahon, isang paa lang.",
	  "Bawal tapakan ang guhit, ha!" },
	{ "Chinese garter! Sali ka?",
	  "Lulundag kami habang tumataas ang garter.",
	  "Tuhod, baywang, tapos hanggang ulo!" },
	{ "Patintero ito! Ang saya!",
	  "Bantay ang taya sa mga linya.",
	  "Lusot ka nang hindi nataya!" }
};

typedef struct {
	float x, y;   // feet position in world pixels
	int lift;     // how high off the ground (jumping)
	int game;
	int look;     // colors and hair
	bool moving, armsOut;
	int dir;      // 1 = facing right, -1 = facing left, 0 = facing you
	bool gone;    // at home, indoors: not drawn and can't be talked to
} Kid;

// a value that goes 0 -> len -> 0 over and over
static int pingPong(int t, int len) {
	int m = t % (2 * len);
	return m < len ? m : 2 * len - m;
}

static int hopHeight(int t, int len, int high) { // one smooth hop lasting len frames
	return (int)(4.0f * high * t * (len - t) / (float)(len * len));
}

// --- tumbang preso: one throw every 4 seconds
#define PRESO_X (8 * TS)
#define PRESO_Y (47 * TS)
#define PRESO_CYCLE 240
#define PRESO_THROW 60    // the slipper leaves the thrower's hand
#define PRESO_FLY 36      // frames in the air
#define PRESO_UP 180      // the taya has stood the can up again

// --- piko: hopping up and down a column of boxes
#define PIKO_X (27 * TS + 8)
#define PIKO_Y (47 * TS + 4)

// --- Chinese garter
#define GARTER_X (33 * TS)
#define GARTER_Y (49 * TS + 8)

// --- patintero court
#define PAT_X (57 * TS + 8)
#define PAT_Y (46 * TS + 8)
#define PAT_W 84
#define PAT_H 68

// where kid i is playing at game time t
static Kid kidBaseAt(int i, int t) {
	Kid k = { 0, 0, 0, 0, i, false, false, 0, false };
	switch (i) {
	case 0: { // thrower
		int c = t % PRESO_CYCLE;
		k.game = G_PRESO; k.x = PRESO_X + 16; k.y = PRESO_Y + 36; k.dir = 1;
		k.armsOut = c >= PRESO_THROW - 12 && c < PRESO_THROW + 10;
		break;
	}
	case 1: // waiting for a turn, bouncing with excitement
		k.game = G_PRESO; k.x = PRESO_X + 14; k.y = PRESO_Y + 58; k.dir = 1;
		k.lift = (t / 20) % 4 == 0 ? hopHeight(t % 20, 20, 4) : 0;
		break;
	case 2: { // the taya guards the can and runs over to stand it up
		int c = t % PRESO_CYCLE;
		float home = PRESO_X + 112, can = PRESO_X + 90;
		float f = 0;
		int hit = PRESO_THROW + PRESO_FLY;
		if (c >= hit && c < PRESO_UP) f = (c - hit) / (float)(PRESO_UP - hit);
		else if (c >= PRESO_UP) f = 1.0f - (c - PRESO_UP) / (float)(PRESO_CYCLE - PRESO_UP);
		k.game = G_PRESO; k.x = home + (can - home) * f; k.y = PRESO_Y + 42;
		k.moving = c >= hit; k.dir = (c >= PRESO_UP) ? 1 : -1;
		break;
	}
	case 3: { // hopping through the piko boxes
		int step = pingPong(t / 24, 4);
		int next = pingPong(t / 24 + 1, 4);
		float f = (t % 24) / 24.0f;
		k.game = G_PIKO; k.x = PIKO_X;
		k.y = PIKO_Y + 12 + (step + (next - step) * f) * 11;
		k.lift = hopHeight(t % 24, 24, 7);
		break;
	}
	case 4: // watching the piko
		k.game = G_PIKO; k.x = PIKO_X - 24; k.y = PIKO_Y + 44; k.dir = 1;
		k.armsOut = (t / 45) % 5 == 0;
		break;
	case 5: case 6: // the two who hold the garter
		k.game = G_GARTER; k.x = GARTER_X + (i == 5 ? 7 : 57); k.y = GARTER_Y;
		k.dir = i == 5 ? 1 : -1;
		break;
	case 7: { // the jumper
		int c = t % 48;
		k.game = G_GARTER; k.x = GARTER_X + 32 + pingPong(t / 4, 12) - 6; k.y = GARTER_Y + 4;
		k.lift = c < 36 ? hopHeight(c, 36, 15) : 0;
		k.armsOut = k.lift > 5;
		break;
	}
	case 8: case 9: // patintero guards slide along their lines, arms out
		k.game = G_PATINTERO;
		k.x = PAT_X + 8 + pingPong(t * (i == 8 ? 3 : 2) / 4 + (i == 9 ? 40 : 0), PAT_W - 16);
		k.y = PAT_Y + (i == 8 ? PAT_H / 3 : PAT_H * 2 / 3) + 4;
		k.moving = true; k.armsOut = true;
		break;
	default: { // runners try to get through
		int p = pingPong(t / 2 + (i == 11 ? 55 : 0), PAT_H + 4);
		int wob = pingPong(t / 3 + i * 9, 14) - 7;
		k.game = G_PATINTERO;
		k.x = PAT_X + (i == 10 ? PAT_W / 4 : PAT_W * 3 / 4) + wob;
		k.y = PAT_Y + 6 + p;
		k.moving = true;
		break;
	}
	}
	return k;
}

// ---- going home: at sunset each kid walks down to the sidewalk and along it off the edge of the
// map (home is out of sight), and in the morning they walk back. Everything is worked out from
// dayTime alone, so nothing needs saving.
#define KID_SPEED 0.55f                  // pixels per frame
#define SIDEWALK_FEET (52 * TS + 13)     // the sidewalk row the villagers walk along

static void kidPath(int i, float *x0, float *y0, float *dirx, float *l1, float *l2) {
	Kid n = kidBaseAt(i, 0);             // a fixed spot to start from, so the path never wobbles
	*x0 = n.x; *y0 = n.y;
	*dirx = n.x < MAP_W * TS / 2 ? -1.0f : 1.0f;
	*l1 = SIDEWALK_FEET > n.y ? SIDEWALK_FEET - n.y : 0.0f;
	float edge = *dirx < 0 ? -20.0f : MAP_W * TS + 20.0f;
	*l2 = fabsf(edge - n.x);
}

// 0 = playing, between 0 and the whole path = walking, whole path = indoors
static float kidWalked(int i) {
	float x0, y0, dx, l1, l2;
	kidPath(i, &x0, &y0, &dx, &l1, &l2);
	float total = l1 + l2;
	float perDay = DAY_FRAMES * KID_SPEED;
	float s;
	if (dayTime >= 0.5f) s = (dayTime - (0.70f + (i % 6) * 0.008f)) * perDay;      // leaving at sunset
	else s = total - (dayTime - (0.25f + (i % 6) * 0.008f)) * perDay;             // coming back at sunrise
	if (s < 0.0f) s = 0.0f;
	if (s > total) s = total;
	return s;
}

static Kid kidAt(int i) {
	Kid k = kidBaseAt(i, playClock);
	float s = kidWalked(i);
	if (s <= 0.0f) return k;
	float x0, y0, dx, l1, l2;
	kidPath(i, &x0, &y0, &dx, &l1, &l2);
	if (s >= l1 + l2) { k.gone = true; return k; }
	float wx, wy;
	if (s <= l1) { wx = x0; wy = y0 + s; k.dir = 0; }
	else { wx = x0 + dx * (s - l1); wy = y0 + l1; k.dir = dx < 0 ? -1 : 1; }
	float b = s < 40.0f ? 1.0f - s / 40.0f : 0.0f; // blend from where the kid was playing
	k.x = wx + (k.x - x0) * b;
	k.y = wy + (k.y - y0) * b;
	k.lift = 0; k.armsOut = false; k.moving = true;
	return k;
}

// index of the kid close enough to chat with, or -1
static int nearKid(void) {
	float cx = px + PLAYER_SIZE / 2.0f, cy = py + PLAYER_SIZE / 2.0f;
	int best = -1;
	float bestD = 26.0f * 26.0f;
	for (int i = 0; i < KID_COUNT; i++) {
		Kid k = kidAt(i);
		if (k.gone) continue;
		float dx = k.x - cx, dy = (k.y - 5.0f) - cy;
		float d2 = dx * dx + dy * dy;
		if (d2 < bestD) { bestD = d2; best = i; }
	}
	return best;
}

// ---------------------------------------------------------------- vehicles
// Philippine traffic keeps to the right: the top lane drives west, the bottom lane east.
enum { VEH_JEEP, VEH_TRIKE };

typedef struct {
	float x;             // left edge in world pixels
	int kind, dir;       // dir: 1 = east, -1 = west
	int feet;            // bottom of the wheels in world pixels
	u32 body, trim;
} Vehicle;

static Vehicle vehicles[VEH_COUNT];

// Jeepneys are drawn 1.5x bigger (66 px of art -> 99 px on screen), so they look the right size
// next to the villagers. vehW() is the size on screen; drawVehicle() draws the 66 px art and scales it.
static int vehW(const Vehicle *v) { return v->kind == VEH_JEEP ? 99 : 34; }

static void initVehicles(void) {
	int westFeet = (ROAD_Y0 + 2) * TS - 4, eastFeet = (ROAD_Y1 + 1) * TS - 6;
	vehicles[0] = (Vehicle){ 700, VEH_JEEP,  -1, westFeet, RGB(214, 62, 66),  GOLD_UI };
	vehicles[1] = (Vehicle){ 180, VEH_TRIKE, -1, westFeet, RGB(70, 118, 196), WHITE };
	vehicles[2] = (Vehicle){ 260, VEH_JEEP,   1, eastFeet, RGB(62, 142, 104), ROSE };
	vehicles[3] = (Vehicle){ 780, VEH_TRIKE,  1, eastFeet, RGB(222, 70, 80),  GOLD_UI };
}

// does the player's box at (x, y) touch the road space taken up by this vehicle?
static bool vehicleHits(const Vehicle *v, float x, float y) {
	return x < v->x + vehW(v) && x + PLAYER_SIZE > v->x &&
	       y < v->feet && y + PLAYER_SIZE > v->feet - 14;
}

static bool vehicleBlocks(float x, float y) {
	for (int i = 0; i < VEH_COUNT; i++)
		if (vehicleHits(&vehicles[i], x, y) && !vehicleHits(&vehicles[i], px, py)) return true;
	return false;
}

// true when no vehicle is on, or about to reach, the pedestrian lane
static bool crossingClear(void) {
	const float left = CROSS_X0 * TS, right = (CROSS_X1 + 1) * TS;
	for (int i = 0; i < VEH_COUNT; i++) {
		const Vehicle *v = &vehicles[i];
		float lo = v->x - (v->dir < 0 ? 44 : 4);
		float hi = v->x + vehW(v) + (v->dir > 0 ? 44 : 4);
		if (hi > left && lo < right) return false;
	}
	return true;
}

// is someone with a feet box at (x, y) just ahead of this vehicle, in its lane?
static bool inTheWay(const Vehicle *v, float x, float y) {
	float front = v->dir > 0 ? v->x + vehW(v) : v->x;
	float ahead = (v->dir > 0) ? x - front : front - (x + PLAYER_SIZE);
	return ahead > -2 && ahead < 28 && y < v->feet + 4 && y + PLAYER_SIZE > v->feet - 18;
}

static void updateWalkers(void) {
	for (int i = 0; i < WALKER_COUNT; i++) {
		Walker *w = &walkers[i];
		const WalkerInfo *info = &WALKER_INFO[i];
		w->walking = false;
		if (w->wait > 0) { w->wait--; continue; }

		float tx = info->wp[w->wp][0] * TS + TS / 2;
		float ty = info->wp[w->wp][1] * TS + TS / 2;
		float dx = tx - w->x, dy = ty - w->y;
		float d = sqrtf(dx * dx + dy * dy);
		if (d < 1.0f) {
			w->wp = (w->wp + 1) % info->points;
			w->wait = 20 + rand() % 90; // pause for a moment
			continue;
		}
		w->fx = dx / d;
		w->fy = dy / d;
		float nx = w->x + w->fx * info->speed, ny = w->y + w->fy * info->speed;

		// wait if you are standing in the way
		float pdx = px + PLAYER_SIZE / 2.0f - nx, pdy = py + PLAYER_SIZE / 2.0f - ny;
		if (!insideHouse && pdx * pdx + pdy * pdy < 18.0f * 18.0f && pdx * w->fx + pdy * w->fy > 0)
			continue;
		// look both ways before crossing
		if (!onRoad(w->y) && onRoad(ny) && !crossingClear()) continue;

		// never walk into the side of a vehicle
		bool bump = false;
		for (int j = 0; j < VEH_COUNT; j++)
			if (vehicleHits(&vehicles[j], nx - PLAYER_SIZE / 2, ny - PLAYER_SIZE / 2) &&
			    !vehicleHits(&vehicles[j], w->x - PLAYER_SIZE / 2, w->y - PLAYER_SIZE / 2)) bump = true;
		if (bump) continue;

		w->x = nx;
		w->y = ny;
		w->walking = true;
	}
}

static void updateVehicles(void) {
	const float span = MAP_W * TS + 160.0f;
	for (int i = 0; i < VEH_COUNT; i++) {
		Vehicle *v = &vehicles[i];
		float w = vehW(v);
		float front = v->dir > 0 ? v->x + w : v->x;
		bool stop = false;
		// drivers stop for you if you are just ahead in their lane
		if (inTheWay(v, px, py)) stop = true;
		// ... and for villagers crossing the road
		for (int j = 0; j < WALKER_COUNT; j++)
			if (onRoad(walkers[j].y) &&
			    inTheWay(v, walkers[j].x - PLAYER_SIZE / 2, walkers[j].y - PLAYER_SIZE / 2)) stop = true;
		// and they keep their distance from each other
		for (int j = 0; j < VEH_COUNT; j++) {
			const Vehicle *o = &vehicles[j];
			if (j == i || o->dir != v->dir) continue;
			float gap = (v->dir > 0) ? o->x - front : front - (o->x + vehW(o));
			if (gap < -(w + vehW(o))) gap += span; // it is ahead, past the wrap-around
			if (gap >= 0 && gap < 24) stop = true;
		}
		if (stop) continue;
		v->x += v->dir * (v->kind == VEH_JEEP ? 1.1f : 0.8f);
		if (v->x > MAP_W * TS + 80) v->x -= span;
		if (v->x < -80) v->x += span;
	}
}

static void resetPlayer(void) {
	px = 31 * TS + 3;
	py = 35 * TS + 3;
}

static void initGame(void) {
	// Customization palettes (runtime initialization is required by C2D_Color32).
	skinTones[0] = RGB(255, 226, 204);
	skinTones[1] = RGB(244, 208, 180);
	skinTones[2] = RGB(235, 183, 162);
	skinTones[3] = RGB(196, 140, 108);
	skinTones[4] = RGB(140, 92, 66);

	hairColors[0] = RGB(102, 77, 70);
	hairColors[1] = RGB(54, 40, 46);
	hairColors[2] = RGB(176, 126, 78);
	hairColors[3] = RGB(196, 98, 112);

	outfitColors[0] = RGB(251, 245, 222);
	outfitColors[1] = RGB(176, 206, 236);
	outfitColors[2] = RGB(214, 188, 232);
	outfitColors[3] = RGB(248, 184, 206);
	hairColors[4] = RGB(214, 170, 104);
	hairColors[5] = RGB(118, 100, 150);
	outfitColors[4] = RGB(170, 214, 184);
	outfitColors[5] = RGB(236, 128, 128);

	skirtColors[0] = RGB(170, 130, 108);
	skirtColors[1] = RGB(96, 112, 164);
	skirtColors[2] = RGB(196, 96, 112);
	skirtColors[3] = RGB(104, 142, 108);
	skirtColors[4] = RGB(76, 68, 84);

	shoeColors[0] = RGB(40, 30, 30);
	shoeColors[1] = RGB(132, 82, 54);
	shoeColors[2] = RGB(196, 70, 90);
	shoeColors[3] = RGB(74, 94, 150);
	shoeColors[4] = RGB(230, 150, 180);

	flowerCols[0] = RGB(164, 186, 252); // soft periwinkle hydrangea
	flowerCols[1] = RGB(252, 218, 102); // warm buttercup
	flowerCols[2] = RGB(254, 154, 188); // soft blossom
	flowerCols[3] = RGB(204, 174, 252); // pastel lilac
	flowerCols[4] = RGB(255, 250, 240); // cream daisy

	genMap();
	resetPlayer();
	pfx = 0;
	pfy = 1;

	jay.x = WP[3][0] * TS + TS / 2;
	jay.y = WP[3][1] * TS + TS / 2;
	jay.fx = 0; jay.fy = -1;
	jay.wp = 0;
	jay.wait = 60;
	dialogActive = false;
	dialogLine = 0;
	insideHouse = false;
	talkingToOwner = false;
	shopId = 0;
	shopMenu = false;
	tahoMenu = false;
	shopSel = 0;
	shopMsg = NULL;
	coins = 100;
	for (int i = 0; i < HOTBAR_SLOTS; i++) { invType[i] = -1; invCount[i] = 0; }
	hotbarSel = 0;
	hbNameTimer = 0;
	hbFade = 255;
	fullness = 60;
	hungerTick = 0;
	eatTimer = 0;
	eatItem = -1;
	eatMsg = NULL;
	eatMsgTimer = 0;
	initPets();
	initVehicles();
	initWalkers();
	playClock = 0;
	talkingGame = -1;
	talkingSchool = -1;
	setWeather(W_SUNNY);
	rainLevel = 0.0f;
	dayTime = DAY_START;
	eventMode = EVM_AUTO;
	for (int i = 0; i < 2; i++) { giftMask[i] = 0; giftYear[i] = 0; }
	refreshEvent();
}

// ---------------------------------------------------------------- inside the cottage
#define ROOM_X 56
#define ROOM_Y 12
#define ROOM_W 288
#define ROOM_H 216
#define FLOOR_Y (ROOM_Y + 64) // where the back wall meets the floor

static float inX, inY; // player position in screen coordinates while indoors

// solid furniture: { x, y, w, h } of the area on the floor that blocks the player
enum { FUR_COUNTER, FUR_TABLE, FUR_STAND_L, FUR_STAND_R, FUR_COUNT };
static const int FURN[SHOP_COUNT][FUR_COUNT][4] = {
	{ // flower shop
		{ 72, 106, 80, 18 },
		{ 262, 128, 44, 20 },
		{ 62, 178, 34, 18 },
		{ 304, 178, 34, 18 }
	}, { // bakery
		{ 72, 106, 128, 18 },
		{ 246, 118, 44, 22 },
		{ 70, 176, 64, 22 },
		{ 232, 176, 64, 22 }
	}, { // sari-sari store
		{ 88, 106, 96, 18 },
		{ 262, 128, 44, 20 },
		{ 62, 178, 34, 18 },
		{ 304, 178, 34, 18 }
	}, { // ukay-ukay: counter, clothes rack, two bins of clothes
		{ 80, 106, 96, 18 },
		{ 252, 128, 70, 20 },
		{ 62, 178, 58, 18 },
		{ 280, 178, 58, 18 }
	}, { // pet shop: counter, puppy pen, two shelves
		{ 80, 106, 96, 18 },
		{ 256, 126, 64, 24 },
		{ 62, 178, 58, 18 },
		{ 280, 178, 58, 18 }
	}, { // karenderya: counter with pots, three dining tables
		{ 72, 106, 128, 18 },
		{ 252, 126, 62, 22 },
		{ 70, 176, 64, 20 },
		{ 266, 176, 64, 20 }
	}, { // school: teacher's desk (Miss Luz stands behind it), then three student desks back to front
		{ 152, 100, 96, 16 },
		{ 262, 122, 38, 34 },
		{ 80, 130, 38, 34 },
		{ 232, 176, 38, 34 }
	}
};

// true when the player is standing right in front of the outside door
static bool atDoorOf(int dtx, int dty) {
	float cx = px + PLAYER_SIZE / 2.0f;
	float cy = py + PLAYER_SIZE / 2.0f;
	return fabsf(cx - (dtx * TS + TS / 2)) < 13.0f &&
	       cy >= (dty + 1) * TS && cy < (dty + 1) * TS + 14;
}

static bool atDoor(void) { return atDoorOf(DOOR_TX, DOOR_TY); }
static bool atBakeryDoor(void) { return atDoorOf(BDOOR_TX, BDOOR_TY); }
static bool atSariDoor(void) { return atDoorOf(SDOOR_TX, SDOOR_TY); }
static bool atSchoolDoor(void) { return atDoorOf(SCH_DOOR_TX, SCH_DOOR_TY); }

// shop id of the street shop whose door you are standing at, or 0
static int atStreetDoor(void) {
	for (int i = 0; i < 3; i++)
		if (atDoorOf(STREET_SHOP_TX[i] + 3, STREET_DOOR_TY)) return 3 + i;
	return 0;
}

// true when the player is standing on the doormat indoors
static bool atExit(void) {
	float cx = inX + PLAYER_SIZE / 2.0f;
	return fabsf(cx - SCR_W / 2) < 24.0f && inY + PLAYER_SIZE > ROOM_Y + ROOM_H - 22;
}

// The five people inside the school. x = middle of the character, base = screen y of the desk
// (or feet) that the player stands in front of, top = screen y of the top of the head.
typedef struct {
	const char *name;
	int x, base, top;
} SchoolNpc;

static const SchoolNpc SCHOOL_NPCS[SCHOOL_NPC_COUNT] = {
	{ "MISS LUZ",  200, 116, 54 },    // teacher behind her desk
	{ "SIR DANTE", 110, 110, 70 },    // teacher by the bookshelf
	{ "ANGEL",      99, 164, 133 },   // students at their desks
	{ "MIKO",      281, 156, 125 },
	{ "RENZ",      251, 210, 179 }
};

// index of the school character the player is standing in front of, or -1
static int nearSchoolNpc(void) {
	float cx = inX + PLAYER_SIZE / 2.0f;
	int best = -1;
	float bestDx = 999.0f;
	for (int i = 0; i < SCHOOL_NPC_COUNT; i++) {
		float dx = fabsf(cx - SCHOOL_NPCS[i].x);
		int b = SCHOOL_NPCS[i].base;
		if (dx < 30.0f && inY >= b - 12 && inY < b + 16 && dx < bestDx) { bestDx = dx; best = i; }
	}
	return best;
}

// where the shopkeeper of the current shop stands (screen x)
static int npcX(void) {
	static const int X[SHOP_COUNT] = { 104 /* OWNER_X */, BAKER_X, SARI_X, 128, 128, 132, 0 };
	return X[shopId];
}

static int baseDialogCount(void) {
	if (insideHouse && shopId == SCHOOL_ID) return SCHOOL_LINES;
	if (!talkingToOwner && talkingGame >= 0) return GAME_LINES;
	if (!talkingToOwner) return talkingWalker >= 0 ? WALKER_LINES : JAY_LINES;
	if (shopId >= 3) return STREET_LINES;
	return shopId == 2 ? SARI_LINES : shopId == 1 ? BAKER_LINES : OWNER_LINES;
}

static int dialogCount(void) { return baseDialogCount() + eventLines(); }

// the group of carolers close enough to chat with, or -1
static int nearCaroler(void) {
	if (activeEvent != EV_PASKO) return -1;
	float cx = px + PLAYER_SIZE / 2.0f, cy = py + PLAYER_SIZE / 2.0f;
	for (int i = 0; i < CAROL_COUNT; i++) {
		int gx, gy;
		carolSpot(i, &gx, &gy);
		// the group is wide, so measure sideways from its nearest child
		float dx = fmaxf(fabsf(cx - gx) - CAROL_GAP, 0.0f), dy = cy - gy;
		if (dx * dx + dy * dy < 24.0f * 24.0f) return i;
	}
	return -1;
}

// which grown-up you are chatting with (see GIVER_*), or -1 for the children
static int currentGiver(void) {
	if (insideHouse) {
		if (talkingToOwner) return GIVER_SHOP0 + shopId;
		if (shopId == SCHOOL_ID && talkingSchool >= 0 && talkingSchool < 2) return GIVER_TEACH0 + talkingSchool;
		return -1;
	}
	if (talkingGame >= 0) return -1;
	return talkingWalker >= 0 ? GIVER_WALKER0 + talkingWalker : 0;
}

static const char *eventGreeting(void) {
	int g = currentGiver();
	bool hw = activeEvent == EV_HALLOWEEN;
	if (talkingGame == GAME_CAROL && !insideHouse) return "Namamasko po! Kakantahan po namin kayo!";
	if (g < 0) return hw ? "Happy Halloween! Ang saya ngayon, 'no?"
	                     : "Merry Christmas! Ang saya ng Pasko, 'no?";
	if (giftMask[activeEvent - 1] & (1u << g))
		return hw ? "Happy Halloween! Nabigyan na kita ng candy, ha."
		          : "Maligayang Pasko! Ingat ka palagi.";
	return hw ? "Trick or treat ba? Heto, may candy ka!"
	          : "Namamasko ka ba? Heto ang aguinaldo mo!";
}

// true when the player is standing in front of the counter, near the shopkeeper
static bool nearOwner(void) {
	if (shopId == SCHOOL_ID) return false; // the school has no counter (see nearSchoolNpc)
	float cx = inX + PLAYER_SIZE / 2.0f;
	return fabsf(cx - npcX()) < 34.0f &&
	       inY >= FURN[shopId][FUR_COUNTER][1] && inY < FURN[shopId][FUR_COUNTER][1] + FURN[shopId][FUR_COUNTER][3] + 14;
}

static void enterHouse(int id) {
	shopId = id;
	talkingSchool = -1;
	shopMenu = false;
	insideHouse = true;
	inX = SCR_W / 2 - PLAYER_SIZE / 2;
	inY = ROOM_Y + ROOM_H - PLAYER_SIZE - 6;
	pfx = 0; pfy = -1;
	playerMoving = false;
}

// where the player stands just outside the door of the shop they are in
static void shopExitPos(float *ox, float *oy) {
	int dtx = DOOR_TX, dty = DOOR_TY;
	if (shopId == SCHOOL_ID) { dtx = SCH_DOOR_TX; dty = SCH_DOOR_TY; }
	else if (shopId == 1) { dtx = BDOOR_TX; dty = BDOOR_TY; }
	else if (shopId == 2) { dtx = SDOOR_TX; dty = SDOOR_TY; }
	else if (shopId >= 3) { dtx = STREET_SHOP_TX[shopId - 3] + 3; dty = STREET_DOOR_TY; }
	*ox = dtx * TS + (TS - PLAYER_SIZE) / 2;
	*oy = (dty + 1) * TS + 1;
}

static void leaveHouse(void) {
	insideHouse = false;
	shopExitPos(&px, &py);
	pfx = 0; pfy = 1;
	playerMoving = false;
}

static bool roomBlocked(float x, float y) {
	if (x < ROOM_X + 8 || x + PLAYER_SIZE > ROOM_X + ROOM_W - 8) return true;
	if (y < FLOOR_Y || y + PLAYER_SIZE > ROOM_Y + ROOM_H - 4) return true;
	// the space behind the counter belongs to Aling Rosa
	const int *ct = FURN[shopId][FUR_COUNTER];
	if (x < ct[0] + ct[2] && x + PLAYER_SIZE > ct[0] && y < ct[1]) return true;
	for (int i = 0; i < FUR_COUNT; i++) {
		const int *f = FURN[shopId][i];
		if (x < f[0] + f[2] && x + PLAYER_SIZE > f[0] &&
		    y < f[1] + f[3] && y + PLAYER_SIZE > f[1]) return true;
	}
	// Sir Dante stands by the bookshelf: nobody can walk through him
	if (shopId == SCHOOL_ID && x < 124 && x + PLAYER_SIZE > 96 && y < 110) return true;
	return false;
}

static bool blockedAt(float x, float y) {
	return solid(tileAt(x, y)) || solid(tileAt(x + PLAYER_SIZE - 1, y)) ||
	       solid(tileAt(x, y + PLAYER_SIZE - 1)) ||
	       solid(tileAt(x + PLAYER_SIZE - 1, y + PLAYER_SIZE - 1));
}

static void updateJay(void) {
	if (jay.wait > 0) { jay.wait--; return; }

	float tx = WP[jay.wp][0] * TS + TS / 2;
	float ty = WP[jay.wp][1] * TS + TS / 2;
	float dx = tx - jay.x, dy = ty - jay.y;
	float d = sqrtf(dx * dx + dy * dy);
	if (d < 1.5f) {
		jay.wp = (jay.wp + 1) % 4;
		jay.wait = 60;
		return;
	}
	jay.fx = dx / d;
	jay.fy = dy / d;
	jay.x += jay.fx * 0.55f;
	jay.y += jay.fy * 0.55f;
}

// true when the player is close enough to chat with Jay
static bool nearJay(void) {
	float dx = jay.x - (px + PLAYER_SIZE / 2.0f);
	float dy = jay.y - (py + PLAYER_SIZE / 2.0f);
	return dx * dx + dy * dy < 30.0f * 30.0f;
}

// ---------------------------------------------------------------- My Journal (customization)
static const char *STYLE_NAMES[] = { "Long wavy", "Short bob", "Twin buns", "Braids" };
static const char *ACC_NAMES[] = { "None", "Ribbon", "Sampaguita", "Headband", "Salakot" };
static const char *SKIN_NAMES[] = { "Porcelain", "Light", "Peach", "Tan", "Deep" };
static const char *HAIR_NAMES[] = { "Brown", "Black", "Caramel", "Rose", "Honey", "Violet" };
static const char *TOP_NAMES[] = { "Cream", "Sky", "Lilac", "Pink", "Mint", "Coral" };
static const char *SKIRT_NAMES[] = { "Cocoa", "Navy", "Berry", "Leaf", "Charcoal" };
static const char *SHOE_NAMES[] = { "Black", "Brown", "Red", "Blue", "Pink" };
static const char *PRONOUN_NAMES[] = { "She / Her", "He / Him", "They / Them" };
static const char *MONTH_NAMES[] = { "January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December" };
static const int MONTH_DAYS[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static const char *STATUS_NAMES[] = { "Single", "In love", "Taken", "Secret" };

typedef struct {
	const char *label;  // pixel font: letters only
	int *value;         // NULL for rows you type into
	int count;
	const char **names; // NULL for the birthday number
	const u32 *colors;  // color swatch, or NULL
	char *text;         // typed field
} JournalRow;

#define JOURNAL_TABS 3
#define JOURNAL_MAX_ROWS 7
static const char *TAB_NAMES[JOURNAL_TABS] = { "LOOK", "OUTFIT", "ABOUT ME" };
static const int TAB_ROWS[JOURNAL_TABS] = { 4, 3, 7 };
static const JournalRow JOURNAL[JOURNAL_TABS][JOURNAL_MAX_ROWS] = {
	{
		{ "HAIRSTYLE", &styleIndex, 4, STYLE_NAMES, NULL, NULL },
		{ "HAIR", &hairIndex, 6, HAIR_NAMES, hairColors, NULL },
		{ "SKIN", &skinIndex, 5, SKIN_NAMES, skinTones, NULL },
		{ "ACCESSORY", &accIndex, 5, ACC_NAMES, NULL, NULL }
	}, {
		{ "TOP", &outfitIndex, 6, TOP_NAMES, outfitColors, NULL },
		{ "SKIRT", &skirtIndex, 5, SKIRT_NAMES, skirtColors, NULL },
		{ "SHOES", &shoeIndex, 5, SHOE_NAMES, shoeColors, NULL }
	}, {
		{ "NAME", NULL, 0, NULL, NULL, playerName },
		{ "PRONOUNS", &pronounIndex, 3, PRONOUN_NAMES, NULL, NULL },
		{ "MONTH", &monthIndex, 12, MONTH_NAMES, NULL, NULL },
		{ "DAY", &dayIndex, 31, NULL, NULL, NULL },
		{ "YEAR", &yearIndex, LAST_YEAR - FIRST_YEAR + 1, NULL, NULL, NULL },
		{ "HOME", NULL, 0, NULL, NULL, homeName },
		{ "STATUS", &statusIndex, 4, STATUS_NAMES, NULL, NULL }
	}
};

// bottom-screen layout
#define TAB_Y 4
#define TAB_W 104
#define TAB_H 26
#define ROW_Y 36
#define ROW_STEP 24
#define ROW_H 22
#define DONE_Y 210

static void openJournal(void) {
	showCustomize = true;
	showHowTo = false;
	playerMoving = false;
	journalTab = 0;
	journalRow = 0;
}

// opens the 3DS keyboard; the old text is kept if the player cancels
static void typeField(char *dest, const char *hint) {
	SwkbdState kb;
	char out[NAME_BYTES] = "";
	swkbdInit(&kb, SWKBD_TYPE_NORMAL, 2, NAME_CHARS);
	swkbdSetHintText(&kb, hint);
	swkbdSetInitialText(&kb, dest);
	swkbdSetValidation(&kb, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
	if (swkbdInputText(&kb, out, sizeof(out)) == SWKBD_BUTTON_CONFIRM)
		snprintf(dest, NAME_BYTES, "%s", out);
}

static int daysInMonth(void) {
	int year = FIRST_YEAR + yearIndex;
	bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
	if (monthIndex == 1 && !leap) return 28;
	return MONTH_DAYS[monthIndex];
}

static void changeRow(const JournalRow *r, int dir) {
	if (r->text) {
		typeField(r->text, r->text == playerName ? "Your name" : "Name of your home");
		return;
	}
	int count = r->count;
	if (r->value == &dayIndex) count = daysInMonth();
	*r->value = (*r->value + count + dir) % count;
	if (dayIndex >= daysInMonth()) dayIndex = daysInMonth() - 1;
}

static void updateJournal(u32 kDown) {
	if (kDown & (KEY_B | KEY_SELECT)) { showCustomize = false; return; }
	if (kDown & (KEY_L | KEY_R)) {
		journalTab = (journalTab + ((kDown & KEY_R) ? 1 : JOURNAL_TABS - 1)) % JOURNAL_TABS;
		journalRow = 0;
	}
	int rows = TAB_ROWS[journalTab];
	if (kDown & KEY_DUP) journalRow = (journalRow + rows - 1) % rows;
	if (kDown & KEY_DDOWN) journalRow = (journalRow + 1) % rows;
	const JournalRow *r = &JOURNAL[journalTab][journalRow];
	if (kDown & KEY_A) changeRow(r, 1);
	else if ((kDown & (KEY_DLEFT | KEY_DRIGHT)) && !r->text)
		changeRow(r, (kDown & KEY_DRIGHT) ? 1 : -1);

	if (kDown & KEY_TOUCH) {
		touchPosition tp;
		hidTouchRead(&tp);
		if (tp.py < TAB_Y + TAB_H + 2) {
			int t = tp.px / (TAB_W + 2);
			if (t < JOURNAL_TABS) { journalTab = t; journalRow = 0; }
		} else if (tp.py >= DONE_Y) {
			if (tp.px >= 100 && tp.px <= 220) showCustomize = false;
		} else if (tp.py >= ROW_Y) {
			int i = (tp.py - ROW_Y) / ROW_STEP;
			if (i >= TAB_ROWS[journalTab]) return;
			journalRow = i;
			// left arrow goes back; the rest of the row goes forward
			changeRow(&JOURNAL[journalTab][i], (tp.px >= 104 && tp.px < 144) ? -1 : 1);
		}
	}
}

// ---------------------------------------------------------------- saving and loading
// One small binary file on the SD card. It keeps your look, name, coins, bag, weather and
// where you are standing. (If you are inside a shop, you wake up outside its door.)
#define SAVE_PATH  "sdmc:/3ds/mahal_kita.sav"
#define SAVE_MAGIC 0x564B414Du   // "MAKV"
#define SAVE_VERSION 3

typedef struct {
	u32 magic, version;
	s32 coins, hotbarSel;
	s32 invType[HOTBAR_SLOTS], invCount[HOTBAR_SLOTS];
	s32 skin, hair, style, acc, outfit, skirt, shoe;
	s32 pronoun, month, day, year, status;
	s32 weather;
	float x, y, fx, fy;
	char name[NAME_BYTES], home[NAME_BYTES];
	float dayTime;   // added in save version 2 (old saves just start in the morning)
	s32 eventMode;   // the next three were added in save version 3 (seasonal events)
	u32 giftMask[2];
	s32 giftYear[2];
} SaveData;

static const char *saveMsg = NULL;  // shown on the SAVE GAME button for a moment
static int saveMsgTimer = 0;

static bool saveGame(void) {
	SaveData d;
	memset(&d, 0, sizeof(d));
	d.magic = SAVE_MAGIC;
	d.version = SAVE_VERSION;
	d.coins = coins;
	d.hotbarSel = hotbarSel;
	for (int i = 0; i < HOTBAR_SLOTS; i++) { d.invType[i] = invType[i]; d.invCount[i] = invCount[i]; }
	d.skin = skinIndex; d.hair = hairIndex; d.style = styleIndex; d.acc = accIndex;
	d.outfit = outfitIndex; d.skirt = skirtIndex; d.shoe = shoeIndex;
	d.pronoun = pronounIndex; d.month = monthIndex; d.day = dayIndex;
	d.year = yearIndex; d.status = statusIndex;
	d.weather = weather;
	d.dayTime = dayTime;
	d.eventMode = eventMode;
	for (int i = 0; i < 2; i++) { d.giftMask[i] = giftMask[i]; d.giftYear[i] = giftYear[i]; }
	float x = px, y = py;
	if (insideHouse) shopExitPos(&x, &y); // inside a shop: save the spot outside its door
	d.x = x; d.y = y;
	d.fx = insideHouse ? 0 : pfx;
	d.fy = insideHouse ? 1 : pfy;
	strncpy(d.name, playerName, NAME_BYTES - 1);
	strncpy(d.home, homeName, NAME_BYTES - 1);

	mkdir("sdmc:/3ds", 0777); // already there on most SD cards; harmless if it is
	FILE *f = fopen(SAVE_PATH, "wb");
	if (!f) return false;
	bool ok = fwrite(&d, sizeof(d), 1, f) == 1;
	if (fclose(f) != 0) ok = false;
	return ok;
}

static int clampInt(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

// Returns true if a save was found and loaded. Call after initGame().
static bool loadGame(void) {
	FILE *f = fopen(SAVE_PATH, "rb");
	if (!f) return false;
	SaveData d;
	memset(&d, 0, sizeof(d));
	size_t got = fread(&d, 1, sizeof(d), f); // version 1 saves are shorter (no dayTime)
	fclose(f);
	bool ok = got >= offsetof(SaveData, dayTime);
	if (!ok || d.magic != SAVE_MAGIC || d.version < 1 || d.version > SAVE_VERSION) return false;

	coins = clampInt(d.coins, 0, 999999);
	hotbarSel = clampInt(d.hotbarSel, 0, HOTBAR_SLOTS - 1);
	for (int i = 0; i < HOTBAR_SLOTS; i++) {
		invType[i] = clampInt(d.invType[i], -1, ITEM_COUNT - 1);
		invCount[i] = invType[i] < 0 ? 0 : clampInt(d.invCount[i], 1, STACK_MAX);
	}
	skinIndex = clampInt(d.skin, 0, 4);
	hairIndex = clampInt(d.hair, 0, 5);
	styleIndex = clampInt(d.style, 0, 3);
	accIndex = clampInt(d.acc, 0, 4);
	outfitIndex = clampInt(d.outfit, 0, 5);
	skirtIndex = clampInt(d.skirt, 0, 4);
	shoeIndex = clampInt(d.shoe, 0, 4);
	pronounIndex = clampInt(d.pronoun, 0, 2);
	monthIndex = clampInt(d.month, 0, 11);
	yearIndex = clampInt(d.year, 0, LAST_YEAR - FIRST_YEAR);
	dayIndex = clampInt(d.day, 0, daysInMonth() - 1);
	statusIndex = clampInt(d.status, 0, 3);
	d.name[NAME_BYTES - 1] = 0;
	d.home[NAME_BYTES - 1] = 0;
	strcpy(playerName, d.name);
	strcpy(homeName, d.home);

	px = fminf(fmaxf(d.x, 0.0f), MAP_W * TS - PLAYER_SIZE);
	py = fminf(fmaxf(d.y, 0.0f), MAP_H * TS - PLAYER_SIZE);
	if (blockedAt(px, py)) resetPlayer(); // safety net (also if any part of you is inside a wall)
	pfx = d.fx; pfy = d.fy;
	insideHouse = false;
	playerMoving = false;
	setWeather(d.weather == W_RAINY ? W_RAINY : W_SUNNY);
	rainLevel = (weather == W_RAINY) ? 1.0f : 0.0f;
	dayTime = (d.version >= 2 && d.dayTime >= 0.0f && d.dayTime < 1.0f) ? d.dayTime : DAY_START;
	// old saves are shorter, so these stay zero: AUTO mode and no gifts yet
	eventMode = clampInt(d.eventMode, 0, EVM_COUNT - 1);
	for (int i = 0; i < 2; i++) { giftMask[i] = d.giftMask[i]; giftYear[i] = d.giftYear[i]; }
	refreshEvent();
	return true;
}

static void activateMenuBtn(int idx) {
	if (idx == BTN_PLAY) {
		showHowTo = false;
		hasStartedOnce = true;
		gameState = STATE_PLAYING;
	} else if (idx == BTN_CUSTOMIZE) {
		openJournal();
	} else if (idx == BTN_SAVE) {
		saveMsg = saveGame() ? "SAVED!" : "SAVE FAILED";
		saveMsgTimer = 120;
	} else if (idx == BTN_HOWTO) {
		showHowTo = !showHowTo;
	} else if (idx == BTN_RESTART) {
		showHowTo = false;
		initGame();
		hasStartedOnce = true;
		gameState = STATE_PLAYING;
	}
}

static void updateMenu(u32 kDown) {
	if (showHowTo) {
		if (kDown & (KEY_A | KEY_B | KEY_SELECT)) showHowTo = false;
		if (kDown & KEY_TOUCH) showHowTo = false;
		return;
	}

	if (kDown & KEY_DUP) selectedBtn = (selectedBtn + BTN_COUNT - 1) % BTN_COUNT;
	if (kDown & KEY_DDOWN) selectedBtn = (selectedBtn + 1) % BTN_COUNT;
	if (kDown & KEY_A) {
		activateMenuBtn(selectedBtn);
		return;
	}

	if (kDown & KEY_TOUCH) {
		touchPosition tp;
		hidTouchRead(&tp);
		for (int i = 0; i < BTN_COUNT; i++) {
			const MenuBtn *b = &MENU_BTNS[i];
			if (tp.px >= b->x && tp.px <= b->x + b->w &&
			    tp.py >= b->y && tp.py <= b->y + b->h) {
				selectedBtn = i;
				activateMenuBtn(i);
				return;
			}
		}
	}
}

// Called when a chat is finished: hands over the treat. Returns the message, or NULL.
static const char *claimGift(void) {
	if (activeEvent == EV_NONE) return NULL;
	int g = currentGiver();
	if (g < 0 || (giftMask[activeEvent - 1] & (1u << g))) return NULL;
	giftMask[activeEvent - 1] |= 1u << g;
	const char *msg;
	if (activeEvent == EV_HALLOWEEN) {
		coins += TREAT_COINS;
		msg = addItem(TREAT_CANDY) ? "Candy + P5! Yehey!" : "P5! (Puno ang bag)";
	} else {
		coins += AGUINALDO_COINS;
		msg = "Aguinaldo! + P20";
	}
	if (giftsGot() == GIVER_COUNT) {
		coins += ALL_GIFTS_BONUS;
		msg = "Kumpleto na! BONUS + P100!";
	}
	eatSay(msg);
	return msg;
}

// Jay, the pets and the weather keep going whenever the game is not paused.
static void updateWorld(void) {
	updateJay();
	updatePets();
	updateVehicles();
	updateWalkers();
	playClock++;
	updateWeather();
	updateDayTime();
	if (++hungerTick >= 420) { // about every 7 seconds you get a little hungrier
		hungerTick = 0;
		if (fullness > 0) fullness--;
	}
	if (fullness < 20 && eatTimer <= 0 && eatMsgTimer <= 0 && playClock % 900 == 450)
		eatSay("*kruuu* Gutom na ako...");
}

static void updateGame(u32 kDown, u32 kHeld) {
	frameCount++;
	if (hbNameTimer > 0) hbNameTimer--;
	if (saveMsgTimer > 0) saveMsgTimer--;
	if (eatMsgTimer > 0) eatMsgTimer--;
	if (!dialogActive && frameCount % 60 == 0) refreshEvent();

	if (showCustomize) { // the game is paused while the journal is open
		updateJournal(kDown);
		return;
	}

	if (gameState == STATE_MENU) {
		updateMenu(kDown);
		return;
	}

	// SELECT or the touch MENU button pauses to the main menu.
	bool toMenu = (kDown & KEY_SELECT) != 0;
	if (kDown & KEY_TOUCH) {
		touchPosition tp;
		hidTouchRead(&tp);
		if (tp.px >= PLAY_MENU_X && tp.px <= PLAY_MENU_X + PLAY_MENU_W &&
		    tp.py >= PLAY_MENU_Y && tp.py <= PLAY_MENU_Y + PLAY_MENU_H) toMenu = true;
		else if (tp.px >= WX_X && tp.px <= WX_X + WX_W &&
		         tp.py >= WX_Y && tp.py <= WX_Y + WX_H)
			setWeather(weather == W_SUNNY ? W_RAINY : W_SUNNY);
		else if (tp.px >= CLK_X && tp.px <= CLK_X + CLK_W &&
		         tp.py >= CLK_Y && tp.py <= CLK_Y + CLK_H)
			skipToNextPhase();
		else if (!dialogActive && !shopMenu && !tahoMenu &&
		         tp.px >= EVB_X && tp.px <= EVB_X + EVB_W &&
		         tp.py >= EVB_Y && tp.py <= EVB_Y + EVB_H) {
			eventMode = (eventMode + 1) % EVM_COUNT;
			refreshEvent();
		}
	}
	if (toMenu) {
		showHowTo = false;
		showCustomize = false;
		dialogActive = false;
		dialogLine = 0;
		shopMenu = false;
		tahoMenu = false;
		gameState = STATE_MENU;
		return;
	}

	playerMoving = false;

	// Bakery buy menu: the game is paused while it is open.
	if (shopMenu) {
		if (kDown & KEY_B) { shopMenu = false; return; }
		if (kDown & KEY_DUP) { shopSel = (shopSel + BAKE_COUNT - 1) % BAKE_COUNT; shopMsg = NULL; }
		if (kDown & KEY_DDOWN) { shopSel = (shopSel + 1) % BAKE_COUNT; shopMsg = NULL; }
		if (kDown & KEY_A) {
			int item = shopBase() + shopSel;
			if (isClothes(item) && *wearSlot(item) == wearValue(item)) {
				shopMsg = "Suot mo na 'yan!";
			} else if (coins >= ITEM_PRICE[item]) {
				if (isClothes(item)) {
					coins -= ITEM_PRICE[item];
					*wearSlot(item) = wearValue(item);
					shopMsg = "Bagay na bagay sa'yo!";
				} else if (addItem(item)) {
					coins -= ITEM_PRICE[item];
					shopMsg = "Salamat po! Enjoy!";
				} else {
					shopMsg = "Puno na ang bag mo!";
				}
			} else {
				shopMsg = "Kulang ang pera mo!";
			}
		}
		return;
	}

	// Mang Taho's buy menu: the game is paused while it is open.
	if (tahoMenu) {
		if (kDown & KEY_B) { tahoMenu = false; talkingWalker = -1; return; }
		if (kDown & KEY_A) {
			if (coins < ITEM_PRICE[ITEM_TAHO]) tahoMsg = "Kulang ang pera mo!";
			else if (addItem(ITEM_TAHO)) {
				coins -= ITEM_PRICE[ITEM_TAHO];
				tahoMsg = "Salamat! Mainit pa 'yan!";
			} else tahoMsg = "Puno na ang bag mo!";
		}
		return;
	}

	// Player movement and Jay's stroll pause during dialogue.
	if (dialogActive) {
		if (kDown & KEY_A) {
			dialogLine++;
			if (dialogLine >= dialogCount()) {
				bool toShop = talkingToOwner && shopId >= 1;
				bool toTaho = !talkingToOwner && talkingGame < 0 && talkingWalker == TAHO_WALKER;
				const char *gift = claimGift(); // also shown inside a buy menu
				dialogActive = false;
				dialogLine = 0;
				if (toShop) { shopMenu = true; shopSel = 0; shopMsg = gift; }
				if (toTaho) { tahoMenu = true; tahoMsg = gift; }
			}
		} else if (kDown & KEY_B) {
			dialogActive = false;
			dialogLine = 0;
		}
		return;
	}

	// L / R scroll the hotbar selection (works outside and inside the shops)
	if (kDown & (KEY_L | KEY_R)) {
		hotbarSel = (hotbarSel + ((kDown & KEY_R) ? 1 : HOTBAR_SLOTS - 1)) % HOTBAR_SLOTS;
		hbNameTimer = 110;
	}

	// eating: you stand still while you munch
	if (eatTimer > 0) {
		updateEating();
		playerMoving = false;
		updateWorld();
		return;
	}
	if (kDown & KEY_Y) { // Y = eat / drink the held food
		tryEat();
		playerMoving = false;
		updateWorld();
		return;
	}

	if (kDown & KEY_B) {
		openJournal();
		return;
	}

	// ---- player movement
	circlePosition cp;
	hidCircleRead(&cp);
	float mx = cp.dx / 156.0f, my = -cp.dy / 156.0f;
	if (fabsf(mx) < 0.15f) mx = 0;
	if (fabsf(my) < 0.15f) my = 0;
	if (kHeld & KEY_DLEFT) mx = -1;
	if (kHeld & KEY_DRIGHT) mx = 1;
	if (kHeld & KEY_DUP) my = -1;
	if (kHeld & KEY_DDOWN) my = 1;
	float len = sqrtf(mx * mx + my * my);
	if (len > 1.0f) { mx /= len; my /= len; }
	if (len > 0.01f) { playerMoving = true; pfx = mx; pfy = my; }

	float speed = 1.7f;

	if (insideHouse) {
		float rx = inX + mx * speed;
		if (!roomBlocked(rx, inY)) inX = rx;
		float ry = inY + my * speed;
		if (!roomBlocked(inX, ry)) inY = ry;

		if ((kDown & KEY_A) && shopId == SCHOOL_ID) { // chat with a teacher or a student
			int sn = nearSchoolNpc();
			if (sn >= 0) {
				talkingToOwner = false;
				talkingWalker = -1;
				talkingGame = -1;
				talkingSchool = sn;
				dialogActive = true;
				dialogLine = 0;
				playerMoving = false;
				pfx = 0; pfy = -1; // turn to face them
				return;
			}
		}
		if ((kDown & KEY_A) && nearOwner()) {
			talkingToOwner = true;
			talkingWalker = -1;
			talkingGame = -1;
			dialogActive = true;
			dialogLine = 0;
			playerMoving = false;
			pfx = 0; pfy = -1; // turn to face her
			return;
		}
		if ((kDown & KEY_A) && atExit()) {
			leaveHouse();
			return;
		}
		updateWorld(); // Jay and the pets keep going outside
		return;
	}

	float nx = px + mx * speed;
	if (!blockedAt(nx, py) && !vehicleBlocks(nx, py)) px = nx;
	float ny = py + my * speed;
	if (!blockedAt(px, ny) && !vehicleBlocks(px, ny)) py = ny;

	// ---- enter the cottage (A) / talk to Jay (A). At the door, A always enters.
	if ((kDown & KEY_A) && atDoor()) {
		enterHouse(0);
		return;
	}
	if ((kDown & KEY_A) && atSchoolDoor()) {
		enterHouse(SCHOOL_ID);
		return;
	}
	if ((kDown & KEY_A) && atBakeryDoor()) {
		enterHouse(1);
		return;
	}
	if ((kDown & KEY_A) && atSariDoor()) {
		enterHouse(2);
		return;
	}
	if ((kDown & KEY_A) && atStreetDoor()) {
		enterHouse(atStreetDoor());
		return;
	}
	if ((kDown & KEY_A) && nearJay()) {
		talkingToOwner = false;
		talkingWalker = -1;
		talkingGame = -1;
		dialogActive = true;
		dialogLine = 0;
		playerMoving = false;
		// Jay and the player turn to face each other
		float dx = px + PLAYER_SIZE / 2.0f - jay.x;
		float dy = py + PLAYER_SIZE / 2.0f - jay.y;
		float d = sqrtf(dx * dx + dy * dy);
		if (d > 0.1f) {
			jay.fx = dx / d; jay.fy = dy / d;
			pfx = -jay.fx; pfy = -jay.fy;
		}
		jay.wait = 60;
		return;
	}
	if ((kDown & KEY_A) && nearWalker() >= 0) {
		talkingToOwner = false;
		talkingWalker = nearWalker();
		talkingGame = -1;
		dialogActive = true;
		dialogLine = 0;
		playerMoving = false;
		// you turn to face each other
		Walker *w = &walkers[talkingWalker];
		float dx = px + PLAYER_SIZE / 2.0f - w->x;
		float dy = py + PLAYER_SIZE / 2.0f - w->y;
		float d = sqrtf(dx * dx + dy * dy);
		if (d > 0.1f) {
			w->fx = dx / d; w->fy = dy / d;
			pfx = -w->fx; pfy = -w->fy;
		}
		w->walking = false;
		w->wait = 60;
		return;
	}
	if ((kDown & KEY_A) && nearKid() >= 0) {
		talkingToOwner = false;
		talkingWalker = -1;
		talkingGame = kidAt(nearKid()).game;
		dialogActive = true;
		dialogLine = 0;
		playerMoving = false;
		return;
	}
	if ((kDown & KEY_A) && nearCaroler() >= 0) {
		talkingToOwner = false;
		talkingWalker = -1;
		talkingCarol = nearCaroler();
		talkingGame = GAME_CAROL;
		dialogActive = true;
		dialogLine = 0;
		playerMoving = false;
		int gx, gy; // turn to face them
		carolSpot(talkingCarol, &gx, &gy);
		float dx = gx - (px + PLAYER_SIZE / 2.0f), dy = gy - (py + PLAYER_SIZE / 2.0f);
		float d = sqrtf(dx * dx + dy * dy);
		if (d > 0.1f) { pfx = dx / d; pfy = dy / d; }
		return;
	}
	if (kDown & KEY_A) {
		int pi = nearPet();
		if (pi >= 0) {
			bool wasHappy = pets[pi].happy > 0;
			petPet(pi);
			// holding the right food? feed it, and it stays happy longer
			int food = pets[pi].kind == PET_DOG ? ITEM_DOG_FOOD :
			           pets[pi].kind == PET_CAT ? ITEM_CAT_FOOD : -2; // -2: nothing (-1 is an empty slot)
			if (!wasHappy && invType[hotbarSel] == food) {
				if (--invCount[hotbarSel] == 0) invType[hotbarSel] = -1;
				pets[pi].happy = 240;
			}
		}
	}

	updateWorld();
}

// ---------------------------------------------------------------- characters

// chibi character; (cx, fy) = horizontal center and feet line
UNUSED_FN static void drawChar(int cx, int fy, float fx, float fyv, bool walking,
                     u32 skin, u32 hair, u32 body, u32 accent, bool girl) {
	int step = walking ? ((frameCount / 8) & 1) : 0;
	int b = step;
	u32 legc = girl ? skin : RGB(84, 72, 104);
	u32 shoe = RGB(110, 70, 70);

	rect(cx - 7, fy - 2, 14, 4, SHADOW);
	rect(cx - 3, fy - 4 - (step ? 1 : 0), 2, 4, legc);
	rect(cx + 1, fy - 4 - (step ? 0 : 1), 2, 4, legc);
	rect(cx - 3, fy - 1 - (step ? 1 : 0), 2, 1, shoe);
	rect(cx + 1, fy - 1 - (step ? 0 : 1), 2, 1, shoe);

	blob(cx - 5, fy - 12 - b, 10, 9, body, INK);
	if (girl) blob(cx - 6, fy - 7 - b, 12, 4, body, INK);
	rect(cx - 3, fy - 11 - b, 6, 2, accent);
	if (girl) rect(cx - 1, fy - 10 - b, 2, 3, RGB(255, 120, 160));

	blob(cx - 6, fy - 24 - b, 12, 12, skin, INK);

	bool up = fyv < -0.4f;
	if (up) {
		blob(cx - 6, fy - 24 - b, 12, 11, hair, INK);
	} else {
		rect(cx - 5, fy - 23 - b, 10, 4, hair);
		rect(cx - 6, fy - 21 - b, 2, 5, hair);
		rect(cx + 4, fy - 21 - b, 2, 5, hair);
		rect(cx - 3, fy - 19 - b, 3, 1, hair);
		rect(cx + 1, fy - 19 - b, 2, 1, hair);
		int ex = (int)lroundf(fx * 1.4f);
		if (ex > 1) ex = 1;
		if (ex < -1) ex = -1;
		u32 eye = RGB(50, 36, 60);
		rect(cx - 3 + ex, fy - 18 - b, 2, 3, eye);
		rect(cx + 1 + ex, fy - 18 - b, 2, 3, eye);
		rect(cx - 3 + ex, fy - 18 - b, 1, 1, RGB(255, 255, 255));
		rect(cx + 1 + ex, fy - 18 - b, 1, 1, RGB(255, 255, 255));
		rect(cx - 5 + ex, fy - 15 - b, 2, 1, RGBA(255, 130, 150, 200));
		rect(cx + 3 + ex, fy - 15 - b, 2, 1, RGBA(255, 130, 150, 200));
		rect(cx - 1 + ex, fy - 14 - b, 2, 1, RGB(200, 100, 110));
	}
	if (girl) {
		rect(cx - 8, fy - 20 - b, 2, 7, hair);
		rect(cx + 6, fy - 20 - b, 2, 7, hair);
		blob(cx + 1, fy - 26 - b, 6, 4, RGB(255, 120, 160), INK);
		rect(cx + 3, fy - 25 - b, 2, 2, GOLD_UI);
	} else {
		rect(cx - 1, fy - 26 - b, 4, 2, hair);
	}
}


// ---------------------------------------------------------------- player sprite
// 24x40 pixel-art girl (wavy brown hair, cream sweater, plaid skirt, black shoes).
// Letters are color roles so skin / hair / sweater can be customized:
//  a b c = hair dark/mid/light   s S p = skin / skin shade / blush
//  w W   = sweater / sweater shade   k m l = skirt   x = socks   o = shoes
//  q e y = eye lash / iris / eye white
#define SPR_W 24
#define SPR_H 40
#define SPR_LEG_ROW 33

static const char SPR[SPR_H][SPR_W + 1] = {
	"........................",
	".......bbbbbbbbbbb......",
	"......bbbbbbbbbbbbb.....",
	".....bbbbbbbbbbbbbbb....",
	"....bbbbbbbbbbbbbbbb....",
	"...bbcbbbbbbbbbbbbccb...",
	"...bbccccbbccbcbcccbb...",
	"..bbbbbccbbccccccbbbbb..",
	"..bbbbbbbbbbbbbbbbbbbb..",
	"..bbbbbbbbbbbsbbbbbbbb..",
	"..bbbbbbbsbbbssbbbbbbbb.",
	".abbbbbbbssbbssbbbbbbbb.",
	".abbbbcssssbbbssbbbbbbb.",
	".abbbbSSssssbbssSssbbbb.",
	".aabbaqqqqssbbqqqSabbbb.",
	"..abb.yybyssbbyybyaabbc.",
	"..cbbayeeyssssyeeyabbb..",
	"..bbbbpppsssssspppabbb..",
	".cbbbbppppssssspppbbbbbc",
	".bbbbbbbpSsssssSbbbbbbba",
	".bbbbbbbbSSSSSSSbbbbbbb.",
	"..bbbbbbWssssssWWbbbbba.",
	".cbbbbbWwwWWWWWwWbbbbb..",
	"..bbbbbWWWwwwwwWWbbbbb..",
	"....bbbwWWWWWWWWwbbbb...",
	"......wwWwwwwwwWww......",
	"......wwwwwwwwwwww......",
	"......wwwwwwwwwwww......",
	"......wwwwwwwwwwWw......",
	".....ssWwwwwwwwWWss.....",
	"......mmmmllllmmlmm.....",
	"......mmmmmlmllmkm......",
	"......lmmmmmmmmmml......",
	".......SssssmsssS.......",
	".......SsssS.sssS.......",
	".......xssx..xssx.......",
	".......xxxx..xxxx.......",
	".......xooo..ooox.......",
	".......oooooooooo.......",
	"........ooo..ooo........"
};

// ---------------------------------------------------------------- Jay's sprite
// Same 24x40 size, chibi proportions and flat pixel-art style as the player.
// Hair a b c = dark / mid / light     s S p = skin / shade / blush     q e y = eye
// j J = blazer / shade   g G = tee / print   u = white cuff
// n N = trousers / shade   o O = white sneakers / sole
static const char SPR_JAY[SPR_H][SPR_W + 1] = {
	".........b.....b........",
	"......b..bbb..bbbb......",
	".....bbbbbbbbbbbbbbb....",
	"....bbbbcccbbbbbbbbbbb..",
	"...bbcccccbbbbcbbbbbbbb.",
	".bbbbbbbbbbbbbbbbbbbbbb.",
	".bbbccbbbbbbbcbbbbbbbbb.",
	".bbbbbbbbbbbbbbbbbbbbbb.",
	".bbbbaabbbbbbbbbaabbbbb.",
	".bbbbbbbbbbbbbbbbbbbbbb.",
	".bbbbbbsbbbbbbsbbbbbbbb.",
	".bbbbbssbbbsbbbssbbbbbb.",
	".bbbbsssbbsssbbsssbbbbb.",
	".bbbbssssssssssssssbbbb.",
	".bbbbsqqqqssssqqqqsbbbb.",
	".bbbbsyyeessssyyeesbbbb.",
	".bbbbsyeeyssssyeeysbbbb.",
	".bbbbppppssSSssppppbbbb.",
	"....bbsssssSSsssssbb....",
	".......SssssssssS.......",
	"........SSSSSSSS........",
	"..........ssss..........",
	".....jjjjjSSSSjjjjj.....",
	".....jjjjJggggJjjjj.....",
	".....jjjjJggggJjjjj.....",
	".....jjjjJgGGgJjjjj.....",
	".....jjjjJggggJjjjj.....",
	".....jjjjJggggJjjjj.....",
	".....uujjJggggJjjuu.....",
	".....ss.JjjggjjJ.ss.....",
	".......nnnnnnnnnn.......",
	".......nnnnNNnnnn.......",
	".......nnnnnnnnnn.......",
	".......nnnnNNnnnn.......",
	".......nnnN..Nnnn.......",
	".......nnnN..Nnnn.......",
	".......NnnN..NnnN.......",
	"......ooooo..ooooo......",
	"......ooooo..ooooo......",
	"......OOOOO..OOOOO......"
};

static int jayVariant = 0; // 0 = Jay, 1 = Kuya Ben, 2 and 3 = villagers (same sprite, new colors)
static u32 jayColorBase(char ch) {
	switch (ch) {
	case 'a': return RGB(36, 24, 30);
	case 'b': return RGB(70, 54, 60);
	case 'c': return RGB(98, 80, 86);
	case 's': return RGB(235, 183, 162);
	case 'S': return RGB(211, 159, 138);
	case 'p': return RGB(240, 150, 150);
	case 'q': return RGB(10, 6, 6);
	case 'e': return RGB(84, 62, 84);
	case 'y': return RGB(246, 238, 230);
	case 'j': return RGB(82, 80, 98);
	case 'J': return RGB(60, 58, 76);
	case 'g': return RGB(224, 226, 234);
	case 'G': return RGB(186, 190, 202);
	case 'u': return RGB(248, 246, 250);
	case 'n': return RGB(62, 60, 84);
	case 'N': return RGB(46, 44, 66);
	case 'o': return RGB(250, 250, 250);
	case 'O': return RGB(196, 198, 210);
	default:  return RGB(255, 0, 255);
	}
}

static u32 jayColor(char ch) {
	if (jayVariant == 1) {
		switch (ch) {
		case 'j': return RGB(238, 154, 92);
		case 'J': return RGB(198, 112, 66);
		case 'n': return RGB(88, 104, 138);
		case 'N': return RGB(64, 78, 110);
		case 's': return RGB(214, 164, 132);
		case 'S': return RGB(186, 134, 106);
		case 'p': return RGB(226, 132, 120);
		default: break;
		}
	}
	if (jayVariant == 2) { // green shirt, brown trousers
		if (ch == 'j') return RGB(96, 158, 108);
		if (ch == 'J') return RGB(68, 120, 82);
		if (ch == 'n') return RGB(142, 106, 82);
		if (ch == 'N') return RGB(108, 78, 62);
	}
	if (jayVariant == 3) { // white polo, navy trousers, tan skin
		if (ch == 'j') return RGB(236, 232, 220);
		if (ch == 'J') return RGB(196, 190, 182);
		if (ch == 's') return RGB(196, 140, 108);
		if (ch == 'S') return RGB(170, 116, 90);
	}
	return jayColorBase(ch);
}

static u32 spriteColor(char ch, u32 skin, u32 hair, u32 sweater) {
	switch (ch) {
	case 'a': return shadeColor(hair, -30, -30, -30);
	case 'b': return hair;
	case 'c': return shadeColor(hair, 22, 22, 22);
	case 's': return skin;
	case 'S': return shadeColor(skin, -24, -24, -24);
	case 'p': return mixc(skin, RGB(250, 140, 160), 0.30f);
	case 'w': return sweater;
	case 'W': return shadeColor(sweater, -20, -20, -26);
	case 'k': return shadeColor(skirtColors[skirtIndex], -56, -50, -44);
	case 'm': return skirtColors[skirtIndex];
	case 'l': return shadeColor(skirtColors[skirtIndex], 34, 34, 34);
	case 'x': return RGB(250, 250, 250);
	case 'o': return shoeColors[shoeIndex];
	case 'q': return RGB(10, 6, 6);
	case 'e': return RGB(150, 122, 100);
	case 'y': return RGB(246, 238, 230);
	default:  return RGB(255, 0, 255);
	}
}

// Reshapes the long hair into another style: 0 long, 1 bob, 2 twin buns, 3 braids
static void applyHairStyle(char out[SPR_H][SPR_W], int style) {
	if (style == 0) return;
	// every other style starts by trimming the hair at the chin
	for (int r = 21; r < SPR_H; r++)
		for (int c = 0; c < SPR_W; c++)
			if (out[r][c] == 'a' || out[r][c] == 'b' || out[r][c] == 'c')
				out[r][c] = (c >= 6 && c <= 17 && r <= 28) ? 'w' : '.';
	if (style == 2) {
		for (int r = 0; r <= 7; r++)
			for (int c = 0; c < SPR_W; c++) {
				int cx = (c < SPR_W / 2) ? 3 : 20;
				int d = (c - cx) * (c - cx) + (r - 3) * (r - 3);
				if (d <= 10) out[r][c] = d >= 7 ? 'a' : (d <= 1 ? 'c' : 'b');
			}
	} else if (style == 3) {
		for (int r = 19; r <= 31; r++)
			for (int k = 0; k < 3; k++) {
				char ch = ((r + k) % 4 == 0) ? 'a' : (((r + k) % 4 == 2) ? 'c' : 'b');
				if (r >= 29) ch = (r == 29) ? 'm' : 'b'; // tie and tip
				out[r][3 + k] = ch;
				out[r][18 + k] = ch;
			}
	}
}

// dir: 0 = down (front), 1 = up (back of head), 2 = left, 3 = right
static void buildSprite(char out[SPR_H][SPR_W], const char (*src)[SPR_W + 1], int dir) {
	for (int r = 0; r < SPR_H; r++)
		for (int c = 0; c < SPR_W; c++) out[r][c] = src[r][c];

	if (dir == 1) {
		for (int r = 9; r <= 21; r++)
			for (int c = 0; c < SPR_W; c++)
				if (strchr("sSpqey", out[r][c]))
					out[r][c] = (((c + r) & 3) == 0) ? 'a' : 'b';
		// back view: no shirt front / bare neck (only Jay's sprite has g, G)
		for (int r = 22; r < SPR_H; r++)
			for (int c = 0; c < SPR_W; c++) {
				if (out[r][c] == 'g' || out[r][c] == 'G') out[r][c] = 'j';
				if (r == 22 && out[r][c] == 'S') out[r][c] = 'a';
			}
	} else if (dir == 2 || dir == 3) {
		int dx = (dir == 3) ? 1 : -1;
		char feat[SPR_H][SPR_W];
		memset(feat, 0, sizeof(feat));
		for (int r = 9; r <= 19; r++)
			for (int c = 0; c < SPR_W; c++)
				if (strchr("qeyp", out[r][c])) { feat[r][c] = out[r][c]; out[r][c] = 's'; }
		for (int r = 9; r <= 19; r++)
			for (int c = 0; c < SPR_W; c++) {
				if (!feat[r][c]) continue;
				int nc = c + dx;
				if (nc >= 0 && nc < SPR_W && (out[r][nc] == 's' || out[r][nc] == 'S')) out[r][nc] = feat[r][c];
			}
	}
}

// (cx, fy) = horizontal center and the line the shoes stand on
static void drawSpriteGen(int cx, int fy, int dir, bool walking, const char (*src)[SPR_W + 1], bool isJay,
                          u32 skin, u32 hair, u32 sweater) {
	char buf[SPR_H][SPR_W];
	char styled[SPR_H][SPR_W + 1];
	if (!isJay) {
		char shaped[SPR_H][SPR_W];
		for (int r = 0; r < SPR_H; r++) memcpy(shaped[r], src[r], SPR_W);
		applyHairStyle(shaped, styleIndex);
		for (int r = 0; r < SPR_H; r++) {
			memcpy(styled[r], shaped[r], SPR_W);
			styled[r][SPR_W] = 0;
		}
		src = (const char (*)[SPR_W + 1])styled;
	}
	buildSprite(buf, src, dir);
	int step = walking ? ((frameCount / 8) & 1) : 0;
	int x0 = cx - SPR_W / 2, y0 = fy - SPR_H;
	int mid = SPR_W / 2;

	if (spriteRows == SPR_H) rect(cx - 8, fy - 2, 16, 4, SHADOW);
	for (int r = 0; r < spriteRows; r++) {
		int c = 0;
		while (c < SPR_W) {
			char ch = buf[r][c];
			if (ch == '.') { c++; continue; }
			int e = c + 1;
			while (e < SPR_W && buf[r][e] == ch && !(r >= SPR_LEG_ROW && e == mid)) e++;
			int dy = 0;
			if (r >= SPR_LEG_ROW) dy = (c < mid) ? (step ? -1 : 0) : (step ? 0 : -1);
			rect(x0 + c, y0 + r + dy, e - c, 1, isJay ? jayColor(ch) : spriteColor(ch, skin, hair, sweater));
			c = e;
		}
	}
}

static void drawAccessory(int cx, int fy, int dir);

static void drawPlayerSprite(int cx, int fy, int dir, bool walking, u32 skin, u32 hair, u32 sweater) {
	drawSpriteGen(cx, fy, dir, walking, SPR, false, skin, hair, sweater);
}

// accessories sit on top of the head; (cx, fy) matches the sprite
static void drawAccessory(int cx, int fy, int dir) {
	int y = fy - SPR_H;
	if (accIndex == 1) { // ribbon
		blob(cx + 3, y + 1, 5, 6, ROSE, ROSE_DK);
		blob(cx + 9, y + 1, 5, 6, ROSE, ROSE_DK);
		rect(cx + 7, y + 3, 3, 3, ROSE_DK);
	} else if (accIndex == 2) { // sampaguita flower, tucked on one side
		int x = (dir == 1) ? cx + 5 : cx - 9;
		rect(x + 1, y + 5, 3, 7, WHITE);
		rect(x - 1, y + 7, 7, 3, WHITE);
		rect(x + 2, y + 8, 1, 1, GOLD_UI);
		rect(x + 4, y + 11, 3, 2, VINE);
	} else if (accIndex == 3) { // headband
		rect(cx - 9, y + 4, 18, 3, MINT_UI);
		rect(cx - 10, y + 6, 3, 3, MINT_UI);
		rect(cx + 7, y + 6, 3, 3, MINT_UI);
		for (int k = -7; k < 8; k += 4) rect(cx + k, y + 5, 1, 1, WHITE);
	} else if (accIndex == 4) { // salakot (woven hat)
		u32 straw = RGB(224, 188, 112), weave = RGB(168, 126, 70);
		rect(cx - 1, y - 4, 2, 2, weave);
		for (int r = 0; r < 5; r++) {
			int half = 3 + r * 2;
			rect(cx - half, y - 2 + r * 2, half * 2, 2, straw);
			rect(cx - half, y - 1 + r * 2, half * 2, 1, (r & 1) ? weave : RGB(202, 164, 92));
		}
		rect(cx - 12, y + 8, 24, 1, weave);
	}
}

static void drawJaySprite(int cx, int fy, int dir, bool walking) {
	drawSpriteGen(cx, fy, dir, walking, SPR_JAY, true, 0, 0, 0);
}

static int dirFromFacing(float fx, float fy) {
	if (fabsf(fy) >= fabsf(fx)) return fy < 0 ? 1 : 0;
	return fx < 0 ? 2 : 3;
}

static void drawVignette(void) {
	for (int i = 0; i < 6; i++) {
		u32 c = RGBA(70, 40, 70, (6 - i) * 9);
		rect(0, i * 4, SCR_W, 4, c);
		rect(0, SCR_H - 4 - i * 4, SCR_W, 4, c);
		rect(i * 4, 0, 4, SCR_H, c);
		rect(SCR_W - 4 - i * 4, 0, 4, SCR_H, c);
	}
}

// ---------------------------------------------------------------- pink title-screen art

static void drawLace(int y0, int w) {
	rect(0, y0, w, 240 - y0, PINK_LT);
	rect(0, y0, w, 3, PINK_PALE);
	rect(0, y0 + 3, w, 1, PINK_MID);
	rect(0, 240 - 5, w, 5, PINK_MID);
	int cy = y0 + (240 - y0) / 2 + 1;
	for (int dx = 0; dx < w + 16; dx += 16) {
		int cx = dx + 8;
		rect(cx - 1, cy - 8, 2, 2, WHITE);
		rect(cx - 3, cy - 6, 6, 2, WHITE);
		rect(cx - 5, cy - 4, 10, 2, WHITE);
		rect(cx - 7, cy - 2, 14, 2, WHITE);
		rect(cx - 5, cy,     10, 2, WHITE);
		rect(cx - 3, cy + 2, 6, 2, WHITE);
		rect(cx - 1, cy + 4, 2, 2, WHITE);
		rect(cx - 1, cy - 1, 2, 2, PINK_MID);   // diamond center
		rect(dx - 1, cy - 1, 2, 2, PINK_PALE);  // dot between diamonds
	}
}

static void drawBush(bool right) {
	u32 cols[3] = { PINK_DK, PINK_MID, PINK_LT };
	for (int layer = 0; layer < 3; layer++) {
		for (int i = 0; i < 10; i++) {
			u32 h = hash2(i + layer * 17, layer + (right ? 9 : 3));
			int w = 26 + (int)(h % 22) - layer * 3;
			int hh = 20 + (int)((h >> 5) % 16);
			int bx = -12 + (int)((h >> 9) % 110);
			int by = 118 + layer * 10 + (int)((h >> 14) % 60) - (110 - bx > 0 ? 0 : 0);
			if (right) bx = SCR_W - bx - w;
			blob(bx, by, w, hh, cols[layer], cols[layer]);
		}
	}
	// white blossoms
	for (int i = 0; i < 14; i++) {
		u32 h = hash2(i * 5 + 1, right ? 77 : 33);
		int fx = 6 + (int)(h % 100);
		int fy = 124 + (int)((h >> 7) % 76);
		if (right) fx = SCR_W - fx;
		rect(fx - 1, fy - 3, 3, 3, WHITE);
		rect(fx - 1, fy + 1, 3, 3, RGB(238, 226, 242));
		rect(fx - 3, fy - 1, 3, 3, WHITE);
		rect(fx + 1, fy - 1, 3, 3, WHITE);
		rect(fx - 1, fy - 1, 3, 3, RGB(255, 224, 150));
	}
}

static void drawPole(int x, int top) {
	rect(x, top, 6, 212 - top, RGB(176, 116, 146));
	rect(x + 4, top, 2, 212 - top, RGB(150, 94, 124));
	rect(x - 14, top + 14, 34, 4, RGB(176, 116, 146));
	rect(x - 12, top + 10, 3, 5, WHITE);
	rect(x + 15, top + 10, 3, 5, WHITE);
}

static void drawTitleScene(bool withTitle) {
	// sky gradient
	for (int y = 0; y < 212; y += 4)
		rect(0, y, SCR_W, 4, mixc(RGB(208, 200, 228), RGB(250, 218, 232), (float)y / 160.0f > 1 ? 1 : (float)y / 160.0f));

	// drifting clouds
	for (int i = 0; i < 4; i++) {
		int cx = ((frameCount / 5 + i * 150) % 520) - 110;
		int cy = 14 + i * 24;
		rect(cx, cy + 8, 56, 8, RGB(244, 236, 248));
		rect(cx + 8, cy + 3, 24, 8, RGB(244, 236, 248));
		rect(cx + 26, cy + 5, 20, 8, RGB(244, 236, 248));
		rect(cx, cy + 14, 56, 2, RGB(224, 214, 236));
	}

	// utility poles + sagging wire
	drawPole(60, 70);
	drawPole(332, 56);
	for (int x = 52; x < 336; x += 4) {
		float u = (x - 52) / 284.0f;
		float y0 = 82 + (68 - 82) * u + 16.0f * (1.0f - (2 * u - 1) * (2 * u - 1));
		rect(x, (int)y0, 4, 2, RGB(150, 98, 132));
	}

	// lattice fence behind the title
	rect(92, 140, 216, 6, PINK_MID);
	rect(96, 146, 208, 62, RGB(246, 212, 228));
	for (int gy = 0; gy < 10; gy++)
		for (int gx = 0; gx < 35; gx++)
			if (((gx + gy) & 1) == 0) rect(96 + gx * 6 + 1, 146 + gy * 6 + 1, 4, 4, RGB(255, 238, 246));
	rect(96, 204, 208, 4, PINK_MID);

	// blossoming bushes on both sides
	drawBush(false);
	drawBush(true);

	// falling petals
	for (int i = 0; i < 16; i++) {
		float fx = fmodf(i * 53.0f + frameCount * 0.5f + sinf(frameCount * 0.03f + i) * 10.0f, 420.0f) - 10.0f;
		float fy = fmodf(i * 29.0f + frameCount * 0.7f, 250.0f) - 10.0f;
		rect((int)fx, (int)fy, 3, 2, RGB(255, 236, 244));
		rect((int)fx + 1, (int)fy + 2, 2, 1, RGB(246, 190, 214));
	}

	drawLace(212, SCR_W);

	if (withTitle) {
		int bob = (int)(sinf(frameCount * 0.05f) * 2.0f);
		pixTextC(SCR_W / 2, 24 + bob, 6, "MAHAL", WHITE, PINK_MID, MAUVE);
		pixTextC(SCR_W / 2, 76 + bob, 6, "KITA", WHITE, PINK_MID, MAUVE);
		drawHeartO(66, 60 + bob, 3, PINK_MID, WHITE);
		drawHeartO(310, 60 + bob, 3, PINK_MID, WHITE);
		pixTextC(SCR_W / 2, 124, 2, "IKAW LANG ANG IIBIGIN", RGB(150, 84, 126), 0, WHITE);
	}
}

static void drawPinkBottomBg(void) {
	for (int y = 0; y < 240; y += 6)
		rect(0, y, BOT_W, 6, mixc(RGB(248, 214, 232), RGB(254, 238, 244), (float)y / 240.0f));
}

static void drawMenuButton(const MenuBtn *b, const char *label, bool selected) {
	u32 fill = selected ? PINK_LT : RGB(240, 228, 238);
	u32 out  = selected ? PINK_DK : RGB(190, 164, 184);
	pill(b->x, b->y, b->w, b->h, fill, out);
	rect(b->x + 12, b->y + 5, b->w - 24, 2, selected ? RGB(255, 226, 238) : WHITE);
	rect(b->x + 12, b->y + b->h - 7, b->w - 24, 2, selected ? PINK_MID : RGB(208, 192, 206));
	int tw = pixWidth(label, 2);
	int tx = b->x + (b->w - tw) / 2;
	int ty = b->y + (b->h - 14) / 2;
	if (selected) {
		pixRuns(tx + 1, ty + 1, 2, label, PLUM, 0);
		pixRuns(tx, ty, 2, label, WHITE, 0);
		int pulse = (frameCount / 10) & 1;
		drawHeartO(b->x - 20 - pulse, b->y + 12, 1, PINK_MID, WHITE);
		drawHeartO(b->x + b->w + 12 + pulse, b->y + 12, 1, PINK_MID, WHITE);
	} else {
		pixRuns(tx + 1, ty + 1, 2, label, WHITE, 0);
		pixRuns(tx, ty, 2, label, RGB(156, 128, 150), 0);
	}
}

// ---------------------------------------------------------------- screens

// Big leafy tree: six rounded clumps of bright leaves on a thick brown trunk with two
// spreading branches. (sx, sy) is the top-left of its one-tile trunk; the canopy overhangs
// the tiles around it (about 62 px wide and 72 px tall), so trees are drawn in their own
// pass after the ground.

// an octagon with stepped corners: three rectangles, so it costs only three shapes
static void treeOctagon(int x, int y, int w, int h, int s, u32 c) {
	rect(x + 2 * s, y, w - 4 * s, h, c);
	rect(x + s, y + s, w - 2 * s, h - 2 * s, c);
	rect(x, y + 2 * s, w, h - 4 * s, c);
}

// one clump of leaves: dark outline, bright face, lit top, shaded underside (bh tall) and a glint
static void treeLobe(int x, int y, int w, int h, int bh) {
	const int s = 3;
	treeOctagon(x, y, w, h, s, TREE_OUT);
	treeOctagon(x + 1, y + 1, w - 2, h - 2, s, TREE_MID);
	rect(x + 2 * s, y + h - 1 - bh, w - 4 * s, bh, TREE_DK);            // shaded underside
	rect(x + w - 4, y + s + 2, 3, h - 2 * s - 6, TREE_DK);              // shaded right edge
	treeOctagon(x + w / 4, y + 3, w / 2, h * 2 / 5, 2, TREE_HI);        // lit top
	rect(x + w / 2 - 2, y + h / 3, 4, 1, TREE_GLINT);                   // glint
}

static void drawTree(int sx, int sy, int tx, int ty) {
	int cx = sx + 8, gy = sy + 16; // cx = middle of the trunk tile, gy = the ground line

	// ground shadow + a few fallen leaves
	rect(cx - 17, gy - 5, 34, 7, SHADOW);
	for (int k = 0; k < 4; k++) {
		u32 q = hash2(tx * 7 + k, ty * 13 + k);
		rect(cx - 16 + (int)(q % 32), gy - 2 + (int)((q >> 8) % 3), 2, 1, (k & 1) ? TREE_HI : TREE_MID);
	}

	// dark backing behind the clumps, so no gaps show between them
	treeOctagon(cx - 27, gy - 68, 54, 46, 6, TREE_OUT);

	// the six clumps, back to front: top, upper left, upper right, centre, lower left, lower right
	treeLobe(cx - 12, gy - 72, 24, 14, 3);
	treeLobe(cx - 28, gy - 65, 23, 23, 3);
	treeLobe(cx + 5, gy - 65, 23, 23, 3);
	treeLobe(cx - 18, gy - 60, 37, 25, 4);
	treeLobe(cx - 31, gy - 42, 24, 23, 6);
	treeLobe(cx + 8, gy - 42, 24, 23, 6);

	// trunk and branches: all outlines first, then the fills on top
	rect(cx - 6, gy - 35, 12, 7, TRUNK_OUT);       // forked top
	rect(cx - 6, gy - 29, 12, 20, TRUNK_OUT);      // trunk
	rect(cx - 8, gy - 10, 16, 7, TRUNK_OUT);       // widening
	rect(cx - 11, gy - 4, 22, 4, TRUNK_OUT);       // root flare
	for (int i = 0; i < 3; i++) {                  // branches climb outwards in three steps
		rect(cx - 9 - 4 * i, gy - 19 - 2 * i, 7, 6, TRUNK_OUT);
		rect(cx + 2 + 4 * i, gy - 19 - 2 * i, 7, 6, TRUNK_OUT);
	}

	rect(cx - 5, gy - 34, 10, 6, TRUNK_MID);
	rect(cx - 1, gy - 35, 2, 3, TREE_OUT);         // notch between the two forks
	rect(cx - 5, gy - 29, 10, 20, TRUNK_MID);
	rect(cx - 3, gy - 29, 2, 18, TRUNK_HI);
	rect(cx + 2, gy - 29, 3, 20, TRUNK_DK);
	rect(cx, gy - 24, 1, 3, TRUNK_DK);             // bark marks
	rect(cx - 4, gy - 15, 1, 3, TRUNK_DK);
	rect(cx - 7, gy - 10, 14, 7, TRUNK_MID);
	rect(cx - 4, gy - 10, 2, 7, TRUNK_HI);
	rect(cx + 3, gy - 10, 4, 7, TRUNK_DK);
	rect(cx - 10, gy - 4, 20, 3, TRUNK_MID);
	rect(cx - 7, gy - 4, 4, 2, TRUNK_HI);
	rect(cx + 4, gy - 4, 6, 3, TRUNK_DK);
	rect(cx - 15, gy - 2, 30, 2, TRUNK_OUT);       // soil line at the foot of the tree
	for (int i = 0; i < 3; i++) {                  // branch fills
		rect(cx - 8 - 4 * i, gy - 18 - 2 * i, 5, 4, TRUNK_MID);
		rect(cx - 8 - 4 * i, gy - 18 - 2 * i, 5, 1, TRUNK_HI);
		rect(cx + 3 + 4 * i, gy - 18 - 2 * i, 5, 4, TRUNK_MID);
		rect(cx + 3 + 4 * i, gy - 18 - 2 * i, 5, 1, TRUNK_HI);
	}
}

// street lamp: tall, so it is drawn in the same depth-sorted pass as the trees
// Lamps (and anything else that lights up at night) register a glow while the world is drawn;
// drawDayNight() paints them after the night tint so they stay bright.
#define MAX_GLOWS 40
static int glowN = 0;
static int glowX[MAX_GLOWS], glowY[MAX_GLOWS], glowGround[MAX_GLOWS];
static void addNightGlow(int x, int y, int groundY) {
	if (glowN >= MAX_GLOWS) return;
	glowX[glowN] = x; glowY[glowN] = y; glowGround[glowN] = groundY;
	glowN++;
}

// ---------------------------------------------------------------- seasonal decorations
// Parol centres and pumpkin faces are remembered here and lit up by drawDayNight().
#define MAX_EV_GLOWS 64
static int evGlowN = 0;
static int evGlow[MAX_EV_GLOWS][4];
static void eventGlow(int x, int y, int w, int h) {
	if (evGlowN >= MAX_EV_GLOWS || x > SCR_W || x + w < 0 || y > SCR_H || y + h < 0) return;
	evGlow[evGlowN][0] = x; evGlow[evGlowN][1] = y; evGlow[evGlowN][2] = w; evGlow[evGlowN][3] = h;
	evGlowN++;
}

// A parol (Filipino star lantern); (cx, cy) is its centre. The colours slowly blink.
static void drawParol(int cx, int cy, int seed) {
	static const u8 HALF[13] = { 1, 1, 2, 2, 8, 6, 4, 4, 5, 5, 3, 2, 1 };
	static const u8 GAP[13]  = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 3, 3 };
	int k = (frameCount / 24 + seed) % 3;
	u32 c = k == 0 ? RGB(236, 72, 96) : k == 1 ? RGB(250, 190, 60) : RGB(80, 180, 120);
	for (int pass = 0; pass < 2; pass++) { // outline first, then the fill
		int g = pass == 0 ? 1 : 0;
		u32 col = pass == 0 ? INK : c;
		for (int r = 0; r < 13; r++) {
			int y = cy - 6 + r;
			if (GAP[r] == 0) rect(cx - HALF[r] - g, y - g, 2 * HALF[r] + 1 + 2 * g, 1 + 2 * g, col);
			else {
				rect(cx - HALF[r] - GAP[r] - g, y - g, HALF[r] + 2 * g, 1 + 2 * g, col);
				rect(cx + GAP[r] + 1 - g, y - g, HALF[r] + 2 * g, 1 + 2 * g, col);
			}
		}
	}
	rect(cx - 2, cy - 2, 5, 5, PAPER);          // glowing middle
	rect(cx - 1, cy - 1, 3, 3, GOLD_UI);
	eventGlow(cx - 2, cy - 2, 5, 5);
	rect(cx - 8, cy + 8, 1, 6, PAPER);          // tassels
	rect(cx + 8, cy + 8, 1, 6, PAPER);
}

// A jack-o'-lantern standing on the ground; (cx, gy) is the middle of its bottom edge.
static void drawPumpkin(int cx, int gy, int s) {
	u32 face = ((frameCount / 10 + cx) % 7 == 0) ? RGB(255, 190, 80) : RGB(255, 232, 130);
	rect(cx - 6 * s, gy - 1, 12 * s, 2, SHADOW);
	blob(cx - 6 * s, gy - 10 * s, 12 * s, 10 * s, RGB(240, 130, 44), INK);
	rect(cx - 3 * s, gy - 9 * s, s, 8 * s, RGB(206, 98, 34));  // ribs
	rect(cx + 2 * s, gy - 9 * s, s, 8 * s, RGB(206, 98, 34));
	rect(cx - s, gy - 12 * s, 2 * s, 3 * s, VINE_DK);          // stem
	rect(cx - 4 * s, gy - 7 * s, 2 * s, 2 * s, face);          // eyes
	rect(cx + 2 * s, gy - 7 * s, 2 * s, 2 * s, face);
	rect(cx - 4 * s, gy - 4 * s, 8 * s, s, face);              // grin
	rect(cx - 2 * s, gy - 3 * s, 4 * s, s, face);
	eventGlow(cx - 4 * s, gy - 7 * s, 2 * s, 2 * s);          // the eyes and grin shine at night
	eventGlow(cx + 2 * s, gy - 7 * s, 2 * s, 2 * s);
	eventGlow(cx - 4 * s, gy - 4 * s, 8 * s, s);
}

// A string of blinking Christmas lights, sagging a little between its hooks.
static void drawLights(int x0, int x1, int y) {
	static const int SAG[6] = { 0, 2, 3, 3, 2, 0 };
	int blink = frameCount / 20;
	for (int x = x0, i = 0; x < x1; x += 6, i++) {
		int yy = y + SAG[i % 6];
		rect(x, yy, 6, 1, VINE_DK);
		int k = (i + blink) % 4;
		u32 c = k == 0 ? RGB(255, 80, 96) : k == 1 ? RGB(255, 214, 70) :
		        k == 2 ? RGB(96, 220, 130) : RGB(104, 180, 255);
		rect(x + 1, yy + 1, 3, 4, c);
		rect(x + 1, yy + 1, 1, 2, WHITE);
	}
}

// Santa hat or witch hat for a 40 px tall character; (cx, fy) = centre and feet line.
static void drawEventHat(int cx, int fy) {
	int top = fy - SPR_H + 3; // top of the hair
	if (activeEvent == EV_PASKO) {
		u32 red = RGB(214, 52, 68);
		rect(cx - 7, top - 3, 14, 4, red);
		rect(cx - 5, top - 6, 11, 3, red);
		rect(cx - 2, top - 8, 9, 2, red);
		rect(cx + 5, top - 7, 5, 4, red);
		rect(cx - 9, top, 18, 4, WHITE);          // fluffy trim
		rect(cx - 9, top + 4, 18, 1, PAPER_DK);
		rect(cx + 8, top - 4, 5, 5, WHITE);       // pom-pom
	} else if (activeEvent == EV_HALLOWEEN) {
		u32 black = RGB(52, 36, 66);
		rect(cx - 12, top + 1, 24, 3, black);     // brim
		rect(cx - 7, top - 4, 14, 5, black);
		rect(cx - 5, top - 8, 10, 4, black);
		rect(cx - 2, top - 11, 7, 3, black);
		rect(cx + 2, top - 13, 6, 2, black);
		rect(cx - 7, top - 1, 14, 2, RGB(240, 130, 44)); // orange band
		rect(cx - 2, top - 2, 4, 3, GOLD_UI);
	}
}

// Decorations on the front of a building. (cx = door centre, gy = ground line by the door,
// l and r = how far the walls reach left and right of the door, eave = height of the lights)
static void drawHouseDecor(int cx, int gy, int l, int r, int eave, int seed) {
	if (activeEvent == EV_PASKO) {
		drawLights(cx - l, cx + r, gy - eave);
		drawParol(cx - l + 10, gy - eave + 14, seed);
		drawParol(cx + r - 10, gy - eave + 14, seed + 1);
	} else if (activeEvent == EV_HALLOWEEN) {
		drawPumpkin(cx - 24, gy + 3, 1);
		drawPumpkin(cx + 24, gy + 3, 1);
		drawPumpkin(cx - l + 8, gy + 1, 1);
		for (int i = 0; i < 2; i++) { // cobwebs in the top corners
			int x = i == 0 ? cx - l : cx + r - 1, d = i == 0 ? 1 : -1;
			for (int k = 0; k < 14; k++) {
				rect(x + d * k, gy - eave, 1, 1, RGBA(255, 255, 255, 190));
				rect(x, gy - eave + k, 1, 1, RGBA(255, 255, 255, 190));
				rect(x + d * k, gy - eave + 13 - k, 1, 1, RGBA(255, 255, 255, 150));
				rect(x + d * k / 2, gy - eave + (13 - k) / 2, 1, 1, RGBA(255, 255, 255, 150));
			}
		}
	}
}

// The plaza centrepiece stands in the fountain (already solid, so nothing gets blocked):
// a big Christmas tree, or a giant jack-o'-lantern.
static void drawCenterpiece(int x, int y) {
	int cx = x + 32, gy = y + 30;
	if (activeEvent == EV_HALLOWEEN) {
		drawPumpkin(cx, gy, 3);
	} else if (activeEvent == EV_PASKO) {
		rect(cx - 4, gy - 8, 8, 8, TRUNK_DK);
		for (int tier = 0; tier < 3; tier++) { // three layers of leaves, widest at the bottom
			int base = gy - 6 - tier * 16, half = 24 - tier * 5;
			for (int k = 0; k < 6; k++) {
				int w = half - k * (half - 4) / 6;
				rect(cx - w - 1, base - (k + 1) * 4 - 1, 2 * w + 2, 5, TREE_OUT);
			}
			for (int k = 0; k < 6; k++) {
				int w = half - k * (half - 4) / 6;
				rect(cx - w, base - (k + 1) * 4, 2 * w, 4, k < 2 ? TREE_DK : TREE_MID);
			}
			drawLights(cx - half + 3, cx + half - 6, base - 8);
		}
		drawParol(cx, gy - 66, 0);                       // parol on top
		blob(cx - 17, gy - 6, 10, 8, ROSE, INK);         // presents
		rect(cx - 13, gy - 6, 2, 8, GOLD_UI);
		blob(cx + 7, gy - 7, 11, 9, SKY_UI, INK);
		rect(cx + 12, gy - 7, 2, 9, PAPER);
	}
}

// Bats and ghosts for Halloween, twinkling stars for Pasko. Drawn after the day/night tint.
static void drawEventSky(void) {
	float n = nightLevel();
	if (activeEvent == EV_HALLOWEEN) {
		rect(0, 0, SCR_W, SCR_H, RGBA(92, 40, 128, 30));
		for (int i = 0; i < 4; i++) { // bats
			int span = SCR_W + 60;
			int x = (frameCount * (2 + i % 2) / 2 + i * 131) % span - 30;
			int y = 22 + i * 17 + (int)(7.0f * sinf(frameCount * 0.05f + i * 2.0f));
			int up = ((frameCount / 7 + i) & 1) ? -3 : 1;
			u32 c = RGB(46, 30, 60);
			rect(x - 2, y, 5, 4, c);
			rect(x - 2, y - 2, 1, 2, c);
			rect(x + 2, y - 2, 1, 2, c);
			rect(x - 8, y + up, 6, 2, c);
			rect(x + 3, y + up, 6, 2, c);
			rect(x - 11, y + up + 2, 4, 2, c);
			rect(x + 8, y + up + 2, 4, 2, c);
			rect(x - 1, y + 1, 1, 1, GOLD_UI);
			rect(x + 1, y + 1, 1, 1, GOLD_UI);
		}
		if (n > 0.3f) { // friendly ghosts drift around after dark
			int a = (int)((n - 0.3f) / 0.7f * 170);
			for (int i = 0; i < 2; i++) {
				int span = SCR_W + 80;
				int x = (frameCount / 3 + i * 247) % span - 40;
				if (i == 1) x = SCR_W - x;
				int y = 70 + i * 80 + (int)(9.0f * sinf(frameCount * 0.03f + i * 3.0f));
				u32 c = RGBA(236, 240, 255, a), e = RGBA(40, 30, 60, a);
				rect(x - 5, y, 10, 3, c);
				rect(x - 8, y + 3, 16, 15, c);
				for (int k = 0; k < 3; k++) rect(x - 8 + k * 6, y + 18, 4, 3, c);
				rect(x - 4, y + 6, 2, 4, e);
				rect(x + 2, y + 6, 2, 4, e);
				rect(x - 2, y + 12, 4, 3, e);
			}
		}
	} else if (activeEvent == EV_PASKO && rainLevel < 0.5f) {
		for (int i = 0; i < 14; i++) {
			const int period = 110;
			u32 h = hash2(i, 205);
			int t = (frameCount + (int)(h % period)) % period;
			if (t >= 40) continue;
			int x = 10 + (int)((h >> 8) % (u32)(SCR_W - 20));
			int y = 10 + (int)((h >> 18) % (u32)(SCR_H - 20));
			int a = (t < 20 ? t : 40 - t) * 11;
			u32 col = RGBA(255, 236, 150, a);
			int s = 1 + (t > 10 && t < 30);
			rect(x - 2 * s, y, 4 * s + 1, 1, col);
			rect(x, y - 2 * s, 1, 4 * s + 1, col);
		}
	}
}

// Garlands along the top of the back wall inside every building.
static void drawRoomDecor(void) {
	if (activeEvent == EV_PASKO) {
		drawLights(ROOM_X, ROOM_X + ROOM_W - 4, ROOM_Y + 1);
		drawParol(ROOM_X + 12, ROOM_Y + 56, 0);
		drawParol(ROOM_X + ROOM_W - 13, ROOM_Y + 56, 1);
	} else if (activeEvent == EV_HALLOWEEN) {
		rect(ROOM_X, ROOM_Y + 2, ROOM_W, 1, INK);
		for (int x = ROOM_X + 4, i = 0; x < ROOM_X + ROOM_W - 8; x += 14, i++) { // bunting
			u32 c = (i & 1) ? RGB(240, 130, 44) : RGB(110, 62, 140);
			for (int k = 0; k < 5; k++) rect(x + k, ROOM_Y + 3 + k * 2, 10 - 2 * k, 2, c);
		}
		drawPumpkin(ROOM_X + 14, ROOM_Y + ROOM_H - 6, 1);
		drawPumpkin(ROOM_X + ROOM_W - 14, ROOM_Y + ROOM_H - 6, 1);
	}
}

static void drawLamp(int sx, int sy) {
	// about twice as tall as the player (the sprite is 40 px)
	rect(sx + 1, sy + 12, 14, 4, SHADOW);
	rect(sx + 4, sy + 8, 8, 7, STONE_DK);                 // stone base
	rect(sx + 4, sy + 8, 8, 1, STONE_HI);
	rect(sx + 5, sy + 5, 6, 3, STONE_MID);
	rect(sx + 7, sy - 40, 3, 46, FENCE_MID);             // pole
	rect(sx + 7, sy - 40, 1, 46, FENCE_HI);
	rect(sx + 6, sy + 3, 5, 2, FENCE_DK);               // rings on the pole
	rect(sx + 6, sy - 14, 5, 2, FENCE_DK);
	rect(sx + 6, sy - 38, 5, 2, FENCE_DK);
	rect(sx + 2, sy - 55, 14, 15, INK);                 // lantern box
	rect(sx + 4, sy - 53, 10, 11, SHOP_GLOW);
	rect(sx + 8, sy - 53, 1, 11, FENCE_DK);             // window frame
	rect(sx + 4, sy - 49, 10, 1, FENCE_DK);
	rect(sx + 1, sy - 59, 16, 4, FENCE_DK);             // roof
	rect(sx + 4, sy - 62, 10, 3, FENCE_DK);
	rect(sx + 8, sy - 65, 2, 3, FENCE_DK);
	int ga = 24 + (int)(rainLevel * 44); // the glow is stronger on rainy days
	rect(sx - 6, sy - 60, 30, 24, RGBA(255, 226, 150, ga));
	rect(sx - 10, sy - 54, 38, 12, RGBA(255, 226, 150, ga / 2));
	addNightGlow(sx + 8, sy - 47, sy + 3);
	if (activeEvent == EV_PASKO) drawParol(sx + 8, sy - 28, sx / TS);
	else if (activeEvent == EV_HALLOWEEN) drawPumpkin(sx + 14, sy + 14, 1);
}

// palm tree: a leaning trunk with a crown of fronds and a few coconuts
static void drawPalm(int sx, int sy, int tx, int ty) {
	int cx = sx + 8, gy = sy + 16;
	int lean = ((hash2(tx, ty) >> 3) & 1) ? 1 : -1;
	rect(cx - 9, gy - 4, 18, 5, SHADOW);
	for (int i = 0; i < 9; i++) { // trunk, bending as it climbs
		int x = cx - 2 + lean * (i * i) / 20;
		int y = gy - 6 - i * 5;
		rect(x, y, 4, 5, (i & 1) ? TRUNK_MID : TRUNK_DK);
		rect(x, y, 1, 5, TRUNK_HI);
	}
	int tcx = cx + lean * 3, tcy = gy - 52;
	for (int d = -1; d <= 1; d += 2) {
		for (int k = 0; k < 6; k++) { // upper fronds arch out and droop
			int fx = tcx + d * (k * 5 + 1) - (d < 0 ? 5 : 0);
			int fy = tcy + (k * k) / 3 - 2;
			rect(fx, fy, 6, 3, (k & 1) ? LEAF_MID : LEAF_HI);
			rect(fx, fy + 3, 6, 1, LEAF_DK);
		}
		for (int k = 0; k < 4; k++) { // lower fronds hang down
			int fx = tcx + d * (k * 3 + 1) - (d < 0 ? 4 : 0);
			int fy = tcy + 5 + k * 3;
			rect(fx, fy, 4, 3, LEAF_DK);
		}
	}
	rect(tcx - 2, tcy - 6, 5, 5, LEAF_HI);
	rect(tcx - 1, tcy - 9, 3, 3, LEAF_MID);
	rect(tcx - 6, tcy + 3, 3, 3, TRUNK_DK); // coconuts
	rect(tcx + 3, tcy + 4, 3, 3, TRUNK_DK);
	rect(tcx - 2, tcy + 4, 3, 3, TRUNK_MID);
}

// striped beach parasol standing in the sand
static void drawParasol(int sx, int sy, int tx, int ty) {
	int cx = sx + 8, gy = sy + 16;
	u32 h = hash2(tx, ty);
	u32 c = (h % 3) == 0 ? ROSE : (h % 3) == 1 ? SKY_UI : GOLD_UI;
	rect(cx - 16, gy - 4, 32, 6, SHADOW);
	rect(cx - 1, gy - 38, 3, 38, STONE_HI);
	rect(cx, gy - 38, 1, 38, STONE_MID);
	static const int RW[4] = { 40, 32, 22, 8 };
	static const int RY[4] = { -38, -41, -44, -46 };
	static const int RH[4] = { 3, 3, 3, 2 };
	for (int r = 0; r < 4; r++) {
		int x0 = cx - RW[r] / 2;
		for (int xx = 0; xx < RW[r]; xx += 6) {
			int w = RW[r] - xx < 6 ? RW[r] - xx : 6;
			rect(x0 + xx, gy + RY[r], w, RH[r], ((xx / 6) & 1) ? WHITE : c);
		}
	}
	rect(cx - 20, gy - 35, 40, 1, INK_SOFT);
	rect(cx - 1, gy - 49, 3, 3, INK_SOFT);
}

// wooden sign at the top of the boardwalk
static void drawBeachSign(int sx, int sy) {
	rect(sx + 1, sy + 13, 14, 3, SHADOW);
	rect(sx + 1, sy - 8, 3, 22, SHOP_WOOD_DK);
	rect(sx + 12, sy - 8, 3, 22, SHOP_WOOD_DK);
	blob(sx - 12, sy - 34, 40, 26, SHOP_WOOD_HI, INK);
	pixTextC(sx + 8, sy - 30, 1, "DAGAT", INK, 0, 0);
	pixTextC(sx + 8, sy - 20, 1, "BEACH", PINK_DK, 0, 0);
}

// scarecrow for the veggie garden
static void drawScarecrow(int sx, int sy) {
	rect(sx + 3, sy + 12, 10, 3, SHADOW);
	rect(sx + 7, sy - 6, 2, 20, BARK);
	rect(sx, sy, 16, 2, BARK);
	rect(sx + 4, sy + 1, 8, 8, RGB(196, 98, 112));
	rect(sx + 6, sy + 4, 3, 3, PAPER);
	rect(sx - 1, sy - 1, 2, 4, THATCH);
	rect(sx + 15, sy - 1, 2, 4, THATCH);
	blob(sx + 4, sy - 9, 8, 8, BAMBOO, INK);
	rect(sx + 6, sy - 6, 1, 1, INK);
	rect(sx + 9, sy - 6, 1, 1, INK);
	rect(sx + 2, sy - 11, 12, 2, THATCH_DK);
	rect(sx + 5, sy - 15, 6, 4, THATCH);
}

static void drawJayEntity(int sx, int sfeet, bool walking) {
	drawJaySprite(sx, sfeet, dirFromFacing(jay.fx, jay.fy), walking);
}

// Wading: in the shallows the water covers the legs, with ripples around them and splashes
// when you walk.
static void drawWading(int pcxs, int pfeet) {
	int wave = (frameCount / 8) % 4;
	int bob = playerMoving ? ((frameCount / 6) & 1) : 0;
	rect(pcxs - 11, pfeet - 9 - bob, 22, 10 + bob, RGBA(146, 216, 220, 200)); // water over the legs
	rect(pcxs - 11, pfeet - 9 - bob, 22, 1, SHALLOW_HI);                       // surface
	rect(pcxs - 9 + wave, pfeet - 5, 6, 1, SHALLOW_HI);
	rect(pcxs + 2 - wave, pfeet - 3, 6, 1, SHALLOW_HI);
	rect(pcxs - 14 - wave, pfeet - 1, 28 + wave * 2, 1, RGBA(252, 254, 255, 190)); // ripple ring
	if (playerMoving) { // splashes
		int k = (frameCount / 5) % 3;
		rect(pcxs - 12 - k, pfeet - 11 - k, 2, 2, FOAM);
		rect(pcxs + 10 + k, pfeet - 11 - k, 2, 2, FOAM);
		rect(pcxs - 1, pfeet - 13 - k, 2, 2, FOAM);
	}
}

static void drawPlayerEntity(int pcxs, int pfeet) {
	drawPlayerSprite(pcxs, pfeet, dirFromFacing(pfx, pfy), playerMoving,
	                 skinTones[skinIndex], hairColors[hairIndex], outfitColors[outfitIndex]);
	drawAccessory(pcxs, pfeet, dirFromFacing(pfx, pfy));
	if (accIndex != 4) drawEventHat(pcxs, pfeet); // the salakot stays on
	if (!insideHouse && tileAt(px + PLAYER_SIZE / 2.0f, py + PLAYER_SIZE - 1) == T_SHALLOW)
		drawWading(pcxs, pfeet);
}

static void drawPrompt(int cx, int y, const char *s) {
	pill(cx - 38, y, 76, 18, PAPER, PINK_DK);
	pixTextC(cx, y + 6, 1, s, PINK_DK, 0, 0);
}

static void drawDialogBox(const char *name, const char *line) {
	int tag = pixWidth(name, 1) + 28;
	if (tag < 84) tag = 84;
	blob(16, 150, 368, 78, PAPER, INK);
	rect(18, 152, 364, 3, PINK_PALE);
	pill(28, 140, tag, 20, PINK_LT, PINK_DK);
	pixTextC(28 + tag / 2, 144, 1, name, WHITE, 0, PLUM);
	drawText(30, 168, 0.55f, INK, line);
	drawText(236, 206, 0.42f, INK_SOFT, "A: next    B: close");
	drawHeart(358, 208, 1, ROSE);
}

// ---------------------------------------------------------------- inventory hotbar
static u32 breadIconColor(int i) {
	switch (i) {
	case 0:  return BREAD_MID;
	case 1:  return RGB(250, 236, 206);
	case 2:  return RGB(238, 214, 170);
	default: return RGB(222, 180, 120);
	}
}

// icons for the clothes, pet supplies and karenderya food
static void drawStreetItem(int x, int y, int type) {
	switch (type) {
	case 8: case 9: { // top
		u32 c = type == 8 ? RGB(248, 150, 184) : SKY_UI;
		rect(x + 1, y + 1, 14, 5, INK);
		rect(x + 2, y + 2, 12, 3, c);
		blob(x + 4, y + 1, 8, 11, c, INK);
		rect(x + 6, y + 1, 4, 2, PAPER);
		break;
	}
	case 10: // navy bottoms
		blob(x + 3, y + 1, 10, 11, RGB(96, 112, 164), INK);
		rect(x + 4, y + 3, 8, 1, WHITE);
		rect(x + 7, y + 6, 2, 6, INK);
		break;
	case 11: // red shoes
		blob(x + 1, y + 5, 7, 6, RGB(196, 70, 90), INK);
		blob(x + 8, y + 5, 7, 6, RGB(196, 70, 90), INK);
		rect(x + 3, y + 6, 2, 1, WHITE);
		rect(x + 10, y + 6, 2, 1, WHITE);
		break;
	case 12: case 13: { // dog food / cat food bag
		u32 c = type == 12 ? RGB(190, 132, 84) : MINT_UI;
		blob(x + 3, y, 10, 12, c, INK);
		rect(x + 5, y + 4, 6, 5, PAPER);
		rect(x + 6, y + 6, 4, 2, type == 12 ? BARK : ROSE);
		break;
	}
	case 14: // bola
		blob(x + 3, y + 1, 10, 10, ROSE, INK);
		rect(x + 4, y + 5, 8, 2, GOLD_UI);
		rect(x + 5, y + 3, 2, 1, WHITE);
		break;
	case 15: // collar with a little bell
		blob(x + 2, y + 1, 12, 7, ROSE_DK, INK);
		rect(x + 4, y + 3, 8, 3, PAPER);
		blob(x + 6, y + 6, 5, 5, GOLD_UI, INK);
		break;
	case 20: // taho: a clear cup of silken tofu with dark arnibal and sago pearls, and a straw
		blob(x + 3, y + 3, 10, 10, WHITE, INK);
		rect(x + 4, y + 4, 8, 4, RGB(255, 246, 226));          // soft tofu
		rect(x + 4, y + 8, 8, 3, RGB(150, 84, 44));            // arnibal syrup
		rect(x + 5, y + 9, 2, 2, INK);                         // sago pearls
		rect(x + 9, y + 9, 2, 2, INK);
		rect(x + 10, y - 2, 2, 6, ROSE);                       // straw
		break;
	case 19: // halo-halo in a tall glass
		blob(x + 4, y + 2, 8, 10, RGB(214, 188, 232), INK);
		rect(x + 5, y + 7, 6, 2, GOLD_UI);
		rect(x + 5, y + 9, 6, 2, ROSE);
		blob(x + 4, y - 1, 8, 5, RGB(150, 100, 170), INK);
		break;
	default: { // plates and bowls: adobo, sinigang, lugaw
		u32 food = type == 16 ? RGB(128, 78, 48) : type == 17 ? RGB(232, 168, 90) : RGB(255, 244, 214);
		blob(x + 1, y + 4, 14, 8, WHITE, INK);
		rect(x + 4, y + 5, 8, 3, food);
		if (type == 16) rect(x + 3, y + 5, 3, 3, PAPER);           // rice
		if (type == 17) rect(x + 6, y + 5, 2, 2, LEAF_MID);        // kangkong
		if (type == 18) rect(x + 8, y + 5, 2, 2, GOLD_UI);         // egg
		rect(x + 6, y, 1, 3, STONE_HI);                            // steam
		rect(x + 10, y + 1, 1, 3, STONE_HI);
		break;
	}
	}
}

// icon for any buyable item, drawn inside a 16 x 12 box at (x, y)
static void drawItem(int x, int y, int w, int h, int type) {
	(void)w; (void)h;
	if (type < BAKE_COUNT) { bun(x, y, 16, 12, breadIconColor(type), BREAD_DK); return; }
	if (type >= 8) { drawStreetItem(x, y, type); return; }
	switch (type) {
	case 4: // sago't gulaman: a cup of iced drink with a straw
		blob(x + 3, y + 2, 10, 10, RGB(206, 138, 84), INK);
		rect(x + 4, y + 3, 8, 2, RGB(250, 226, 190));
		rect(x + 5, y + 8, 2, 2, INK);
		rect(x + 9, y + 9, 2, 2, INK);
		rect(x + 9, y - 2, 2, 6, ROSE);
		break;
	case 5: // turon: golden fried banana roll
		blob(x + 1, y + 3, 14, 7, RGB(226, 168, 78), BREAD_DK);
		rect(x + 4, y + 4, 1, 5, BREAD_DK);
		rect(x + 8, y + 4, 1, 5, BREAD_DK);
		rect(x + 12, y + 4, 1, 5, BREAD_DK);
		rect(x + 3, y + 4, 10, 1, RGB(252, 232, 170));
		break;
	case 6: // chichirya: a bag of chips
		blob(x + 3, y, 10, 12, ROSE, INK);
		rect(x + 5, y + 4, 6, 4, GOLD_UI);
		rect(x + 4, y + 1, 8, 1, WHITE);
		break;
	default: // pastillas: milk candy in a twisted wrapper
		blob(x + 3, y + 3, 10, 7, PAPER, PINK_DK);
		rect(x, y + 5, 3, 3, PAPER);
		rect(x + 13, y + 5, 3, 3, PAPER);
		rect(x + 6, y + 5, 4, 1, PINK_MID);
		break;
	}
}

static u32 withAlpha(u32 c, int a) {
	return (c & 0x00FFFFFFu) | ((u32)((int)(c >> 24) * a / 255) << 24);
}

static float textW(const char *s, float scale) {
	C2D_Text t;
	float w = 0, h = 0;
	C2D_TextParse(&t, textBuf, s);
	C2D_TextOptimize(&t);
	C2D_TextGetDimensions(&t, scale, scale, &w, &h);
	return w;
}

#define HB_CELL 22
#define HB_W (HOTBAR_SLOTS * HB_CELL + 4)
#define HB_H (HB_CELL + 4)
#define HB_X ((SCR_W - HB_W) / 2)

// Soft pink hotbar. Outdoors it sits at the bottom of the screen like Minecraft's. Inside the
// shops the exit mat is at the bottom, so there it sits at the top (over the back wall) instead.
// (pcxs, pfeet) = the player's position on screen, so the bar can fade when they walk behind it.
static void drawHotbar(int pcxs, int pfeet, bool top) {
	int hy = top ? 14 : SCR_H - HB_H - 3;
	bool behind = pcxs + 12 > HB_X && pcxs - 12 < HB_X + HB_W &&
	              pfeet + 2 > hy && pfeet - SPR_H < hy + HB_H;
	int target = behind ? 90 : 255;
	if (hbFade < target) { hbFade += 30; if (hbFade > target) hbFade = target; }
	if (hbFade > target) { hbFade -= 30; if (hbFade < target) hbFade = target; }

	u32 cell = RGBA(255, 226, 238, 235);
	gAlpha = hbFade;

	// frame
	blob(HB_X, hy, HB_W, HB_H, PINK_LT, PINK_DK);
	rect(HB_X + 3, hy + 1, HB_W - 6, 1, PINK_PALE);

	// slots
	for (int i = 0; i < HOTBAR_SLOTS; i++) {
		int sx = HB_X + 2 + i * HB_CELL, sy = hy + 2;
		rect(sx, sy, HB_CELL, HB_CELL, PINK_MID);
		rect(sx + 1, sy + 1, HB_CELL - 2, HB_CELL - 2, cell);
		rect(sx + 1, sy + 1, HB_CELL - 2, 1, WHITE);                 // inset bevel: light top-left
		rect(sx + 1, sy + 1, 1, HB_CELL - 2, WHITE);
		rect(sx + 1, sy + HB_CELL - 2, HB_CELL - 2, 1, PINK_LT);    // shade bottom-right
		rect(sx + HB_CELL - 2, sy + 1, 1, HB_CELL - 2, PINK_LT);
	}

	// selector: white frame with a pink edge, drawn over the neighbouring slots
	{
		int sx = HB_X + 2 + hotbarSel * HB_CELL, sy = hy + 2;
		rect(sx - 2, sy - 2, HB_CELL + 4, HB_CELL + 4, PINK_DK);
		rect(sx - 1, sy - 1, HB_CELL + 2, HB_CELL + 2, WHITE);
		rect(sx + 1, sy + 1, HB_CELL - 2, HB_CELL - 2, RGB(255, 236, 244));
	}

	// items and stack counts
	for (int i = 0; i < HOTBAR_SLOTS; i++) {
		if (invType[i] < 0) continue;
		int sx = HB_X + 2 + i * HB_CELL, sy = hy + 2;
		drawItem(sx + 3, sy + 5, 16, 12, invType[i]);
		if (invCount[i] > 1) {
			char buf[8];
			snprintf(buf, sizeof(buf), "%d", invCount[i]);
			float rx = sx + HB_CELL - 3, ty = sy + 10, w = textW(buf, 0.4f);
			drawText(rx - w + 1, ty + 1, 0.4f, withAlpha(PLUM, hbFade), buf);
			drawText(rx - w, ty, 0.4f, withAlpha(WHITE, hbFade), buf);
		}
	}

	// name of the item you just switched to (fades out)
	gAlpha = 255;
	if (hbNameTimer > 0 && invType[hotbarSel] >= 0) {
		int a = hbNameTimer < 25 ? hbNameTimer * 255 / 25 : 255;
		const char *nm = ITEM_NAMES[invType[hotbarSel]];
		float w = textW(nm, 0.5f);
		int bw = (int)w + 22, bx = SCR_W / 2 - bw / 2;
		int by = top ? hy + HB_H + 3 : hy - 23;
		gAlpha = a;
		pill(bx, by, bw, 20, PINK_PALE, PINK_DK);
		gAlpha = 255;
		drawText(SCR_W / 2 - w / 2, by + 3, 0.5f, withAlpha(PLUM, a), nm);
	}
	gAlpha = 255;
}

// The BUSOG bar (top-left): a little rice bowl and a bar that empties as you get hungry.
static void drawFullnessHud(void) {
	int x = 6, y = 6, w = 74, h = 18;
	blob(x, y, w, h, PAPER, INK);
	rect(x + 6, y + 4, 8, 3, WHITE);          // rice
	rect(x + 4, y + 7, 12, 5, ROSE);          // bowl
	rect(x + 6, y + 12, 8, 1, ROSE_DK);
	int bx = x + 21, by = y + 6, bw = 48, bh = 6;
	u32 col = fullness > 50 ? MINT_UI : fullness >= 20 ? GOLD_UI : ROSE;
	if (fullness < 20 && ((frameCount / 15) & 1)) col = ROSE_DK; // flashes when you are very hungry
	rect(bx - 1, by - 1, bw + 2, bh + 2, INK);
	rect(bx, by, bw, bh, PAPER_DK);
	rect(bx, by, bw * fullness / FULL_MAX, bh, col);
}

// Food held up to the mouth, crumbs after each bite, a heart when done, and the speech bubble.
static void drawEatFx(int pcxs, int pfeet) {
	if (eatTimer > 0) {
		if (eatTimer > 14) { // the food bobs with every bite
			int bite = (eatTimer / 6) & 1;
			drawItem(pcxs - 8, pfeet - 31 - bite, 16, 12, eatItem);
		}
		for (int b = 0; b < 3; b++) { // crumbs (or drips) fly out after each bite
			int s = (44 - 13 * b) - eatTimer;
			if (s < 0 || s >= 9) continue;
			u32 c1 = isDrink(eatItem) ? WATER_MID : BREAD_MID;
			u32 c2 = isDrink(eatItem) ? WATER_HI : BREAD_HI;
			rect(pcxs - 7 - s / 2, pfeet - 24 + s, 2, 2, c1);
			rect(pcxs + 6 + s / 2, pfeet - 24 + s, 2, 2, c2);
			rect(pcxs - 1 + (s & 1), pfeet - 20 + s, 1, 1, c1);
		}
		if (eatTimer <= 14) // all done: a happy heart floats up
			drawHeartO(pcxs - 3, pfeet - SPR_H - 4 - (14 - eatTimer), 1, ROSE, PLUM);
	}
	if (eatMsgTimer > 0 && eatMsg) {
		int a = eatMsgTimer < 25 ? eatMsgTimer * 255 / 25 : 255;
		float tw = textW(eatMsg, 0.45f);
		int bw = (int)tw + 18;
		int bx = pcxs - bw / 2, by = pfeet - SPR_H - 26;
		if (bx < 2) bx = 2;
		if (bx + bw > SCR_W - 2) bx = SCR_W - 2 - bw;
		if (by < 2) by = 2;
		gAlpha = a;
		pill(bx, by, bw, 18, PAPER, PINK_DK);
		gAlpha = 255;
		drawText(bx + 9, by + 2, 0.45f, withAlpha(INK, a), eatMsg);
	}
}

// ---------------------------------------------------------------- pets and weather drawing
// rectangle relative to the pet's feet; dx grows toward the way the pet is facing
static void pr(int cx, int fy, int dir, int dx, int dy, int w, int h, u32 c) {
	rect(dir > 0 ? cx + dx : cx - dx - w, fy + dy, w, h, c);
}

static void pb(int cx, int fy, int dir, int dx, int dy, int w, int h, u32 fill, u32 out) {
	blob(dir > 0 ? cx + dx : cx - dx - w, fy + dy, w, h, fill, out);
}

// A string of sampaguita blossoms from (dx, dy), sagging in the middle.
static void drawSampaguita(int cx, int fy, int dir, int dx, int dy) {
	static const int SAG[5] = { 0, 2, 3, 2, 0 };
	for (int k = 0; k < 5; k++) {
		pr(cx, fy, dir, dx + k * 3, dy + SAG[k], 3, 3, WHITE);
		pr(cx, fy, dir, dx + k * 3 + 1, dy + SAG[k] + 1, 1, 1, GOLD_UI);
	}
	pr(cx, fy, dir, dx + 7, dy + 6, 2, 3, VINE);
}

// Kiko's big round head: curved horns, tiny salakot, shiny eyes and rosy cheeks.
// (hx, hy) = top-left of the head, measured like pr() from the feet.
static void drawKalabawHead(const Pet *p, int cx, int fy, int hx, int hy, bool eyesShut, bool flick) {
	int d = p->dir;
	u32 horn = RGB(238, 224, 190), hornDk = RGB(168, 142, 112);
	for (int side = 0; side < 2; side++) { // horns sweep out and up
		int bx = side ? hx + 14 : hx - 4;
		pr(cx, fy, d, bx - 1, hy + 1, 8, 5, hornDk);
		pr(cx, fy, d, bx, hy + 2, 6, 3, horn);
		pr(cx, fy, d, side ? bx + 4 : bx - 2, hy - 4, 4, 7, hornDk);
		pr(cx, fy, d, side ? bx + 5 : bx - 1, hy - 3, 2, 6, horn);
	}
	int ey = hy + 6 + (flick ? 1 : 0);
	pb(cx, fy, d, hx - 5, ey, 7, 5, p->coat, p->coatDk);        // ears
	pr(cx, fy, d, hx - 3, ey + 2, 3, 2, p->ear);
	pb(cx, fy, d, hx + 14, ey, 7, 5, p->coat, p->coatDk);
	pr(cx, fy, d, hx + 16, ey + 2, 3, 2, p->ear);
	pb(cx, fy, d, hx, hy, 16, 17, p->coat, p->coatDk);          // head
	pb(cx, fy, d, hx + 2, hy + 10, 13, 8, p->patch, p->coatDk); // muzzle
	pr(cx, fy, d, hx + 5, hy + 13, 2, 2, INK);                  // nostrils
	pr(cx, fy, d, hx + 10, hy + 13, 2, 2, INK);
	for (int k = 0; k < 2; k++) {
		int x = hx + 3 + k * 8;
		if (eyesShut) pr(cx, fy, d, x, hy + 7, 3, 1, INK);
		else {
			pr(cx, fy, d, x, hy + 5, 3, 4, INK);
			pr(cx, fy, d, x + 1, hy + 5, 1, 1, WHITE);              // sparkle
		}
	}
	pr(cx, fy, d, hx + 1, hy + 9, 3, 2, ROSE);                  // blush
	pr(cx, fy, d, hx + 13, hy + 9, 2, 2, ROSE);
	pr(cx, fy, d, hx + 1, hy - 1, 14, 3, THATCH_DK);            // salakot
	pr(cx, fy, d, hx + 3, hy - 3, 10, 2, THATCH);
	pr(cx, fy, d, hx + 6, hy - 5, 4, 2, THATCH);
	pr(cx, fy, d, hx + 7, hy - 6, 2, 1, THATCH_DK);
}

static void drawKalabaw(const Pet *p, int cx, int fy) {
	int d = p->dir;
	if (p->state == PS_SLEEP && p->happy == 0) { // lying down for a nap
		rect(cx - 17, fy - 2, 35, 4, SHADOW);
		pb(cx, fy, d, -16, -13, 25, 13, p->coat, p->coatDk);
		pr(cx, fy, d, -12, -3, 18, 2, p->patch);
		drawKalabawHead(p, cx, fy, 1, -16, true, false);
		int zy = fy - 30 - ((frameCount / 20) % 3) * 3;
		pixText(cx + d * 14, zy, 1, "Z", PLUM, 0, 0);
		return;
	}
	int step = p->state == PS_WALK ? (frameCount / 10) & 1 : 0;
	int wag = (frameCount / (p->happy > 0 ? 4 : 18)) & 1; // the tail swishes
	rect(cx - 16, fy - 2, 33, 4, SHADOW);
	for (int k = 0; k < 4; k++) { // stubby legs with dark hooves
		static const int LX[4] = { -13, -8, 0, 5 };
		int up = ((k & 1) == step && p->state == PS_WALK) ? 1 : 0;
		pr(cx, fy, d, LX[k], -8, 4, 8 - up, (k & 1) ? p->coat : p->coatDk);
		pr(cx, fy, d, LX[k], -2 - up, 4, 2, INK);
	}
	pr(cx, fy, d, -17 - wag, -17, 3, 9, p->coatDk);             // tail with a tuft
	pr(cx, fy, d, -18 - wag * 2, -9, 4, 4, INK);
	pb(cx, fy, d, -15, -20 - step, 25, 15, p->coat, p->coatDk); // chubby body
	pr(cx, fy, d, -11, -8 - step, 17, 2, p->patch);
	bool shut = p->happy > 0 || frameCount % 170 < 8;           // blinks, and smiles when petted
	drawKalabawHead(p, cx, fy, 3, -25 - step, shut, frameCount % 140 < 10);
	drawSampaguita(cx, fy, d, 3, -10 - step);
}

static void drawPetAsleep(const Pet *p, int cx, int fy) {
	int d = p->dir;
	u32 c = p->coat, dk = p->coatDk, pt = p->patch;
	rect(cx - 10, fy - 1, 20, 3, SHADOW);
	if (p->kind == PET_DOG) {
		pb(cx, fy, d, -8, -8, 15, 8, c, dk);          // curled-up body
		if (p->pattern) pr(cx, fy, d, -4, -8, 5, 3, pt);
		pr(cx, fy, d, -11, -4, 3, 2, c);              // tail
		pb(cx, fy, d, 5, -7, 7, 7, c, dk);            // head on its paws
		pr(cx, fy, d, 6, -8, 2, 4, p->ear);           // floppy ear
		pr(cx, fy, d, 9, -5, 2, 1, INK);              // closed eye
		pr(cx, fy, d, 11, -3, 2, 2, INK);             // nose
		pr(cx, fy, d, 9, -2, 3, 2, pt);               // paws
	} else {
		pb(cx, fy, d, -6, -6, 11, 6, c, dk);
		if (p->pattern == 0) {
			pr(cx, fy, d, -4, -6, 1, 3, dk);
			pr(cx, fy, d, -1, -6, 1, 3, dk);
			pr(cx, fy, d, 2, -6, 1, 3, dk);
		} else {
			pr(cx, fy, d, -2, -2, 5, 2, pt);
		}
		pr(cx, fy, d, -9, -3, 4, 2, c);               // curled tail
		pr(cx, fy, d, -9, -1, 6, 1, dk);
		pb(cx, fy, d, 4, -6, 6, 6, c, dk);            // head
		pr(cx, fy, d, 4, -9, 2, 3, c);                // ears
		pr(cx, fy, d, 8, -9, 2, 3, c);
		pr(cx, fy, d, 7, -4, 2, 1, INK);              // closed eye
		if (p->pattern) pr(cx, fy, d, 7, -2, 3, 2, pt);
	}
	int zy = fy - 20 - ((frameCount / 20) % 3) * 3;
	pixText(cx + 4, zy, 1, "Z", PLUM, 0, 0);
}

static void drawPet(const Pet *p, int cx, int fy) {
	if (p->kind == PET_KALABAW) { drawKalabaw(p, cx, fy); return; }
	if (p->state == PS_SLEEP && p->happy == 0) { drawPetAsleep(p, cx, fy); return; }

	int d = p->dir;
	bool walking = p->state == PS_WALK;
	int step = walking ? (frameCount / 6) & 1 : 0;
	int b = walking ? step : 0; // little body bounce
	int wag = (frameCount / (p->happy > 0 ? 3 : 7)) & 1;
	u32 c = p->coat, dk = p->coatDk, pt = p->patch;

	if (p->kind == PET_DOG) {
		rect(cx - 10, fy - 1, 20, 3, SHADOW);
		// legs: diagonal pairs swing when walking
		pr(cx, fy, d, -8, -5, 2, step ? 4 : 5, dk);
		pr(cx, fy, d, -5, -5, 2, step ? 5 : 4, c);
		pr(cx, fy, d, 1, -5, 2, step ? 5 : 4, dk);
		pr(cx, fy, d, 4, -5, 2, step ? 4 : 5, c);
		if (p->pattern) { // white socks
			pr(cx, fy, d, -8, -2, 2, 1, pt);
			pr(cx, fy, d, -5, -2, 2, 1, pt);
			pr(cx, fy, d, 1, -2, 2, 1, pt);
			pr(cx, fy, d, 4, -2, 2, 1, pt);
		}
		// body, belly shade, chest patch
		pr(cx, fy, d, -9, -12 - b, 15, 7, c);
		pr(cx, fy, d, -9, -6 - b, 15, 1, dk);
		pr(cx, fy, d, 3, -11 - b, 3, 5, pt);
		if (p->pattern) pr(cx, fy, d, -5, -12 - b, 5, 3, pt);
		// curly askal tail
		pr(cx, fy, d, -11, -12 - b, 2, 4, c);
		pr(cx, fy, d, -12 + wag, -16 - b, 2, 5, c);
		// collar, head, snout, nose, eye, ear
		if (p->collar) pr(cx, fy, d, 3, -12 - b, 2, 4, p->collar);
		pr(cx, fy, d, 4, -17 - b, 7, 7, c);
		pr(cx, fy, d, 10, -14 - b, 4, 4, p->pattern ? pt : c);
		pr(cx, fy, d, 12, -14 - b, 2, 2, INK);
		pr(cx, fy, d, 8, -15 - b, 1, 2, INK);
		pr(cx, fy, d, 6, -21 - b, 1, 1, p->ear);   // pointed ear
		pr(cx, fy, d, 5, -20 - b, 2, 1, p->ear);
		pr(cx, fy, d, 4, -19 - b, 3, 2, p->ear);
		if (p->happy > 0) pr(cx, fy, d, 11, -10 - b, 2, 2, ROSE); // tongue out
	} else {
		rect(cx - 8, fy - 1, 16, 3, SHADOW);
		pr(cx, fy, d, -7, -4, 2, step ? 3 : 4, dk);
		pr(cx, fy, d, -4, -4, 2, step ? 4 : 3, c);
		pr(cx, fy, d, 0, -4, 2, step ? 4 : 3, dk);
		pr(cx, fy, d, 2, -4, 2, step ? 3 : 4, c);
		if (p->pattern) { // white paws
			pr(cx, fy, d, -7, -2, 2, 1, pt);
			pr(cx, fy, d, -4, -2, 2, 1, pt);
			pr(cx, fy, d, 0, -2, 2, 1, pt);
			pr(cx, fy, d, 2, -2, 2, 1, pt);
		}
		// body
		pr(cx, fy, d, -8, -9 - b, 12, 5, c);
		if (p->pattern == 0) { // tabby stripes
			pr(cx, fy, d, -6, -9 - b, 1, 3, dk);
			pr(cx, fy, d, -3, -9 - b, 1, 3, dk);
			pr(cx, fy, d, 0, -9 - b, 1, 3, dk);
			pr(cx, fy, d, -7, -5 - b, 10, 1, pt);
		} else {
			pr(cx, fy, d, 2, -8 - b, 2, 4, pt);
			pr(cx, fy, d, -7, -5 - b, 9, 1, pt);
		}
		// tail held up high
		pr(cx, fy, d, -10, -8 - b, 2, 2, c);
		pr(cx, fy, d, -11, -14 - b, 2, 6, c);
		pr(cx, fy, d, -11 + wag, -16 - b, 2, 2, c);
		if (p->pattern == 0) pr(cx, fy, d, -11, -12 - b, 2, 1, dk);
		// head, ears, eye, nose
		pr(cx, fy, d, 2, -14 - b, 7, 7, c);
		pr(cx, fy, d, 2, -17 - b, 2, 3, c);
		pr(cx, fy, d, 7, -17 - b, 2, 3, c);
		if (p->pattern == 0) {
			pr(cx, fy, d, 3, -14 - b, 1, 2, dk);
			pr(cx, fy, d, 5, -14 - b, 1, 2, dk);
		} else {
			pr(cx, fy, d, 6, -10 - b, 3, 2, pt);
		}
		pr(cx, fy, d, 6, -12 - b, 1, 2, INK);
		pr(cx, fy, d, 8, -10 - b, 1, 1, ROSE);
	}
}

// heart and greeting shown above a pet while you pet it
static void drawPetBubble(const Pet *p, int sx, int sfeet) {
	if (p->happy <= 0) return;
	if (sx < -40 || sx > SCR_W + 40 || sfeet < 0 || sfeet > SCR_H + 60) return;
	char buf[40];
	snprintf(buf, sizeof(buf), "%s: %s", p->name, p->sound);
	int w = (int)textW(buf, 0.5f) + 12;
	int bx = sx - w / 2;
	int by = sfeet - (p->kind == PET_KALABAW ? 58 : 50); // clear of his salakot
	if (bx < 2) bx = 2;
	if (bx + w > SCR_W - 2) bx = SCR_W - 2 - w;
	if (by < 2) by = 2;
	pill(bx, by, w, 16, PAPER, PINK_DK);
	drawText(bx + 6, by + 1, 0.5f, INK, buf);
	drawHeart(sx - 3, by - 10 - ((frameCount / 10) & 1), 1, ROSE);
}

// ---------------------------------------------------------------- the street
// the three shops across the bridge: 0 = ukay-ukay, 1 = pet shop, 2 = karenderya
static void drawStreetShop(int x, int y, int v) {
	u32 wall   = v == 0 ? RGB(244, 170, 192) : v == 1 ? RGB(168, 200, 232) : RGB(238, 196, 120);
	u32 wallDk = v == 0 ? RGB(204, 124, 152) : v == 1 ? RGB(124, 158, 200) : RGB(196, 148, 84);
	u32 roof   = v == 0 ? RGB(150, 104, 160) : v == 1 ? RGB(80, 132, 120)  : RGB(176, 88, 72);
	u32 roofDk = v == 0 ? RGB(108, 70, 120)  : v == 1 ? RGB(54, 96, 88)    : RGB(128, 58, 50);
	const char *name = v == 0 ? "UKAY UKAY" : v == 1 ? "PET SHOP" : "KARENDERYA";
	u32 pal[4] = { ROSE, GOLD_UI, MINT_UI, SKY_UI };

	rect(x + 2, y + 62, 96, 4, SHADOW);

	// corrugated tin roof
	rect(x - 3, y - 4, 102, 22, roofDk);
	rect(x - 2, y - 3, 100, 19, roof);
	for (int k = 4; k < 98; k += 7) rect(x - 2 + k, y - 3, 2, 19, roofDk);

	// front wall and sign board
	rect(x + 1, y + 18, 94, 46, INK);
	rect(x + 2, y + 18, 92, 45, wall);
	rect(x + 2, y + 59, 92, 4, wallDk);
	blob(x + 10, y + 4, 76, 15, PAPER, INK);
	pixTextC(x + 48, y + 8, 1, name, ROSE_DK, 0, 0);

	// striped awning
	for (int k = 0; k < 12; k++) {
		rect(x + k * 8, y + 22, 8, 7, (k & 1) ? PAPER : roof);
		rect(x + 1 + k * 8, y + 29, 6, 2, (k & 1) ? PAPER : roof);
	}

	// door (same spot as the tile you stand in front of)
	rect(x + 49, y + 36, 15, 28, INK);
	rect(x + 50, y + 37, 13, 27, SHOP_WOOD);
	rect(x + 52, y + 40, 9, 10, SHOP_GLOW);
	winGlow(x + 52, y + 40, 9, 10);
	rect(x + 60, y + 53, 2, 2, GOLD_UI);

	// window on the left
	rect(x + 6, y + 35, 40, 23, INK);
	rect(x + 7, y + 36, 38, 21, SHOP_DARK);
	winGlow(x + 7, y + 36, 38, 21);

	if (v == 0) { // clothes hanging in the window and on a rack outside
		rect(x + 8, y + 38, 36, 1, STONE_HI);
		for (int k = 0; k < 4; k++) {
			int cx = x + 9 + k * 9;
			rect(cx + 3, y + 39, 1, 2, STONE_HI);
			rect(cx, y + 41, 7, 4, pal[k]);
			rect(cx + 1, y + 45, 5, 8, pal[k]);
		}
		rect(x + 68, y + 40, 26, 2, INK);
		rect(x + 69, y + 42, 2, 21, INK);
		rect(x + 91, y + 42, 2, 21, INK);
		for (int k = 0; k < 3; k++) {
			rect(x + 71 + k * 7, y + 43, 6, 5, pal[(k + 2) % 4]);
			rect(x + 72 + k * 7, y + 48, 4, 9, pal[(k + 2) % 4]);
		}
	} else if (v == 1) { // a puppy and a kitten in the window, paw sign and food sacks
		blob(x + 10, y + 43, 13, 13, RGB(186, 134, 82), INK);
		rect(x + 9, y + 43, 3, 7, BARK);
		rect(x + 21, y + 43, 3, 7, BARK);
		rect(x + 13, y + 48, 2, 2, INK);
		rect(x + 18, y + 48, 2, 2, INK);
		rect(x + 15, y + 51, 3, 2, INK);
		blob(x + 29, y + 45, 12, 11, RGB(238, 166, 94), INK);
		rect(x + 30, y + 42, 3, 4, RGB(238, 166, 94));
		rect(x + 37, y + 42, 3, 4, RGB(238, 166, 94));
		rect(x + 32, y + 49, 2, 2, INK);
		rect(x + 37, y + 49, 2, 2, INK);
		blob(x + 72, y + 35, 18, 16, PAPER, INK);       // paw print
		blob(x + 77, y + 42, 8, 6, ROSE_DK, ROSE_DK);
		for (int k = 0; k < 3; k++) rect(x + 75 + k * 5, y + 38 + (k == 1 ? -1 : 0), 3, 3, ROSE_DK);
		blob(x + 68, y + 52, 12, 12, RGB(190, 132, 84), INK);
		blob(x + 82, y + 52, 12, 12, MINT_UI, INK);
	} else { // big kaldero on the counter, menu board and a long bench
		for (int k = 0; k < 3; k++) {
			int cx = x + 9 + k * 12;
			blob(cx, y + 47, 11, 9, STONE_HI, INK);
			rect(cx, y + 47, 11, 2, STONE_DK);
			rect(cx + 4, y + 45, 3, 2, INK);
			int s = (frameCount / 10 + k) % 3;
			rect(cx + 3 + s, y + 38 + s, 2, 4, RGBA(255, 255, 255, 150));
		}
		rect(x + 4, y + 57, 44, 3, SHOP_WOOD_HI);
		rect(x + 69, y + 34, 22, 20, INK);              // menu board
		rect(x + 70, y + 35, 20, 18, RGB(60, 100, 76));
		for (int k = 0; k < 4; k++) rect(x + 72, y + 38 + k * 4, 10 + (k % 2) * 5, 1, PAPER);
		rect(x + 66, y + 58, 28, 3, BENCH_HI);
		rect(x + 68, y + 61, 2, 4, BENCH_DK);
		rect(x + 90, y + 61, 2, 4, BENCH_DK);
	}
}

// the waiting shed across the road
static void drawShed(int x, int y) {
	rect(x + 4, y + 30, 74, 3, SHADOW);
	rect(x + 5, y + 2, 3, 29, STONE_DK);
	rect(x + 72, y + 2, 3, 29, STONE_DK);
	rect(x + 8, y + 12, 64, 12, RGB(214, 210, 196));   // back wall
	rect(x + 8, y + 24, 64, 3, BENCH_HI);              // bench
	rect(x + 8, y + 27, 64, 2, BENCH_DK);
	rect(x, y - 6, 80, 12, INK);                       // roof
	rect(x + 1, y - 5, 78, 10, TIN_MID);
	rect(x + 1, y - 5, 78, 2, TIN_HI);
	blob(x + 2, y + 5, 76, 11, GOLD_UI, INK);
	pixTextC(x + 40, y + 7, 1, "WAITING SHED", INK, 0, 0);
}

// flat things lying on the beach: a sandcastle, a beach ball and two scuttling crabs
static void drawCrab(int x, int y, int f) {
	u32 c = RGB(228, 96, 80), c2 = RGB(250, 140, 118);
	rect(x + 1, y + 7, 9, 1, SHADOW);
	rect(x + 2, y + 2, 7, 5, c);
	rect(x + 3, y + 2, 5, 1, c2);
	rect(x + 3, y, 1, 2, INK);
	rect(x + 7, y, 1, 2, INK);
	rect(x - 1, y + 1 - f, 3, 3, c);   // claws
	rect(x + 9, y + 1 - (1 - f), 3, 3, c);
	rect(x + 1, y + 6 + f, 1, 2, c);   // legs
	rect(x + 4, y + 7 - f, 1, 1, c);
	rect(x + 6, y + 7 - f, 1, 1, c);
	rect(x + 8, y + 6 + f, 1, 2, c);
}

static void drawBeachDecor(int camx, int camy) {
	if (camy + SCR_H < BEACH_Y0 * TS) return; // beach not on screen
	int x = 21 * TS - camx, y = 66 * TS - camy; // sandcastle
	if (x > -40 && x < SCR_W + 10) {
		rect(x - 1, y + 14, 30, 3, SHADOW);
		blob(x, y + 6, 28, 10, SAND_MID, SAND_DK);
		blob(x, y + 2, 8, 14, SAND_HI, SAND_DK);
		blob(x + 20, y + 2, 8, 14, SAND_HI, SAND_DK);
		blob(x + 8, y - 2, 12, 18, SAND_HI, SAND_DK);
		rect(x + 12, y + 10, 4, 6, SAND_DK);
		rect(x + 13, y - 10, 1, 9, INK_SOFT);
		rect(x + 14, y - 10, 5, 3, ROSE);
	}
	x = 45 * TS + 2 - camx; y = 67 * TS + 3 - camy; // beach ball
	if (x > -20 && x < SCR_W + 10) {
		rect(x, y + 9, 12, 3, SHADOW);
		blob(x, y, 11, 11, WHITE, INK);
		rect(x + 1, y + 4, 9, 2, ROSE);
		rect(x + 4, y + 1, 2, 9, SKY_UI);
		rect(x + 4, y + 4, 2, 2, GOLD_UI);
	}
	static const int CRABS[2][3] = { { 27, 67, 0 }, { 48, 64, 17 } }; // tile x, tile y, phase
	for (int i = 0; i < 2; i++) {
		int p = (frameCount / 3 + CRABS[i][2] * 5) % 120;
		int off = p < 60 ? p : 120 - p; // scuttles back and forth
		int cx = CRABS[i][0] * TS + off - camx, cy = CRABS[i][1] * TS - camy;
		if (cx > -20 && cx < SCR_W + 10 && cy > -20 && cy < SCR_H + 10)
			drawCrab(cx, cy, (frameCount / 6) & 1);
	}
}

// concrete electric post with a crossarm
static void drawElectricPost(int sx, int sy) {
	rect(sx + 4, sy + 13, 9, 3, SHADOW);
	rect(sx + 6, sy - 34, 4, 48, STONE_MID);
	rect(sx + 6, sy - 34, 1, 48, STONE_HI);
	rect(sx - 1, sy - 32, 18, 2, BARK_DK);
	rect(sx + 1, sy - 27, 14, 2, BARK_DK);
	rect(sx + 3, sy - 22, 9, 7, STONE_DK);             // transformer
}

// the famous tangle of wires between the posts
static void drawWires(int camx, int camy) {
	int base = (SHED_TY + 1) * TS - camy;
	if (base < -4 || base > SCR_H + 40) return;
	u32 c = RGBA(36, 28, 34, 150);
	for (int a = 4; a + 9 < MAP_W - 3; a += 9) {
		int x0 = a * TS + 8 - camx, len = 9 * TS;
		if (x0 > SCR_W || x0 + len < 0) continue;
		for (int w = 0; w < 2; w++) {
			int top = base - 32 + w * 5, sag = 3 + ((a + w) % 3) * 2;
			for (int k = 0; k < 6; k++) {
				int dip = sag * (k < 3 ? k : 5 - k) / 2;
				rect(x0 + k * len / 6, top + dip, len / 6, 1, c);
			}
		}
	}
}

// rectangle on a vehicle; dx is measured from its back end, so it mirrors when driving west
static void vr(int x, int w0, int dir, int dx, int y, int w, int h, u32 c) {
	rect(dir > 0 ? x + dx : x + w0 - dx - w, y, w, h, c);
}

static void drawWheel(int x, int y) {
	blob(x, y - 9, 10, 10, INK, INK);
	rect(x + 3, y - 6, 4, 4, STONE_HI);
}

static void drawVehicle(const Vehicle *v, int x, int fy) {
	int w = v->kind == VEH_JEEP ? 66 : 34, d = v->dir; // size of the art, before scaling
	if (v->kind == VEH_JEEP) bigBegin(x, fy);          // grow the jeepney around its back-bottom corner
	u32 chrome = RGB(214, 220, 226), glass = RGB(184, 220, 232);
	rect(x + 2, fy - 2, w - 4, 4, SHADOW);
	if (v->kind == VEH_JEEP) {
		// long passenger cab with open windows
		vr(x, w, d, 0, fy - 31, 50, 25, INK);
		vr(x, w, d, 1, fy - 30, 48, 23, v->body);
		vr(x, w, d, 0, fy - 33, 52, 4, chrome);        // roof
		vr(x, w, d, 4, fy - 36, 40, 3, v->trim);       // roof rack
		for (int k = 0; k < 4; k++) vr(x, w, d, 4 + k * 11, fy - 27, 9, 9, SHOP_DARK);
		for (int k = 0; k < 4; k += 2) vr(x, w, d, 6 + k * 11, fy - 24, 5, 6, SKIN_SHD); // riders
		vr(x, w, d, 1, fy - 16, 48, 3, v->trim);       // painted stripes
		// driver's window and the long chrome hood
		vr(x, w, d, 49, fy - 28, 7, 12, INK);
		vr(x, w, d, 49, fy - 27, 6, 10, glass);
		vr(x, w, d, 49, fy - 18, 17, 12, INK);
		vr(x, w, d, 49, fy - 17, 16, 10, chrome);
		vr(x, w, d, 49, fy - 14, 16, 2, v->body);
		vr(x, w, d, 63, fy - 16, 2, 3, GOLD_UI);       // headlight
		vr(x, w, d, 59, fy - 22, 2, 4, chrome);        // hood ornament
		drawWheel(x + (d > 0 ? 7 : w - 17), fy);
		drawWheel(x + (d > 0 ? 49 : w - 59), fy);
		rect(x + (d > 0 ? 8 : w - 42), fy - 16, 34, 9, INK);          // name plate
		pixTextC(x + (d > 0 ? 25 : w - 25), fy - 15, 1, "MAHAL", GOLD_UI, 0, 0);
	} else {
		// tricycle: roofed sidecar in front of you, motorcycle with a driver
		vr(x, w, d, 20, fy - 24, 5, 6, SKIN);          // driver's head
		vr(x, w, d, 19, fy - 26, 7, 3, INK);           // helmet
		vr(x, w, d, 19, fy - 18, 7, 8, v->trim);       // shirt
		vr(x, w, d, 18, fy - 10, 14, 4, STONE_DK);     // motorbike
		vr(x, w, d, 28, fy - 17, 2, 8, INK);           // handlebar
		vr(x, w, d, 1, fy - 22, 17, 17, INK);          // sidecar
		vr(x, w, d, 2, fy - 21, 15, 15, v->body);
		vr(x, w, d, 5, fy - 19, 9, 7, glass);
		vr(x, w, d, 2, fy - 10, 15, 2, v->trim);
		vr(x, w, d, 0, fy - 28, 29, 3, v->body);       // roof
		vr(x, w, d, 0, fy - 25, 29, 1, INK);
		drawWheel(x + (d > 0 ? 4 : w - 14), fy);
		drawWheel(x + (d > 0 ? 24 : w - 34), fy);
	}
	bigEnd();
}

// Mang Taho carries two silver buckets on a bamboo pole across his shoulders and calls out
// "Taho!" in a thought bubble every few seconds.
static void drawTahoVendor(int cx, int feet, bool walking) {
	int bob = walking ? ((frameCount / 8) & 1) : 0;
	int py0 = feet - 25 + bob;
	rect(cx - 17, py0, 35, 2, BAMBOO_DK);                  // the pole
	rect(cx - 17, py0, 35, 1, BAMBOO);
	for (int side = 0; side < 2; side++) {
		int bx = side ? cx + 12 : cx - 19;                 // left edge of this bucket
		rect(bx + 3, py0 + 2, 1, 4, INK_SOFT);             // ropes
		rect(bx + 3, py0 + 2, 1, 1, INK_SOFT);
		rect(bx + 1, py0 + 6, 9, 11, INK);                 // bucket outline
		rect(bx + 2, py0 + 7, 7, 9, TIN_MID);
		rect(bx + 2, py0 + 7, 2, 9, TIN_HI);
		rect(bx + 2, py0 + 12, 7, 1, TIN_DK);
		rect(bx, py0 + 5, 11, 2, TIN_DK);                  // lid
		rect(bx + 4, py0 + 3, 3, 2, STONE_HI);             // lid knob
	}
}

static void drawTahoBubble(int cx, int feet) {
	int bob = (frameCount / 20) & 1;
	int w = 38, h = 17;
	int x = cx - w / 2, y = feet - 40 - 25 - bob;
	if (x < 2) x = 2;
	if (x + w > SCR_W - 2) x = SCR_W - 2 - w;
	blob(x, y, w, h, WHITE, INK);                          // thought cloud
	rect(x + 3, y - 1, 8, 2, INK);                         // little bumps on top
	rect(x + 4, y - 1, 6, 1, WHITE);
	rect(x + 17, y - 2, 9, 2, INK);
	rect(x + 18, y - 2, 7, 1, WHITE);
	pixTextC(x + w / 2, y + 5, 1, "TAHO!", ROSE_DK, 0, 0);
	blob(cx - 1, feet - 40 - 6 - bob, 5, 5, WHITE, INK);   // trail of small bubbles
	blob(cx - 3, feet - 40 - 0 - bob, 3, 3, WHITE, INK);
}

// a villager: borrows the journal settings for a moment so it can reuse your sprite code
static void drawWalker(int i, int sx, int sfeet) {
	const Walker *w = &walkers[i];
	const WalkerInfo *info = &WALKER_INFO[i];
	int dir = dirFromFacing(w->fx, w->fy);
	bool walking = w->walking && !dialogActive;
	if (info->jayLook) {
		jayVariant = info->jayLook;
		drawJaySprite(sx, sfeet, dir, walking);
		jayVariant = 0;
		return;
	}
	int myStyle = styleIndex, myAcc = accIndex, mySkirt = skirtIndex, myShoes = shoeIndex;
	styleIndex = info->style; accIndex = info->acc;
	skirtIndex = info->skirt; shoeIndex = info->shoe;
	drawPlayerSprite(sx, sfeet, dir, walking,
	                 skinTones[info->skin], hairColors[info->hair], outfitColors[info->top]);
	drawAccessory(sx, sfeet, dir);
	styleIndex = myStyle; accIndex = myAcc; skirtIndex = mySkirt; shoeIndex = myShoes;
}

// ---------------------------------------------------------------- trash bins and street games
// painted drum bins for waste segregation: green = nabubulok, blue = di-nabubulok
static void drawTrashBin(int sx, int sy, int tx) {
	bool bio = (tx & 1) == 0;
	u32 c = bio ? RGB(78, 146, 90) : RGB(72, 118, 180);
	u32 dk = bio ? RGB(52, 104, 66) : RGB(48, 82, 134);
	rect(sx + 2, sy + 13, 13, 3, SHADOW);
	rect(sx + 2, sy - 1, 12, 16, INK);
	rect(sx + 3, sy, 10, 14, c);
	rect(sx + 3, sy + 3, 10, 1, dk);
	rect(sx + 3, sy + 11, 10, 1, dk);
	rect(sx + 1, sy - 4, 14, 4, INK);          // lid
	rect(sx + 2, sy - 3, 12, 2, dk);
	rect(sx + 6, sy - 5, 4, 1, INK);
	rect(sx + 5, sy + 5, 6, 5, PAPER);         // label
	rect(sx + 7, sy + 6, 2, 3, c);
}

// a small child, about two thirds of your height; (x, feet) = centre of the feet
static void drawKid(const Kid *k, int x, int feet) {
	static const u8 SHIRT[6][3] = {
		{ 232, 84, 80 }, { 248, 196, 70 }, { 84, 142, 214 },
		{ 242, 136, 178 }, { 96, 176, 120 }, { 244, 244, 236 }
	};
	u32 shirt = RGB(SHIRT[k->look % 6][0], SHIRT[k->look % 6][1], SHIRT[k->look % 6][2]);
	u32 shorts = (k->look & 1) ? RGB(74, 86, 130) : RGB(120, 88, 70);
	u32 skin = skinTones[1 + k->look % 3];
	u32 hair = (k->look % 4 == 2) ? RGB(102, 70, 56) : RGB(44, 32, 38);
	bool girl = k->look == 3 || k->look == 4 || k->look == 7 || k->look == 10;
	int step = k->moving ? ((playClock / 6) & 1) : 0;

	rect(x - 6, feet - 2, 12, 3, SHADOW);      // the shadow stays on the ground
	int y = feet - k->lift;
	// legs and tsinelas
	rect(x - 4, y - 7, 3, 6 - step, skin);
	rect(x + 1, y - 7, 3, 5 + step, skin);
	rect(x - 5, y - 1 - step, 4, 2, TIN_DK);
	rect(x + 1, y - 2 + step, 4, 2, TIN_DK);
	// shorts and shirt
	rect(x - 5, y - 10, 10, 4, shorts);
	rect(x - 5, y - 17, 10, 7, shirt);
	if (k->armsOut) {
		rect(x - 11, y - 16, 6, 3, skin);
		rect(x + 5, y - 16, 6, 3, skin);
	} else {
		rect(x - 7, y - 16, 2, 7, skin);
		rect(x + 5, y - 16, 2, 7, skin);
	}
	// big head
	rect(x - 7, y - 29, 14, 13, INK);
	rect(x - 6, y - 28, 12, 11, skin);
	rect(x - 6, y - 29, 12, 5, hair);
	rect(x - 7, y - 27, 2, girl ? 11 : 5, hair);
	rect(x + 5, y - 27, 2, girl ? 11 : 5, hair);
	if (girl) {                                // pigtails
		rect(x - 9, y - 25, 3, 5, hair);
		rect(x + 6, y - 25, 3, 5, hair);
		rect(x - 8, y - 26, 2, 2, ROSE);
		rect(x + 6, y - 26, 2, 2, ROSE);
	}
	int e = k->dir * 2;
	rect(x - 3 + e, y - 23, 2, 2, INK);
	rect(x + 2 + e, y - 23, 2, 2, INK);
	rect(x - 1 + e, y - 20, 3, 1, ROSE_DK);
}

// Three carolers in Santa hats: a tansan tambourine, a lead singer and a tin-can drum.
static void drawCarolers(int group, int x, int feet) {
	bool singing = !dialogActive;
	for (int k = 0; k < CAROL_SIZE; k++) {
		Kid c;
		memset(&c, 0, sizeof(c));
		c.look = (group * CAROL_SIZE + k) % 12;
		c.lift = singing && ((frameCount / 16 + k + group) & 1) ? 1 : 0; // bobbing to the beat
		int cx = x + (k - 1) * CAROL_GAP, hy = feet - c.lift;
		drawKid(&c, cx, feet);
		if (singing && (frameCount / 12 + k) % 3 != 0) {  // open mouth
			rect(cx - 1, hy - 21, 3, 3, ROSE_DK);
		}
		rect(cx - 5, hy - 33, 10, 4, RGB(214, 52, 68));   // small Santa hat
		rect(cx - 2, hy - 35, 7, 2, RGB(214, 52, 68));
		rect(cx - 7, hy - 29, 14, 2, WHITE);
		rect(cx + 4, hy - 36, 3, 3, WHITE);
	}
	int shake = singing ? (frameCount / 6) & 1 : 0;
	int lx = x - CAROL_GAP - 6 + shake;                  // tambourine of flattened bottle caps
	blob(lx - 4, feet - 17, 9, 9, GOLD_UI, INK);
	rect(lx - 2, feet - 15, 5, 5, PAPER);
	int dx = x + CAROL_GAP;                              // tin-can drum
	blob(dx - 5, feet - 13, 11, 9, STONE_HI, INK);
	rect(dx - 4, feet - 10, 9, 3, ROSE);
	rect(dx - 8 + shake * 2, feet - 16 + shake * 2, 5, 2, SHOP_WOOD_DK); // stick
	if (!singing) return;
	for (int k = 0; k < 2; k++) { // music notes float up
		int t = (frameCount + k * 45 + group * 20) % 90;
		int nx = x - 14 + k * 26 + ((t / 15) & 1) * 2, ny = feet - 38 - t / 5;
		int a = t < 60 ? 255 : (90 - t) * 255 / 30;
		u32 col = withAlpha(k ? PLUM : ROSE_DK, a);
		rect(nx, ny + 5, 4, 3, col);
		rect(nx + 3, ny - 2, 1, 9, col);
		rect(nx + 4, ny - 2, 3, 2, col);
	}
}

// "NAMAMASKO PO!" pops up now and then; drawn on top so trees and lamps do not hide it
static void drawCarolBubbles(int camx, int camy) {
	if (activeEvent != EV_PASKO || dialogActive || tahoMenu) return;
	for (int i = 0; i < CAROL_COUNT; i++) {
		if (i == nearCaroler() || (frameCount + i * 67) % 300 >= 150) continue; // "A TALK" goes there
		int gx, gy;
		carolSpot(i, &gx, &gy);
		int x = gx - camx, y = gy - camy - 62;
		if (x < -50 || x > SCR_W + 50 || y < -20 || y > SCR_H) continue;
		pill(x - 45, y, 90, 15, PAPER, PINK_DK);
		pixTextC(x, y + 4, 1, "NAMAMASKO PO!", ROSE_DK, 0, 0);
	}
}

static void chalkBox(int x, int y, int w, int h, u32 c) {
	rect(x, y, w, 1, c);
	rect(x, y + h - 1, w, 1, c);
	rect(x, y, 1, h, c);
	rect(x + w - 1, y, 1, h, c);
}

static bool areaVisible(int x, int y, int w, int h) {
	return x < SCR_W && x + w > 0 && y < SCR_H && y + h > 0;
}

// dirt patches and chalk lines, drawn flat on the ground under everyone
static void drawPlayAreas(int camx, int camy) {
	u32 dirt = RGB(196, 170, 128), chalk = RGB(250, 248, 236);

	int x = PRESO_X - camx, y = PRESO_Y - camy;
	if (areaVisible(x, y, 128, 66)) {
		rect(x + 2, y + 12, 124, 52, dirt);
		rect(x + 32, y + 14, 2, 48, chalk);               // throwing line
		int c = playClock % PRESO_CYCLE;
		if (kidWalked(2) > 0.0f) c = 0;                   // no taya: the can just stands
		int cx = x + 86, cy = y + 40;
		chalkBox(cx - 7, cy - 4, 16, 10, chalk);          // the can's spot
		if (c >= PRESO_THROW + PRESO_FLY && c < PRESO_UP) { // knocked over
			rect(cx + 4, cy - 3, 11, 7, INK);
			rect(cx + 5, cy - 2, 9, 5, RGB(212, 70, 66));
			rect(cx + 13, cy - 2, 2, 5, STONE_HI);
		} else {                                          // standing
			rect(cx - 3, cy - 10, 8, 12, INK);
			rect(cx - 2, cy - 9, 6, 10, RGB(212, 70, 66));
			rect(cx - 2, cy - 9, 6, 2, STONE_HI);
			rect(cx - 2, cy - 4, 6, 2, PAPER);
		}
	}

	x = PIKO_X - camx; y = PIKO_Y - camy;
	if (areaVisible(x - 20, y, 40, 60)) {
		rect(x - 18, y - 2, 36, 62, dirt);
		for (int i = 0; i < 5; i++) {
			int by = y + 4 + i * 11;
			if (i == 1 || i == 3) {                       // side-by-side boxes
				chalkBox(x - 14, by, 15, 11, chalk);
				chalkBox(x, by, 15, 11, chalk);
			} else chalkBox(x - 7, by, 15, 11, chalk);
		}
		rect(x + 3, y + 29, 4, 3, BARK);                  // the pamato
	}

	x = GARTER_X - camx; y = GARTER_Y - camy;
	if (areaVisible(x, y - 10, 64, 20)) rect(x + 2, y - 8, 60, 16, dirt);

	x = PAT_X - camx; y = PAT_Y - camy;
	if (areaVisible(x - 6, y - 4, PAT_W + 12, PAT_H + 14)) {
		rect(x - 4, y - 2, PAT_W + 8, PAT_H + 10, dirt);
		chalkBox(x, y + 4, PAT_W, PAT_H, chalk);
		rect(x + PAT_W / 2, y + 4, 1, PAT_H, chalk);
		rect(x, y + 4 + PAT_H / 3, PAT_W, 1, chalk);
		rect(x, y + 4 + PAT_H * 2 / 3, PAT_W, 1, chalk);
	}
}

// the flying tsinelas and the stretched garter, drawn over the kids
static void drawPlayAir(int camx, int camy) {
	int c = playClock % PRESO_CYCLE;
	int x = PRESO_X - camx, y = PRESO_Y - camy;
	if (kidWalked(0) <= 0.0f && areaVisible(x, y - 20, 140, 90)) { // the tsinelas goes home with its kid
		int sx, sy;
		if (c >= PRESO_THROW && c < PRESO_THROW + PRESO_FLY) { // flying at the can
			int t = c - PRESO_THROW;
			sx = x + 22 + 64 * t / PRESO_FLY;
			sy = y + 20 + 12 * t / PRESO_FLY - hopHeight(t, PRESO_FLY, 20);
		} else if (c >= PRESO_THROW + PRESO_FLY && c < PRESO_UP) { // lying past the can
			sx = x + 100; sy = y + 54;
		} else {                                          // back in the thrower's hand
			sx = x + 23; sy = y + 22;
		}
		rect(sx, sy, 8, 4, INK);
		rect(sx + 1, sy + 1, 6, 2, SKY_UI);
	}

	x = GARTER_X - camx; y = GARTER_Y - camy;
	if (kidWalked(5) <= 0.0f && kidWalked(6) <= 0.0f && areaVisible(x, y - 30, 64, 40)) { // no garter without its holders
		static const int LEVEL[4] = { 3, 8, 13, 19 };     // ankle, knee, waist, chest
		int h = LEVEL[(playClock / 480) % 4];
		rect(x + 12, y - h - 1, 40, 1, RGB(236, 80, 140));
		rect(x + 12, y - h + 2, 40, 1, RGB(236, 80, 140));
	}
}

// ---------------------------------------------------------------- umbrellas
// Jay and the villagers who walk outside open an umbrella while it rains.
// (cx, feet) = centre of the feet, same as the sprites; pick = which umbrella colour.
static void drawUmbrella(int cx, int feet, int pick) {
	const u32 UMB[6][2] = {   // not static: C2D_Color32 needs runtime initialization
		{ ROSE, ROSE_DK }, { SKY_UI, TIN_DK }, { GOLD_UI, THATCH_DK },
		{ MINT_UI, LEAF_DK }, { BLOSSOM_MID, BLOSSOM_DK }, { PINK_LT, PLUM }
	};
	u32 body = UMB[pick % 6][0], dark = UMB[pick % 6][1];
	static const int RW[4] = { 13, 21, 27, 31 };   // dome rows, top to bottom
	int top = feet - 54;

	// handle, held at the side (drawn first so the canopy covers its top)
	rect(cx + 7, feet - 47, 4, 26, INK);
	rect(cx + 8, feet - 46, 2, 24, WOOD_DK);
	rect(cx + 7, feet - 25, 4, 4, SKIN);

	// outline, then the dome, then the scalloped edge
	for (int r = 0; r < 4; r++) rect(cx - RW[r] / 2 - 1, top + r * 2 - 1, RW[r] + 2, 4, INK);
	rect(cx - 1, top - 4, 2, 4, INK);                       // tip
	for (int r = 0; r < 4; r++) rect(cx - RW[r] / 2, top + r * 2, RW[r], 2, body);
	rect(cx - 16, top + 8, 32, 3, INK);
	for (int sx = -15; sx < 15; sx += 6) {                 // scallops
		rect(cx + sx, top + 8, 5, 2, dark);
		rect(cx + sx + 1, top + 8, 3, 1, body);
	}
	rect(cx - 1, top - 3, 2, 3, dark);
	rect(cx - 1, top + 1, 2, 7, dark);                      // centre rib
	rect(cx - 8, top + 3, 1, 5, dark);
	rect(cx + 7, top + 3, 1, 5, dark);
}

typedef struct { int feet, kind, idx; } DrawEnt; // kind: 0 = Jay, 1 = player, 2 = pet, 3 = vehicle, 4 = villager, 5 = kid, 6 = event centrepiece, 7 = carolers

static void drawEnt(const DrawEnt *e, int jaySx, int jayFeet, bool jayWalk,
					int pcxs, int pfeet, int camx) {
	if (e->kind == 0) {
		drawJayEntity(jaySx, jayFeet, jayWalk);
		drawEventHat(jaySx, jayFeet);
		if (rainLevel > 0.4f) drawUmbrella(jaySx, jayFeet, 5);
	}
	else if (e->kind == 1) drawPlayerEntity(pcxs, pfeet);
	else if (e->kind == 7) {
		int gx, gy;
		carolSpot(e->idx, &gx, &gy);
		drawCarolers(e->idx, gx - camx, e->feet);
	}
	else if (e->kind == 6) // 8 px up: the middle of the water in the bigger basin
		drawCenterpiece(FOUNTAIN_TX * TS - camx, e->feet - 3 * TS - 8);
	else if (e->kind == 2) drawPet(&pets[e->idx], (int)pets[e->idx].x - camx, e->feet);
	else if (e->kind == 3) drawVehicle(&vehicles[e->idx], (int)vehicles[e->idx].x - camx, e->feet);
	else if (e->kind == 4) {
		drawWalker(e->idx, (int)walkers[e->idx].x - camx, e->feet);
		if (WALKER_INFO[e->idx].jayLook || WALKER_INFO[e->idx].acc != 4)
			drawEventHat((int)walkers[e->idx].x - camx, e->feet);
		if (e->idx == TAHO_WALKER) {
			drawTahoVendor((int)walkers[e->idx].x - camx, e->feet, walkers[e->idx].walking && !dialogActive);
			bool chatting = (dialogActive && talkingWalker == TAHO_WALKER) || tahoMenu;
			if (!chatting && (frameCount + 90) % 270 < 130) drawTahoBubble((int)walkers[e->idx].x - camx, e->feet);
		}
		if (rainLevel > 0.4f) drawUmbrella((int)walkers[e->idx].x - camx, e->feet, e->idx);
	}
	else {
		Kid k = kidAt(e->idx);
		drawKid(&k, (int)k.x - camx, e->feet);
	}
}

// Sunrise / day / sunset / night tint over the whole village, glowing lamps and fireflies.
static void drawDayNight(void) {
	int r, g, b, a;
	skyTint(dayTime, &r, &g, &b, &a);
	if (a > 0) rect(0, 0, SCR_W, SCR_H, RGBA(r, g, b, a));

	float n = nightLevel();
	if (n > 0.02f) {
		for (int i = 0; i < glowN; i++) { // lit lamps: a soft halo and a pool of light on the ground
			int cx = glowX[i], cy = glowY[i];
			int a1 = (int)(n * 34), a2 = (int)(n * 52), a3 = (int)(n * 84);
			rect(cx - 28, cy - 22, 56, 44, RGBA(255, 214, 130, a1));
			rect(cx - 19, cy - 15, 38, 30, RGBA(255, 214, 130, a2));
			rect(cx - 10, cy - 8, 20, 16, RGBA(255, 226, 160, a3));
			rect(cx - 24, glowGround[i] - 4, 48, 12, RGBA(255, 214, 130, (int)(n * 40)));
			rect(cx - 14, glowGround[i] - 2, 28, 8, RGBA(255, 226, 160, (int)(n * 44)));
		}
		for (int i = 0; i < evGlowN; i++) { // parols and jack-o'-lanterns
			int ex = evGlow[i][0], ey = evGlow[i][1], ew = evGlow[i][2], eh = evGlow[i][3];
			rect(ex - 3, ey - 3, ew + 6, eh + 6, RGBA(255, 190, 90, (int)(n * 44)));
			rect(ex, ey, ew, eh, RGBA(255, 234, 140, (int)(n * 220)));
		}
		for (int i = 0; i < winN; i++) { // lit windows: a warm halo and a bright pane
			int wx = winR[i][0], wy = winR[i][1], ww = winR[i][2], wh = winR[i][3];
			rect(wx - 3, wy - 3, ww + 6, wh + 6, RGBA(255, 200, 110, (int)(n * 36)));
			rect(wx, wy, ww, wh, RGBA(255, 222, 150, (int)(n * 120)));
		}
		if (n > 0.4f && rainLevel < 0.5f) { // fireflies drift around and blink
			for (int i = 0; i < 12; i++) {
				u32 h = hash2(i, 123);
				float fx = (float)(h % (u32)SCR_W) + 14.0f * sinf(frameCount * 0.013f + (float)(h & 63));
				float fy = (float)(40 + (h >> 9) % (u32)(SCR_H - 60)) + 10.0f * sinf(frameCount * 0.017f + (float)((h >> 5) & 63));
				int t = (frameCount + (int)(h % 120)) % 120;
				int tri = t < 60 ? t : 120 - t;               // 0..60..0
				int al = (int)(tri * 4 * (n - 0.4f) / 0.6f);
				if (al > 230) al = 230;
				rect((int)fx - 2, (int)fy - 2, 5, 5, RGBA(200, 255, 140, al / 5));
				rect((int)fx - 1, (int)fy - 1, 2, 2, RGBA(235, 255, 170, al));
			}
		}
	}
	glowN = 0; // start collecting again for the next frame
	winN = 0;
	evGlowN = 0;
}

// Rain tint, falling drops and ripples, or a warm sunny glow with light shafts.
#define RAIN_DROPS 70
static void drawWeather(void) {
	float dry = (1.0f - rainLevel) * (1.0f - nightLevel()); // no sunshine at night
	if (rainLevel > 0.01f) {
		rect(0, 0, SCR_W, SCR_H, RGBA(68, 92, 128, (int)(rainLevel * 56)));
		int n = (int)(rainLevel * RAIN_DROPS);
		for (int i = 0; i < n; i++) {
			u32 h = hash2(i, 91);
			int sp = 6 + (int)(h % 3);
			int span = SCR_W + 40;
			int x = ((int)((h >> 4) % (u32)span) - frameCount * sp / 6) % span;
			if (x < 0) x += span;
			int y = ((int)((h >> 14) % (u32)(SCR_H + 30)) + frameCount * sp) % (SCR_H + 30) - 15;
			rect(x - 20, y, 1, 6, RGBA(220, 236, 255, 150));
		}
		int rn = (int)(rainLevel * 8);
		for (int i = 0; i < rn; i++) { // ripples on the ground
			const int cycle = 36;
			u32 h = hash2(i, 55);
			int t = (frameCount + (int)(h % cycle)) % cycle;
			int round = (frameCount + (int)(h % cycle)) / cycle;
			u32 q = hash2(i * 31 + round, 77);
			int x = 20 + (int)(q % (u32)(SCR_W - 40));
			int y = 60 + (int)((q >> 10) % (u32)(SCR_H - 90));
			int r = 1 + t / 6;
			u32 col = RGBA(230, 244, 255, 150 - t * 4);
			rect(x - r, y, 2 * r, 1, col);
			rect(x - r, y + 2, 2 * r, 1, col);
			rect(x - r - 1, y + 1, 1, 1, col);
			rect(x + r, y + 1, 1, 1, col);
		}
	}
	if (dry > 0.01f) {
		rect(0, 0, SCR_W, SCR_H, RGBA(255, 228, 160, (int)(dry * 12)));
		for (int k = 0; k < 3; k++) { // slanted light shafts that gently pulse
			int tri = abs((frameCount / 2 + k * 60) % 240 - 120);
			int a = (int)((6 + tri / 20) * dry);
			u32 col = RGBA(255, 244, 200, a);
			for (int j = 0; j < 14; j++)
				rect(90 + k * 120 + j * 8, j * 16, 26, 16, col);
		}
		for (int i = 0; i < 8; i++) { // twinkles
			const int period = 90;
			u32 h = hash2(i, 33);
			int t = (frameCount + (int)(h % period)) % period;
			if (t >= 24) continue;
			int round = (frameCount + (int)(h % period)) / period;
			u32 q = hash2(i * 17 + round, 44);
			int x = 12 + (int)(q % (u32)(SCR_W - 24));
			int y = 12 + (int)((q >> 10) % (u32)(SCR_H - 24));
			int a = (int)((t < 12 ? t : 24 - t) * 16 * dry);
			u32 col = RGBA(255, 246, 190, a);
			rect(x - 1, y, 3, 1, col);
			rect(x, y - 1, 1, 3, col);
		}
	}
}

// bottom-screen button that shows the weather; tap it to switch
static void drawWeatherBtn(void) {
	bool rainy = weather == W_RAINY;
	pill(WX_X, WX_Y, WX_W, WX_H, rainy ? SKY_UI : GOLD_UI, PINK_DK);
	int ix = WX_X + 8, iy = WX_Y + 6;
	if (rainy) {
		blob(ix, iy, 18, 9, PAPER, PLUM);                  // cloud
		int s = (frameCount / 6) % 3;
		rect(ix + 3, iy + 11 + s, 2, 4, WATER_HI);         // falling drops
		rect(ix + 8, iy + 12 + (s + 1) % 3, 2, 4, WATER_HI);
		rect(ix + 13, iy + 11 + (s + 2) % 3, 2, 4, WATER_HI);
	} else {
		blob(ix + 4, iy + 4, 10, 10, PAPER, PLUM);         // sun
		rect(ix + 8, iy, 2, 3, PAPER);
		rect(ix + 8, iy + 15, 2, 3, PAPER);
		rect(ix, iy + 8, 3, 2, PAPER);
		rect(ix + 15, iy + 8, 3, 2, PAPER);
	}
	pixTextC(WX_X + 64, WX_Y + 8, 2, rainy ? "RAINY" : "SUNNY", WHITE, 0, PLUM);
}

static void drawTahoMenu(void);

static void drawWorld(void) {
	float pcx = px + PLAYER_SIZE / 2.0f;
	float pcy = py + PLAYER_SIZE / 2.0f;
	int camx = (int)(pcx - SCR_W / 2);
	int camy = (int)(pcy - SCR_H / 2);
	if (camx < 0) camx = 0;
	if (camy < 0) camy = 0;
	if (camx > MAP_W * TS - SCR_W) camx = MAP_W * TS - SCR_W;
	if (camy > MAP_H * TS - SCR_H) camy = MAP_H * TS - SCR_H;

	evGlowN = 0; // forget lanterns drawn indoors or on the title screen
	int tx0 = camx / TS, ty0 = camy / TS;
	for (int ty = ty0; ty <= ty0 + SCR_H / TS + 1 && ty < MAP_H; ty++)
		for (int tx = tx0; tx <= tx0 + SCR_W / TS + 1 && tx < MAP_W; tx++)
			drawTile(map[ty][tx], tx * TS - camx, ty * TS - camy, tx, ty);

	// Only draw a building when it is actually on screen (saves citro2d shapes). Buildings are
	// drawn 1.5x bigger around the bottom of their door tile (see bigBegin), so they reach about
	// 2 tiles further left, 4 further right and 3-5 tiles higher than their tile position.
#define BLD_ONSCREEN(bx, by) ((bx) > -150 && (bx) < SCR_W + 40 && (by) > -90 && (by) < SCR_H + 100)
	int cbx = COTTAGE_TX * TS - camx, cby = COTTAGE_TY * TS - camy;
	if (BLD_ONSCREEN(cbx, cby)) { bigBegin(cbx + 64, cby + 64); drawCottage(cbx, cby); bigEnd(); }
	if (BLD_ONSCREEN(cbx, cby)) drawHouseDecor(cbx + 52, cby + 64, 82, 56, 68, 0);
	int bbx = BAK_TX * TS - camx, bby = BAK_TY * TS - camy;
	if (BLD_ONSCREEN(bbx, bby)) { bigBegin(bbx + 64, bby + 64); drawBakery(bbx, bby); bigEnd(); }
	if (BLD_ONSCREEN(bbx, bby)) drawHouseDecor(bbx + 52, bby + 64, 82, 56, 62, 1);
	int sbx = SARI_TX * TS - camx, sby = SARI_TY * TS - camy;
	if (BLD_ONSCREEN(sbx, sby)) { bigBegin(sbx + 64, sby + 64); drawSariStore(sbx, sby); bigEnd(); }
	if (BLD_ONSCREEN(sbx, sby)) drawHouseDecor(sbx + 52, sby + 64, 82, 56, 72, 2);
	int fbx = FOUNTAIN_TX * TS - camx, fby = FOUNTAIN_TY * TS - camy;
	if (fbx > -90 && fbx < SCR_W + 26 && fby > -52 && fby < SCR_H + 36) {
		bigBegin(fbx + 32, fby + 48);
		drawFountain(fbx, fby);
		bigEnd();
	}
	for (int i = 0; i < STALL_COUNT; i++) {
		int stx = STALLS[i][0] * TS - camx, sty = STALLS[i][1] * TS - camy;
		if (stx > -40 && stx < SCR_W + 10 && sty > -20 && sty < SCR_H + 10) drawStall(stx, sty, i);
	}
	int hux = HUT_TX * TS - camx, huy = HUT_TY * TS - camy;
	if (BLD_ONSCREEN(hux, huy)) { bigBegin(hux + 32, huy + 48); drawHut(hux, huy); bigEnd(); }
	if (BLD_ONSCREEN(hux, huy)) drawHouseDecor(hux + 43, huy + 48, 52, 32, 42, 0);
	int hox = HOUSE_TX * TS - camx, hoy = HOUSE_TY * TS - camy;
	if (BLD_ONSCREEN(hox, hoy)) { bigBegin(hox + 32, hoy + 48); drawHouse(hox, hoy); bigEnd(); }
	if (BLD_ONSCREEN(hox, hoy)) drawHouseDecor(hox + 43, hoy + 48, 58, 62, 45, 1);
	int scx = SCHOOL_TX * TS - camx, scy = SCHOOL_TY * TS - camy;
	if (BLD_ONSCREEN(scx, scy)) { bigBegin(scx + 64, scy + 64); drawSchool(scx, scy); bigEnd(); }
	if (BLD_ONSCREEN(scx, scy)) drawHouseDecor(scx + 52, scy + 64, 80, 82, 58, 2);

	for (int i = 0; i < 3; i++) {
		int bx = STREET_SHOP_TX[i] * TS - camx, by = STREET_TY * TS - camy;
		if (BLD_ONSCREEN(bx, by)) { bigBegin(bx + 64, by + 64); drawStreetShop(bx, by, i); bigEnd(); }
		if (BLD_ONSCREEN(bx, by)) drawHouseDecor(bx + 52, by + 64, 80, 55, 62, i);
	}
	int shx = SHED_TX * TS - camx, shy = SHED_TY * TS - camy;
	if (shx > -90 && shx < SCR_W + 10 && shy > -30 && shy < SCR_H + 30) drawShed(shx, shy);
	drawBeachDecor(camx, camy);

	drawPlayAreas(camx, camy);

	int sx = (int)jay.x - camx, sy = (int)jay.y - camy;

	int ppx = (int)px - camx, ppy = (int)py - camy;
	int pcxs = ppx + PLAYER_SIZE / 2, pfeet = ppy + PLAYER_SIZE;
	int sfeet = sy + PLAYER_SIZE / 2;
	bool jayWalk = jay.wait == 0 && !dialogActive;

	// Characters, pets and tree canopies are drawn back-to-front by the line they
	// stand on, so you can walk behind a tree as well as in front of it.
	DrawEnt ents[3 + PET_COUNT + VEH_COUNT + WALKER_COUNT + KID_COUNT + CAROL_COUNT];
	int ne = 0, ei = 0;
	ents[ne++] = (DrawEnt){ sfeet, 0, 0 };
	ents[ne++] = (DrawEnt){ pfeet, 1, 0 };
	if (activeEvent != EV_NONE && fbx > -90 && fbx < SCR_W + 26 && fby > -52 && fby < SCR_H + 120)
		ents[ne++] = (DrawEnt){ fby + 3 * TS, 6, 0 };
	for (int i = 0; activeEvent == EV_PASKO && i < CAROL_COUNT; i++) {
		int gx, gy;
		carolSpot(i, &gx, &gy);
		int ex = gx - camx, ey = gy - camy;
		if (ex < -30 || ex > SCR_W + 30 || ey < -4 || ey > SCR_H + 50) continue;
		ents[ne++] = (DrawEnt){ ey, 7, i };
	}
	for (int i = 0; i < PET_COUNT; i++) {
		int ex = (int)pets[i].x - camx, ey = (int)pets[i].y - camy;
		if (ex < -30 || ex > SCR_W + 30 || ey < -10 || ey > SCR_H + 50) continue;
		ents[ne++] = (DrawEnt){ ey, 2, i };
	}
	for (int i = 0; i < VEH_COUNT; i++) {
		int ex = (int)vehicles[i].x - camx, ey = vehicles[i].feet - camy;
		if (ex < -110 || ex > SCR_W + 10 || ey < -10 || ey > SCR_H + 80) continue;
		ents[ne++] = (DrawEnt){ ey, 3, i };
	}
	for (int i = 0; i < WALKER_COUNT; i++) {
		int ex = (int)walkers[i].x - camx, ey = (int)walkers[i].y - camy + PLAYER_SIZE / 2;
		if (ex < -20 || ex > SCR_W + 20 || ey < -4 || ey > SCR_H + 50) continue;
		ents[ne++] = (DrawEnt){ ey, 4, i };
	}
	for (int i = 0; i < KID_COUNT; i++) {
		Kid k = kidAt(i);
		int ex = (int)k.x - camx, ey = (int)k.y - camy;
		if (k.gone || ex < -16 || ex > SCR_W + 16 || ey < -4 || ey > SCR_H + 44) continue;
		ents[ne++] = (DrawEnt){ ey, 5, i };
	}
	for (int i = 1; i < ne; i++) { // tiny insertion sort by feet line
		DrawEnt t = ents[i];
		int j = i - 1;
		while (j >= 0 && ents[j].feet > t.feet) { ents[j + 1] = ents[j]; j--; }
		ents[j + 1] = t;
	}

	for (int ty = ty0; ty <= ty0 + SCR_H / TS + 4 && ty < MAP_H; ty++) {
		for (int tx = tx0 - 3; tx <= tx0 + SCR_W / TS + 2; tx++) {
			if (tx < 0 || tx >= MAP_W) continue;
			u8 tt = map[ty][tx];
			if (tt != T_TREE && tt != T_APPLE && tt != T_LAMP && tt != T_SCARE && tt != T_POLE && tt != T_TRASH &&
			    tt != T_PALM && tt != T_PARASOL && tt != T_SIGN) continue;
			int tsx = tx * TS - camx, tsy = ty * TS - camy;
			while (ei < ne && ents[ei].feet <= tsy + TS) {
				drawEnt(&ents[ei], sx, sfeet, jayWalk, pcxs, pfeet, camx);
				ei++;
			}
			if (tt == T_LAMP) drawLamp(tsx, tsy);
			else if (tt == T_SCARE) drawScarecrow(tsx, tsy);
			else if (tt == T_POLE) drawElectricPost(tsx, tsy);
			else if (tt == T_TRASH) drawTrashBin(tsx, tsy, tx);
			else if (tt == T_PALM) drawPalm(tsx, tsy, tx, ty);
			else if (tt == T_PARASOL) drawParasol(tsx, tsy, tx, ty);
			else if (tt == T_SIGN) drawBeachSign(tsx, tsy);
			else drawTree(tsx, tsy, tx, ty);
		}
	}
	while (ei < ne) {
		drawEnt(&ents[ei], sx, sfeet, jayWalk, pcxs, pfeet, camx);
		ei++;
	}

	drawPlayAir(camx, camy);
	drawWires(camx, camy);
	drawDayNight();
	drawEventSky();
	drawWeather();
	for (int i = 0; i < PET_COUNT; i++)
		drawPetBubble(&pets[i], (int)pets[i].x - camx, (int)pets[i].y - camy);

	drawVignette();
	if (!dialogActive && !tahoMenu) { drawHotbar(pcxs, pfeet, false); drawFullnessHud(); }
	drawCarolBubbles(camx, camy);
	drawEatFx(pcxs, pfeet);

	if (!dialogActive) {
		if (atDoor())
			drawPrompt(DOOR_TX * TS + TS / 2 - camx, (DOOR_TY + 1) * TS + 18 - camy, "A ENTER");
		else if (atBakeryDoor())
			drawPrompt(BDOOR_TX * TS + TS / 2 - camx, (BDOOR_TY + 1) * TS + 18 - camy, "A ENTER");
		else if (atSariDoor())
			drawPrompt(SDOOR_TX * TS + TS / 2 - camx, (SDOOR_TY + 1) * TS + 18 - camy, "A ENTER");
		else if (atSchoolDoor())
			drawPrompt(SCH_DOOR_TX * TS + TS / 2 - camx, (SCH_DOOR_TY + 1) * TS + 18 - camy, "A ENTER");
		else if (atStreetDoor())
			drawPrompt((STREET_SHOP_TX[atStreetDoor() - 3] + 3) * TS + TS / 2 - camx,
			           (STREET_DOOR_TY + 1) * TS + 18 - camy, "A ENTER");
		else if (nearJay())
			drawPrompt(sx, sfeet - SPR_H - 22, "A TALK");
		else if (nearWalker() >= 0) {
			const Walker *w = &walkers[nearWalker()];
			drawPrompt((int)w->x - camx, (int)w->y - camy + PLAYER_SIZE / 2 - SPR_H - 22, "A TALK");
		}
		else if (nearKid() >= 0) {
			Kid k = kidAt(nearKid());
			drawPrompt((int)k.x - camx, (int)k.y - camy - 46, "A TALK");
		}
		else if (nearCaroler() >= 0) {
			int gx, gy;
			carolSpot(nearCaroler(), &gx, &gy);
			drawPrompt(gx - camx, gy - camy - 58, "A TALK");
		}
		else {
			int pi = nearPet();
			if (pi >= 0 && pets[pi].happy == 0)
				drawPrompt((int)pets[pi].x - camx,
				           (int)pets[pi].y - camy - (pets[pi].kind == PET_KALABAW ? 52 : 36), "A PET");
		}
	}

	if (dialogActive) {
		bool greet = dialogLine < eventLines();
		int li = greet ? 0 : dialogLine - eventLines(); // the person's usual lines
		if (talkingGame == GAME_CAROL)
			drawDialogBox("MGA NAMAMASKO",
			              greet ? eventGreeting() : CAROL_TALK[talkingCarol % CAROL_SETS][li]);
		else if (talkingGame >= 0)
			drawDialogBox(KID_NAMES[talkingGame], greet ? eventGreeting() : GAME_TALK[talkingGame][li]);
		else if (talkingWalker >= 0)
			drawDialogBox(WALKER_INFO[talkingWalker].name,
			              greet ? eventGreeting() : WALKER_INFO[talkingWalker].lines[li]);
		else drawDialogBox("JAY", greet ? eventGreeting() : dialogJay[li]);
	}
	if (tahoMenu) drawTahoMenu();
}

// ---------------------------------------------------------------- cottage interior
// ---------------------------------------------------------------- bakery interior
// Ate Mila: young baker in a cream chef's coat, pink apron and a chef's hat
static void drawBaker(void) {
	int x = BAKER_X, fy = OWNER_FEET;
	int myStyle = styleIndex, mySkirt = skirtIndex, myShoes = shoeIndex;
	styleIndex = 2; skirtIndex = 0; shoeIndex = 0;
	drawPlayerSprite(x, fy, 0, false, RGB(240, 196, 168), RGB(176, 126, 78), RGB(252, 248, 240));
	styleIndex = myStyle; skirtIndex = mySkirt; shoeIndex = myShoes;
	int top = fy - SPR_H;
	blob(x - 7, top, 14, 4, WHITE, INK);       // hat band
	blob(x - 5, top - 7, 10, 8, WHITE, INK);   // hat puff
	rect(x - 2, top - 5, 3, 1, PAPER_DK);
	blob(x - 6, fy - 17, 12, 14, PINK_LT, INK); // apron
	drawHeart(x - 3, fy - 14, 1, PAPER);
}

static void drawBakeryRoom(void) {
	u32 wall = RGB(232, 208, 222), stripe = RGB(220, 190, 208);
	u32 wain = RGB(170, 118, 128), wainHi = RGB(200, 150, 156);
	u32 floorC = RGB(204, 178, 160), floorD = RGB(178, 150, 136);
	u32 shelfW = RGB(132, 80, 78), shelfD = RGB(96, 58, 60);
	u32 cream = RGB(246, 232, 208), creamDk = RGB(214, 190, 164);

	rect(ROOM_X - 3, ROOM_Y - 3, ROOM_W + 6, ROOM_H + 6, INK);

	// striped lilac wallpaper and wainscot
	rect(ROOM_X, ROOM_Y, ROOM_W, 64, wall);
	for (int k = 0; k < ROOM_W; k += 12) rect(ROOM_X + k + 3, ROOM_Y, 4, 48, stripe);
	rect(ROOM_X, ROOM_Y + 48, ROOM_W, 16, wain);
	rect(ROOM_X, ROOM_Y + 48, ROOM_W, 2, wainHi);
	rect(ROOM_X, FLOOR_Y - 2, ROOM_W, 2, shelfD);

	// plank floor
	rect(ROOM_X, FLOOR_Y, ROOM_W, ROOM_H - 64, floorC);
	for (int x = ROOM_X + 14; x < ROOM_X + ROOM_W; x += 18) rect(x, FLOOR_Y, 1, ROOM_H - 64, floorD);

	// sunny window
	blob(66, ROOM_Y + 8, 36, 34, shelfW, INK);
	rect(70, ROOM_Y + 12, 28, 26, RGB(198, 228, 238));
	rect(83, ROOM_Y + 12, 2, 26, shelfW);
	rect(70, ROOM_Y + 24, 28, 2, shelfW);
	rect(64, ROOM_Y + 42, 40, 3, shelfD);

	// big back shelf: boxes and jars, bread baskets, tins
	blob(112, ROOM_Y + 4, 184, 50, shelfW, INK);
	rect(114, ROOM_Y + 20, 180, 2, shelfD);
	rect(114, ROOM_Y + 34, 180, 2, shelfD);
	for (int k = 0; k < 8; k++) {
		int bx = 118 + k * 22;
		if (k & 1) {
			rect(bx + 2, ROOM_Y + 10, 10, 10, RGB(236, 120, 140));
			rect(bx + 2, ROOM_Y + 10, 10, 2, RGB(250, 226, 230));
		} else {
			rect(bx, ROOM_Y + 8, 16, 12, cream);
			rect(bx, ROOM_Y + 8, 16, 2, creamDk);
		}
		bun(bx, ROOM_Y + 25, 14, 9, (k & 1) ? BREAD_MID : BREAD_HI, BREAD_DK);
		rect(bx, ROOM_Y + 40, 14, 10, (k % 3 == 0) ? RGB(164, 200, 176) : cream);
		rect(bx, ROOM_Y + 40, 14, 2, creamDk);
	}

	// chalkboard menu
	blob(304, ROOM_Y + 6, 34, 40, shelfW, INK);
	rect(307, ROOM_Y + 9, 28, 34, RGB(54, 50, 60));
	pixTextC(321, ROOM_Y + 12, 1, "MENU", WHITE, 0, 0);
	for (int k = 0; k < 4; k++) {
		rect(310, ROOM_Y + 24 + k * 5, 14, 1, RGB(210, 206, 214));
		rect(327, ROOM_Y + 24 + k * 5, 5, 1, RGB(236, 170, 190));
	}

	// red checkered rug in front of the counter
	blob(87, 131, 82, 38, PAPER, ROSE_DK);
	for (int ry = 0; ry < 3; ry++)
		for (int cx = 0; cx < 5; cx++)
			if ((cx + ry) & 1) rect(88 + cx * 16, 132 + ry * 12, 16, 12, ROSE);

	// potted plant in the corner
	blob(320, FLOOR_Y + 4, 16, 16, LEAF_MID, LEAF_DK);
	rect(322, FLOOR_Y + 6, 6, 3, LEAF_HI);
	blob(322, FLOOR_Y + 20, 12, 10, RGB(196, 118, 90), INK);

	// exit doorway and mat
	rect(SCR_W / 2 - 26, ROOM_Y + ROOM_H, 52, 3, SHOP_GLOW);
	blob(SCR_W / 2 - 22, ROOM_Y + ROOM_H - 14, 44, 12, MINT_UI, INK);
	pixTextC(SCR_W / 2, ROOM_Y + ROOM_H - 11, 1, "EXIT", PAPER, 0, 0);

	drawBaker(); // drawn before the counter so it covers her lower half
}

static void drawBakeryFurniture(int i) {
	const int *f = FURN[shopId][i];
	int x = f[0], y = f[1], w = f[2], h = f[3];
	u32 wood = RGB(150, 92, 84), woodHi = RGB(204, 140, 118), woodDk = RGB(104, 62, 62);
	rect(x + 2, y + h - 2, w, 4, SHADOW);
	if (i == FUR_COUNTER) {
		blob(x, y - 14, w, h + 14, wood, INK);
		rect(x + 1, y - 13, w - 2, 6, woodHi);
		rect(x + 4, y - 5, w - 8, h + 1, RGB(246, 226, 208));   // glass display case
		rect(x + 4, y - 5, w - 8, 1, WHITE);
		for (int k = 0; k < 6; k++)
			bun(x + 8 + k * 20, y + 1, 14, 8, (k & 1) ? BREAD_HI : BREAD_MID, BREAD_DK);
		rect(x + 98, y - 23, 20, 11, INK);                      // cash register
		rect(x + 99, y - 22, 18, 9, STONE_MID);
		rect(x + 102, y - 28, 12, 6, STONE_DK);
		rect(x + 103, y - 27, 10, 3, SHOP_GLOW);
		blob(x + 8, y - 20, 22, 8, SHOP_WOOD_HI, INK);          // bread basket
		bun(x + 11, y - 24, 8, 5, BREAD_MID, BREAD_DK);
		bun(x + 20, y - 24, 8, 5, BREAD_HI, BREAD_DK);
	} else if (i == FUR_TABLE) {
		blob(x, y - 8, w, 14, woodHi, INK);
		rect(x + 4, y + 6, 4, h - 6, woodDk);
		rect(x + w - 8, y + 6, 4, h - 6, woodDk);
		blob(x + 5, y - 13, 20, 8, SHOP_WOOD_HI, INK);
		bun(x + 7, y - 17, 8, 5, BREAD_MID, BREAD_DK);
		bun(x + 15, y - 17, 8, 5, BREAD_HI, BREAD_DK);
		rect(x + 30, y - 11, 6, 4, PAPER);                      // tea cup
		rect(x + 31, y - 11, 4, 1, SHOP_WOOD);
	} else { // two-tier bread display tables
		bool left = (i == FUR_STAND_L);
		blob(x, y - 8, w, 12, woodDk, INK);
		blob(x + 2, y + 4, w - 4, h - 4, wood, INK);
		for (int k = 0; k < 4; k++) {
			u32 top = left ? ((k & 1) ? BREAD_HI : BREAD_MID) : ((k & 1) ? RGB(250, 236, 206) : RGB(238, 176, 170));
			bun(x + 4 + k * 15, y - 17, 12, 10, top, BREAD_DK);
			blob(x + 6 + k * 14, y + 9, 11, 5, left ? BREAD_MID : BREAD_HI, BREAD_DK);
		}
		rect(x + 8, y - 1, 5, 3, WHITE);                        // price tags
		rect(x + w - 14, y - 1, 5, 3, WHITE);
	}
}

// the buy menu, shown over the top screen after talking to Ate Mila
static void drawTahoMenu(void) {
	char buf[32];
	blob(70, 50, 260, 130, PAPER, INK);
	rect(72, 52, 256, 3, PINK_PALE);
	pill(120, 40, 160, 22, PINK_LT, PINK_DK);
	pixTextC(200, 44, 2, "TAHO", WHITE, 0, PLUM);

	blob(82, 74, 236, 34, PINK_PALE, PINK_DK);
	drawHeart(88, 87, 1, ROSE);
	drawItem(104, 85, 16, 12, ITEM_TAHO);
	drawText(130, 80, 0.6f, INK, ITEM_NAMES[ITEM_TAHO]);
	drawText(130, 94, 0.4f, INK_SOFT, "may sago at arnibal");
	snprintf(buf, sizeof(buf), "P%d", ITEM_PRICE[ITEM_TAHO]);
	drawText(236, 80, 0.6f, PINK_DK, buf);
	snprintf(buf, sizeof(buf), "x%d", itemTotal(ITEM_TAHO));
	drawText(284, 84, 0.5f, INK_SOFT, buf);

	snprintf(buf, sizeof(buf), "Coins: P%d", coins);
	drawText(84, 120, 0.55f, PLUM, buf);
	drawText(214, 124, 0.42f, INK_SOFT, "A: buy    B: leave");
	if (tahoMsg) drawText(84, 146, 0.5f, ROSE_DK, tahoMsg);
}

static void drawBakeryMenu(void) {
	char buf[32];
	int base = shopBase();

	blob(60, 18, 280, 204, PAPER, INK);
	rect(62, 20, 276, 3, PINK_PALE);
	pill(120, 8, 160, 22, PINK_LT, PINK_DK);
	static const char *TITLES[SHOP_COUNT] = { "", "BAKERY", "STORE", "UKAY UKAY", "PET SHOP", "KARENDERYA", "" };
	pixTextC(200, 12, 2, TITLES[shopId], WHITE, 0, PLUM);

	for (int i = 0; i < BAKE_COUNT; i++) {
		int y = 40 + i * 34;
		bool sel = (i == shopSel);
		blob(72, y, 256, 30, sel ? PINK_PALE : PAPER, sel ? PINK_DK : PAPER_DK);
		if (sel) drawHeart(78, y + 11, 1, ROSE);
		drawItem(94, y + 9, 16, 12, base + i);
		drawText(118, y + 6, 0.6f, INK, ITEM_NAMES[base + i]);
		snprintf(buf, sizeof(buf), "P%d", ITEM_PRICE[base + i]);
		drawText(240, y + 6, 0.6f, PINK_DK, buf);
		if (isClothes(base + i)) {
			if (*wearSlot(base + i) == wearValue(base + i)) drawText(284, y + 9, 0.4f, ROSE_DK, "SUOT");
		} else {
			snprintf(buf, sizeof(buf), "x%d", itemTotal(base + i));
			drawText(292, y + 8, 0.5f, INK_SOFT, buf);
		}
	}

	snprintf(buf, sizeof(buf), "Coins: P%d", coins);
	drawText(74, 178, 0.55f, PLUM, buf);
	drawText(238, 182, 0.42f, INK_SOFT, "A: buy    B: leave");
	if (shopMsg) drawText(74, 198, 0.5f, ROSE_DK, shopMsg);
}

// Kuya Ben: Jay's sprite in warm colors, with a cap and an apron
static void drawSariOwner(void) {
	int x = SARI_X, fy = OWNER_FEET;
	jayVariant = 1;
	drawJaySprite(x, fy, 0, false);
	jayVariant = 0;
	int top = fy - SPR_H;
	blob(x - 9, top + 1, 18, 6, SKY_UI, INK);   // cap
	rect(x - 9, top + 6, 18, 2, TIN_DK);        // brim
	blob(x - 6, fy - 17, 12, 14, TIN_HI, INK);  // apron
	rect(x - 3, fy - 12, 6, 4, TIN_MID);        // apron pocket
}

static void drawSariRoom(void) {
	u32 wall = RGB(196, 226, 210), stripe = RGB(176, 210, 192);
	u32 wain = RGB(120, 160, 148), wainHi = RGB(158, 196, 178);
	u32 floorC = RGB(222, 204, 170), floorD = RGB(196, 176, 142);
	u32 shelfW = RGB(150, 106, 76), shelfD = RGB(108, 74, 56);
	u32 pal[4] = { ROSE, GOLD_UI, MINT_UI, SKY_UI };

	rect(ROOM_X - 3, ROOM_Y - 3, ROOM_W + 6, ROOM_H + 6, INK);

	// mint striped wall and wainscot
	rect(ROOM_X, ROOM_Y, ROOM_W, 64, wall);
	for (int k = 0; k < ROOM_W; k += 12) rect(ROOM_X + k + 3, ROOM_Y, 4, 48, stripe);
	rect(ROOM_X, ROOM_Y + 48, ROOM_W, 16, wain);
	rect(ROOM_X, ROOM_Y + 48, ROOM_W, 2, wainHi);
	rect(ROOM_X, FLOOR_Y - 2, ROOM_W, 2, shelfD);

	// checkered tile floor
	rect(ROOM_X, FLOOR_Y, ROOM_W, ROOM_H - 64, floorC);
	for (int ry = 0; ry < 5; ry++)
		for (int cx = 0; cx < 12; cx++)
			if ((cx + ry) & 1) rect(ROOM_X + cx * 24, FLOOR_Y + ry * 30, 24, 30, floorD);

	// big back shelf: chip bags, cans and bottles
	blob(70, ROOM_Y + 4, 240, 46, shelfW, INK);
	rect(72, ROOM_Y + 18, 236, 2, shelfD);
	rect(72, ROOM_Y + 32, 236, 2, shelfD);
	for (int k = 0; k < 14; k++) {
		int bx = 76 + k * 16;
		rect(bx, ROOM_Y + 8, 10, 10, pal[k % 4]);
		rect(bx, ROOM_Y + 8, 10, 2, WHITE);
		rect(bx + 1, ROOM_Y + 22, 8, 10, pal[(k + 2) % 4]);
		rect(bx + 1, ROOM_Y + 22, 8, 2, STONE_HI);
		rect(bx + 2, ROOM_Y + 36, 6, 12, pal[(k + 1) % 4]);
		rect(bx + 3, ROOM_Y + 34, 4, 2, STONE_HI);
	}

	// little poster on the right
	blob(318, ROOM_Y + 8, 20, 28, PAPER, INK);
	drawHeart(324, ROOM_Y + 16, 1, ROSE);

	// striped rug in front of the counter
	blob(104, 131, 78, 36, PAPER, TIN_DK);
	for (int k = 0; k < 4; k++) rect(108 + k * 18, 135, 8, 28, TIN_HI);

	// potted plant in the corner
	blob(320, FLOOR_Y + 4, 16, 16, LEAF_MID, LEAF_DK);
	rect(322, FLOOR_Y + 6, 6, 3, LEAF_HI);
	blob(322, FLOOR_Y + 20, 12, 10, RGB(196, 118, 90), INK);

	// exit doorway and mat
	rect(SCR_W / 2 - 26, ROOM_Y + ROOM_H, 52, 3, SHOP_GLOW);
	blob(SCR_W / 2 - 22, ROOM_Y + ROOM_H - 14, 44, 12, MINT_UI, INK);
	pixTextC(SCR_W / 2, ROOM_Y + ROOM_H - 11, 1, "EXIT", PAPER, 0, 0);

	drawSariOwner(); // drawn before the counter so it covers his lower half
}

static void drawSariFurniture(int i) {
	const int *f = FURN[shopId][i];
	int x = f[0], y = f[1], w = f[2], h = f[3];
	u32 pal[4] = { ROSE, GOLD_UI, MINT_UI, SKY_UI };
	u32 wood = RGB(168, 118, 80), woodHi = RGB(214, 164, 112), woodDk = RGB(112, 76, 56);
	rect(x + 2, y + h - 2, w, 4, SHADOW);
	if (i == FUR_COUNTER) {
		blob(x, y - 14, w, h + 14, wood, INK);
		rect(x + 1, y - 13, w - 2, 6, woodHi);
		for (int k = 1; k < 6; k++) rect(x + k * 16, y - 5, 1, h + 3, woodDk);
		for (int k = 0; k < 2; k++) {                          // candy jars
			blob(x + 4 + k * 12, y - 27, 10, 13, RGB(224, 238, 240), INK);
			rect(x + 6 + k * 12, y - 22, 2, 2, pal[k]);
			rect(x + 8 + k * 12, y - 19, 2, 2, pal[k + 2]);
			rect(x + 5 + k * 12, y - 29, 8, 2, woodDk);
		}
		rect(x + 66, y - 22, 22, 9, INK);                      // cash box
		rect(x + 67, y - 21, 20, 7, STONE_MID);
		rect(x + 70, y - 26, 12, 5, STONE_DK);
		rect(x + 71, y - 25, 10, 3, SHOP_GLOW);
	} else if (i == FUR_TABLE) {                               // ice cooler
		blob(x, y - 10, w, 30, RGB(150, 196, 214), INK);
		rect(x + 1, y - 9, w - 2, 6, WHITE);
		pixTextC(x + w / 2, y + 2, 1, "ICE", ROSE_DK, 0, 0);
		for (int k = 0; k < 4; k++) rect(x + 6 + k * 9, y - 18, 3, 8, pal[k]);
	} else {                                                   // crates of goods
		blob(x, y - 8, w, 12, woodDk, INK);
		blob(x + 2, y + 4, w - 4, h - 4, wood, INK);
		for (int k = 0; k < w / 8; k++) {
			rect(x + 3 + k * 8, y - 14, 6, 6, pal[(k + i) % 4]);
			rect(x + 5 + k * 8, y + 8, 6, 5, pal[(k + i + 1) % 4]);
		}
	}
}

// ---------------------------------------------------------------- street shop interiors
// Ate Joy, Mang Tonyo and Aling Nena
static void drawStreetOwner(void) {
	int x = npcX(), fy = OWNER_FEET, v = shopId - 3;
	int myStyle = styleIndex, mySkirt = skirtIndex, myShoes = shoeIndex;
	styleIndex = v == 0 ? 2 : v == 1 ? 1 : 0;
	skirtIndex = v == 1 ? 1 : 2; shoeIndex = 0;
	u32 hair = v == 0 ? RGB(196, 98, 112) : v == 1 ? RGB(54, 40, 46) : RGB(102, 77, 70);
	u32 top = v == 0 ? RGB(214, 188, 232) : v == 1 ? RGB(170, 214, 184) : RGB(248, 184, 206);
	drawPlayerSprite(x, fy, 0, false, v == 1 ? RGB(196, 140, 108) : RGB(235, 183, 162), hair, top);
	styleIndex = myStyle; skirtIndex = mySkirt; shoeIndex = myShoes;
	if (v == 1) {                                       // Mang Tonyo's cap
		blob(x - 8, fy - SPR_H, 16, 6, LEAF_MID, INK);
		rect(x - 8, fy - SPR_H + 5, 16, 2, LEAF_DK);
	}
	if (v == 2) blob(x - 5, fy - SPR_H - 2, 10, 7, hair, INK); // hair bun
	blob(x - 6, fy - 17, 12, 14, v == 1 ? TIN_HI : v == 2 ? GOLD_UI : PINK_LT, INK); // apron
	drawHeart(x - 3, fy - 14, 1, PAPER);
}

static void drawStreetRoom(void) {
	int v = shopId - 3;
	u32 wall   = v == 0 ? RGB(236, 216, 240) : v == 1 ? RGB(210, 230, 244) : RGB(250, 230, 188);
	u32 wain   = v == 0 ? RGB(168, 128, 180) : v == 1 ? RGB(122, 160, 196) : RGB(190, 132, 84);
	u32 floorA = v == 0 ? RGB(226, 196, 168) : v == 1 ? RGB(226, 228, 214) : RGB(214, 206, 190);
	u32 floorB = v == 0 ? RGB(204, 170, 142) : v == 1 ? RGB(198, 204, 188) : RGB(188, 178, 162);
	u32 pal[4] = { ROSE, GOLD_UI, MINT_UI, SKY_UI };

	rect(ROOM_X - 3, ROOM_Y - 3, ROOM_W + 6, ROOM_H + 6, INK);
	rect(ROOM_X, ROOM_Y, ROOM_W, 48, wall);
	rect(ROOM_X, ROOM_Y + 48, ROOM_W, 16, wain);
	rect(ROOM_X, FLOOR_Y - 2, ROOM_W, 2, INK_SOFT);
	rect(ROOM_X, FLOOR_Y, ROOM_W, ROOM_H - 64, floorA);
	for (int ry = 0; ry < 5; ry++)
		for (int cx = 0; cx < 12; cx++)
			if ((cx + ry) & 1) rect(ROOM_X + cx * 24, FLOOR_Y + ry * 30, 24, 30, floorB);

	if (v == 0) { // two clothes rails and a mirror
		for (int r = 0; r < 2; r++) {
			int rx = r ? 214 : 70;
			rect(rx, ROOM_Y + 10, 88, 2, SHOP_WOOD_DK);
			for (int k = 0; k < 6; k++) {
				int cx = rx + 3 + k * 14;
				u32 c = pal[(k + r) % 4];
				rect(cx + 5, ROOM_Y + 12, 1, 3, INK);
				rect(cx, ROOM_Y + 15, 11, 6, c);
				rect(cx + 2, ROOM_Y + 21, 7, (k & 1) ? 20 : 13, c);
			}
		}
		blob(312, ROOM_Y + 6, 22, 44, SHOP_WOOD, INK);
		rect(316, ROOM_Y + 10, 14, 36, RGB(198, 228, 238));
		rect(319, ROOM_Y + 13, 3, 14, WHITE);
	} else if (v == 1) { // shelves of food and toys, and a fish tank
		blob(70, ROOM_Y + 6, 170, 42, SHOP_WOOD, INK);
		rect(72, ROOM_Y + 26, 166, 2, SHOP_WOOD_DK);
		for (int k = 0; k < 10; k++) {
			blob(76 + k * 16, ROOM_Y + 10, 12, 16, pal[k % 4], INK);
			rect(79 + k * 16, ROOM_Y + 15, 6, 4, PAPER);
			blob(77 + k * 16, ROOM_Y + 34, 10, 10, pal[(k + 2) % 4], INK);
		}
		blob(254, ROOM_Y + 10, 76, 36, RGB(130, 192, 222), INK); // fish tank
		for (int k = 0; k < 3; k++) {
			int fx = 262 + k * 20 + ((frameCount / 20 + k) % 4) * 3;
			rect(fx, ROOM_Y + 20 + k * 7, 8, 4, k == 1 ? ROSE : GOLD_UI);
			rect(fx - 3, ROOM_Y + 19 + k * 7, 3, 6, k == 1 ? ROSE : GOLD_UI);
		}
		rect(258, ROOM_Y + 40, 68, 3, SOIL);
	} else { // menu board, hanging ladles and a wall clock
		blob(150, ROOM_Y + 5, 116, 42, RGB(60, 100, 76), INK);
		pixTextC(208, ROOM_Y + 10, 1, "MENU", PAPER, 0, 0);
		for (int k = 0; k < 3; k++) {
			rect(160, ROOM_Y + 24 + k * 7, 56 - k * 7, 2, PAPER);
			rect(240, ROOM_Y + 24 + k * 7, 14, 2, GOLD_UI);
		}
		rect(70, ROOM_Y + 12, 60, 2, SHOP_WOOD_DK);
		for (int k = 0; k < 4; k++) {
			rect(77 + k * 15, ROOM_Y + 14, 2, 14, STONE_DK);
			blob(74 + k * 15, ROOM_Y + 27, 8, 7, STONE_HI, INK);
		}
		blob(298, ROOM_Y + 10, 26, 26, PAPER, INK);
		rect(310, ROOM_Y + 15, 2, 9, INK);
		rect(310, ROOM_Y + 22, 8, 2, INK);
	}

	// exit doorway and mat
	rect(SCR_W / 2 - 26, ROOM_Y + ROOM_H, 52, 3, SHOP_GLOW);
	blob(SCR_W / 2 - 22, ROOM_Y + ROOM_H - 14, 44, 12, MINT_UI, INK);
	pixTextC(SCR_W / 2, ROOM_Y + ROOM_H - 11, 1, "EXIT", PAPER, 0, 0);

	drawStreetOwner(); // drawn before the counter so it covers their lower half
}

static void drawStreetFurniture(int i) {
	const int *f = FURN[shopId][i];
	int x = f[0], y = f[1], w = f[2], h = f[3], v = shopId - 3;
	u32 pal[4] = { ROSE, GOLD_UI, MINT_UI, SKY_UI };
	rect(x + 2, y + h - 2, w, 4, SHADOW);
	if (i == FUR_COUNTER) {
		blob(x, y - 14, w, h + 14, SHOP_WOOD, INK);
		rect(x + 1, y - 13, w - 2, 6, SHOP_WOOD_HI);
		if (v == 2) { // pots of ulam
			u32 ulam[4] = { RGB(128, 78, 48), RGB(232, 168, 90), LEAF_MID, RGB(214, 90, 70) };
			for (int k = 0; k < 4; k++) {
				int cx = x + 56 + k * 18; // right of Aling Nena's spot
				blob(cx, y - 20, 16, 12, STONE_HI, INK);
				rect(cx + 2, y - 19, 12, 3, ulam[k]);
			}
			blob(x + 6, y - 21, 18, 13, WHITE, INK);   // rice cooker
			rect(x + 12, y - 17, 6, 3, ROSE);
		} else {
			rect(x + 70, y - 23, 20, 10, INK);          // cash box
			rect(x + 71, y - 22, 18, 8, v ? STONE_MID : GOLD_UI);
			for (int k = 0; k < 2; k++)                 // folded shirts or food tins
				blob(x + 6 + k * 16, y - 19, 13, v ? 11 : 7, pal[k + 2], INK);
		}
	} else if (v == 2) { // dining table with a tablecloth, plates and a bottle of suka
		blob(x, y - 8, w, 16, RGB(214, 82, 86), INK);
		for (int k = 6; k < w - 4; k += 12) rect(x + k, y - 6, 5, 12, RGB(240, 150, 150));
		rect(x + 5, y + 8, 4, h - 8, SHOP_WOOD_DK);
		rect(x + w - 9, y + 8, 4, h - 8, SHOP_WOOD_DK);
		blob(x + 8, y - 8, 16, 9, WHITE, INK);
		blob(x + w - 26, y - 8, 16, 9, WHITE, INK);
		rect(x + w / 2 - 2, y - 15, 5, 12, BARK);
	} else if (v == 1 && i == FUR_TABLE) { // puppy pen
		rect(x, y - 4, w, h + 4, RGB(236, 214, 160));
		blob(x + 18, y - 6, 22, 18, RGB(250, 240, 222), INK);
		rect(x + 17, y - 6, 5, 10, BARK);
		rect(x + 36, y - 6, 5, 10, BARK);
		rect(x + 24, y, 3, 3, INK);
		rect(x + 32, y, 3, 3, INK);
		rect(x + 28, y + 5, 4, 3, INK);
		for (int k = 0; k <= w - 4; k += 10) rect(x + k, y + 2, 3, h - 2, STONE_HI);
		rect(x, y + 8, w, 3, STONE_MID);
	} else if (v == 0 && i == FUR_TABLE) { // rolling clothes rack
		rect(x + 2, y - 22, w - 4, 2, INK);
		rect(x + 2, y - 20, 3, h + 18, INK);
		rect(x + w - 5, y - 20, 3, h + 18, INK);
		for (int k = 0; k < 5; k++) {
			rect(x + 7 + k * 11, y - 19, 11, 7, pal[(k + 1) % 4]);
			rect(x + 9 + k * 11, y - 12, 7, 18, pal[(k + 1) % 4]);
		}
	} else { // bins of clothes, or shelves of pet goods
		blob(x, y - 6, w, 10, SHOP_WOOD_DK, INK);
		for (int k = 0; k < 4; k++)
			blob(x + 3 + k * 13, y - 12, 13, 10, pal[(k + i) % 4], INK);
		blob(x + 2, y + 2, w - 4, h - 2, SHOP_WOOD, INK);
		rect(x + 8, y + 8, w - 16, 3, SHOP_WOOD_HI);
	}
}

// ---------------------------------------------------------------- school interior
// A classroom: blackboard, flag, bookshelf, curtained window, a teacher's desk and three
// student desks. Everybody has a little thought bubble that floats beside their head.
static const int FUR_NPC[FUR_COUNT] = { 0, 3, 2, 4 };   // which school character belongs to each piece of furniture

static void drawEmote(int cx, int headTop, int n) {
	int bob = (frameCount / 24 + n) & 1;
	int x = cx + 3, y = headTop - 17 - bob;
	int kind = (frameCount / 110 + n) % 3;
	blob(x, y, 17, 12, WHITE, INK);
	rect(x + 3, y + 12, 4, 1, INK);                 // tail
	rect(x + 4, y + 13, 2, 1, INK);
	if (kind == 0) {                                // smile
		rect(x + 5, y + 3, 2, 2, INK);
		rect(x + 10, y + 3, 2, 2, INK);
		rect(x + 5, y + 7, 1, 1, INK);
		rect(x + 11, y + 7, 1, 1, INK);
		rect(x + 6, y + 8, 5, 1, INK);
	} else if (kind == 1) {                         // "..."
		for (int k = 0; k < 3; k++) rect(x + 4 + k * 4, y + 5, 2, 2, INK);
	} else {                                        // heart
		drawHeart(x + 5, y + 3, 1, ROSE);
	}
}

// the bubble hides while the "A TALK" prompt is showing for that person
static void schoolEmote(int idx) {
	if (nearSchoolNpc() == idx) return;
	drawEmote(SCHOOL_NPCS[idx].x, SCHOOL_NPCS[idx].top, idx);
}

// 0 = Miss Luz (glasses, bun, teal blouse), 1 = Sir Dante (white polo and a necktie)
static void drawSchoolTeacher(int which, int x, int fy) {
	if (which == 0) {
		int myStyle = styleIndex, mySkirt = skirtIndex, myShoes = shoeIndex;
		u32 hair = RGB(58, 42, 48);
		styleIndex = 0; skirtIndex = 1; shoeIndex = 0;
		drawPlayerSprite(x, fy, 0, false, RGB(238, 190, 164), hair, RGB(150, 208, 196));
		styleIndex = myStyle; skirtIndex = mySkirt; shoeIndex = myShoes;
		blob(x - 5, fy - SPR_H - 2, 10, 8, hair, INK);   // hair bun
		chalkBox(x - 7, fy - 27, 6, 5, INK_SOFT);        // glasses
		chalkBox(x + 1, fy - 27, 6, 5, INK_SOFT);
		rect(x - 1, fy - 26, 2, 1, INK_SOFT);
	} else {
		jayVariant = 3;
		drawJaySprite(x, fy, 0, false);
		jayVariant = 0;
		rect(x - 2, fy - 18, 4, 2, ROSE_DK);             // necktie
		rect(x - 1, fy - 16, 2, 6, ROSE_DK);
		rect(x + 7, fy - 14, 7, 6, INK);                 // a book under his arm
		rect(x + 8, fy - 13, 5, 4, SKY_UI);
	}
}

static void drawSchoolRoom(void) {
	u32 wall = RGB(252, 238, 196), band = RGB(240, 208, 142);
	u32 wain = RGB(222, 178, 118), wainHi = RGB(244, 210, 156);
	u32 floorA = RGB(236, 220, 184), floorB = RGB(222, 202, 162);
	u32 chalk = RGB(246, 246, 238), curtain = RGB(206, 70, 80), curtainDk = RGB(160, 46, 62);
	u32 pal[4] = { ROSE, GOLD_UI, MINT_UI, SKY_UI };

	rect(ROOM_X - 3, ROOM_Y - 3, ROOM_W + 6, ROOM_H + 6, INK);

	// cream wall, warm stripe and a wooden lower wall
	rect(ROOM_X, ROOM_Y, ROOM_W, 64, wall);
	rect(ROOM_X, ROOM_Y + 38, ROOM_W, 6, band);
	rect(ROOM_X, ROOM_Y + 48, ROOM_W, 16, wain);
	rect(ROOM_X, ROOM_Y + 48, ROOM_W, 2, wainHi);
	rect(ROOM_X, FLOOR_Y - 2, ROOM_W, 2, SHOP_WOOD_DK);

	// checkered tile floor
	rect(ROOM_X, FLOOR_Y, ROOM_W, ROOM_H - 64, floorA);
	for (int r = 0; r < 10; r++)
		for (int c = 0; c < 18; c++)
			if ((r + c) & 1) rect(ROOM_X + c * 16, FLOOR_Y + r * 16, 16, r == 9 ? 8 : 16, floorB);

	// bookshelf
	blob(66, 18, 34, 62, SHOP_WOOD, INK);
	rect(69, 21, 28, 56, SHOP_WOOD_DK);
	for (int s = 0; s < 3; s++) {
		int sy = 21 + s * 19;
		rect(69, sy + 17, 28, 2, SHOP_WOOD_HI);
		for (int k = 0; k < 5; k++)
			rect(71 + k * 5, sy + 4 + (k & 1) * 2, 4, 13 - (k & 1) * 2, pal[(k + s) % 4]);
	}

	// Philippine flag and a cork board
	rect(108, 20, 24, 15, INK);
	rect(109, 21, 22, 6, RGB(0, 56, 168));
	rect(109, 27, 22, 7, RGB(206, 17, 38));
	rect(109, 21, 4, 13, WHITE);
	rect(113, 22, 3, 11, WHITE);
	rect(116, 24, 2, 7, WHITE);
	rect(110, 26, 2, 2, GOLD_UI);
	blob(108, 40, 26, 16, RGB(206, 160, 110), SHOP_WOOD_DK);
	rect(111, 43, 7, 6, PAPER);
	rect(120, 43, 6, 5, PINK_PALE);
	rect(114, 50, 8, 4, SKY_UI);

	// blackboard with chalk writing
	blob(134, 14, 132, 40, SHOP_WOOD, INK);
	rect(138, 18, 124, 30, RGB(48, 88, 76));
	rect(138, 18, 124, 2, RGB(70, 112, 98));
	pixTextC(182, 22, 2, "ABAKADA", chalk, 0, 0);
	pixTextC(170, 40, 1, "MAHAL KITA", PINK_PALE, 0, 0);
	drawHeart(238, 24, 2, PINK_LT);
	rect(236, 42, 22, 1, chalk);
	rect(236, 45, 14, 1, chalk);
	rect(138, 49, 124, 3, SHOP_WOOD_DK);                   // chalk tray
	rect(146, 47, 5, 2, chalk);
	rect(154, 47, 4, 2, PINK_LT);

	// window with red curtains
	rect(272, 12, 64, 3, SHOP_WOOD_DK);
	blob(284, 18, 40, 32, SHOP_WOOD, INK);
	rect(288, 22, 32, 24, RGB(190, 226, 240));
	rect(303, 22, 2, 24, SHOP_WOOD);
	rect(288, 33, 32, 2, SHOP_WOOD);
	rect(292, 26, 9, 3, WHITE);
	rect(310, 38, 8, 3, WHITE);
	rect(276, 14, 12, 40, curtain);
	rect(279, 14, 2, 40, curtainDk);
	rect(320, 14, 12, 40, curtain);
	rect(323, 14, 2, 40, curtainDk);
	rect(282, 50, 48, 3, SHOP_WOOD_DK);

	// potted plant in the corner
	blob(318, FLOOR_Y + 4, 16, 16, LEAF_MID, LEAF_DK);
	rect(320, FLOOR_Y + 6, 6, 3, LEAF_HI);
	blob(320, FLOOR_Y + 20, 12, 10, RGB(196, 118, 90), INK);

	// exit doorway and mat
	rect(SCR_W / 2 - 26, ROOM_Y + ROOM_H, 52, 3, SHOP_GLOW);
	blob(SCR_W / 2 - 22, ROOM_Y + ROOM_H - 14, 44, 12, MINT_UI, INK);
	pixTextC(SCR_W / 2, ROOM_Y + ROOM_H - 11, 1, "EXIT", PAPER, 0, 0);

	// Sir Dante stands in front of the bookshelf (nobody can walk behind him)
	drawSchoolTeacher(1, SCHOOL_NPCS[1].x, 110);
	schoolEmote(1);
}

static void drawSchoolFurniture(int i) {
	const int *f = FURN[SCHOOL_ID][i];
	int x = f[0], y = f[1], w = f[2], h = f[3], bot = y + h;
	u32 wood = RGB(214, 150, 84), woodHi = RGB(244, 190, 118), woodDk = RGB(146, 92, 58);
	rect(x + 2, bot - 2, w, 4, SHADOW);

	if (i == FUR_COUNTER) { // the teacher's desk, with Miss Luz standing behind it
		drawSchoolTeacher(0, SCHOOL_NPCS[0].x, y - 4);
		schoolEmote(0);
		blob(x, y - 14, w, h + 14, wood, INK);
		rect(x + 1, y - 13, w - 2, 6, woodHi);
		rect(x + 6, y - 3, 38, 15, woodDk);
		rect(x + 7, y - 2, 36, 13, wood);
		rect(x + 52, y - 3, 38, 15, woodDk);
		rect(x + 53, y - 2, 36, 13, wood);
		rect(x + 22, y + 3, 4, 2, GOLD_UI);
		rect(x + 68, y + 3, 4, 2, GOLD_UI);
		rect(x + 8, y - 20, 22, 9, INK);                    // open book
		rect(x + 9, y - 19, 10, 7, PAPER);
		rect(x + 20, y - 19, 9, 7, PAPER);
		rect(x + 11, y - 17, 6, 1, INK_SOFT);
		rect(x + 11, y - 15, 6, 1, INK_SOFT);
		rect(x + 22, y - 17, 5, 1, INK_SOFT);
		blob(x + 33, y - 21, 9, 9, RGB(224, 64, 72), INK);  // an apple for the teacher
		rect(x + 37, y - 24, 2, 3, LEAF_DK);
		blob(x + 70, y - 28, 14, 14, RGB(96, 160, 210), INK); // globe
		rect(x + 74, y - 25, 5, 4, LEAF_MID);
		rect(x + 76, y - 14, 2, 3, INK);
		return;
	}

	// a student sitting behind a desk: chair, child, then the desk top over the lap
	static const int LOOK[SCHOOL_NPC_COUNT] = { 0, 0, 3, 5, 2 };
	int n = FUR_NPC[i];
	int cx = x + w / 2, feet = bot - 2;
	u32 chair = RGB(96, 156, 120);
	rect(cx - 10, bot - 25, 20, 11, INK);
	rect(cx - 9, bot - 24, 18, 9, chair);
	Kid kid = { .look = LOOK[n] };
	drawKid(&kid, cx, feet);
	schoolEmote(n);

	blob(x, bot - 16, w, 10, wood, INK);
	rect(x + 1, bot - 15, w - 2, 2, woodHi);
	rect(x + 6, bot - 14, 15, 7, INK);                     // open book
	rect(x + 7, bot - 13, 6, 5, PAPER);
	rect(x + 14, bot - 13, 6, 5, PAPER);
	rect(x + 9, bot - 11, 3, 1, INK_SOFT);
	rect(x + w - 14, bot - 11, 8, 1, GOLD_UI);             // pencil
	rect(x + w - 15, bot - 11, 1, 1, ROSE);
	rect(x + 3, bot - 6, 3, 6, woodDk);                    // desk legs
	rect(x + w - 6, bot - 6, 3, 6, woodDk);
}

static void drawFurniture(int i) {
	if (shopId == SCHOOL_ID) { drawSchoolFurniture(i); return; }
	if (shopId >= 3) { drawStreetFurniture(i); return; }
	if (shopId == 1) { drawBakeryFurniture(i); return; }
	if (shopId == 2) { drawSariFurniture(i); return; }
	const int *f = FURN[shopId][i];
	int x = f[0], y = f[1], w = f[2], h = f[3];
	rect(x + 2, y + h - 2, w, 4, SHADOW);
	if (i == FUR_COUNTER) {
		blob(x, y - 14, w, h + 14, SHOP_WOOD, INK);
		rect(x + 1, y - 13, w - 2, 7, SHOP_WOOD_HI);
		for (int k = 1; k < 5; k++) rect(x + k * 16, y - 5, 1, h + 3, SHOP_WOOD_DK);
		drawPot(x + 8, y - 15, STONE_HI, flowerCols[2]);
		rect(x + 48, y - 20, 18, 10, INK);           // cash box
		rect(x + 49, y - 19, 16, 8, GOLD_UI);
		rect(x + 52, y - 23, 8, 4, STONE_DK);
	} else if (i == FUR_TABLE) {
		blob(x, y - 8, w, 14, SHOP_WOOD_HI, INK);
		rect(x + 4, y + 6, 4, h - 6, SHOP_WOOD_DK);
		rect(x + w - 8, y + 6, 4, h - 6, SHOP_WOOD_DK);
		drawPot(x + 18, y - 7, RGB(176, 206, 236), flowerCols[3]);
		rect(x + 6, y - 5, 6, 4, PAPER);             // tea cup
		rect(x + 7, y - 5, 4, 1, SHOP_WOOD);
	} else { // tiered flower stands
		blob(x, y - 6, w, 10, SHOP_WOOD, INK);
		blob(x + 2, y + 6, w - 4, h - 6, SHOP_WOOD_DK, INK);
		int c = (i == FUR_STAND_L) ? 0 : 2;
		drawPot(x + 3, y - 7, RGB(196, 118, 90), flowerCols[c]);
		drawPot(x + 13, y - 10, STONE_HI, flowerCols[c + 1]);
		drawPot(x + 23, y - 7, RGB(196, 118, 90), flowerCols[c + 2]);
	}
}

// older woman with a graying hairdo, lilac blouse and a green apron
static void drawOwner(void) {
	int x = OWNER_X, fy = OWNER_FEET;
	int myStyle = styleIndex, mySkirt = skirtIndex, myShoes = shoeIndex;
	styleIndex = 0; skirtIndex = 0; shoeIndex = 0;
	drawPlayerSprite(x, fy, 0, false, RGB(235, 183, 162), RGB(150, 138, 142), RGB(204, 174, 232));
	styleIndex = myStyle; skirtIndex = mySkirt; shoeIndex = myShoes;
	blob(x - 5, fy - SPR_H - 2, 10, 8, RGB(150, 138, 142), INK); // hair bun
	rect(x + 2, fy - SPR_H, 4, 3, ROSE);                          // flower in her hair
	blob(x - 6, fy - 17, 12, 14, MINT_UI, INK);                   // apron
	drawHeart(x - 3, fy - 14, 1, PAPER);
}

static void drawFlowerRoom(void);
static void drawInteriorActors(void);

static void drawInterior(void) {
	rect(0, 0, SCR_W, SCR_H, RGB(48, 34, 42));

	if (shopId == SCHOOL_ID) drawSchoolRoom();
	else if (shopId >= 3) drawStreetRoom();
	else if (shopId == 2) drawSariRoom();
	else if (shopId == 1) drawBakeryRoom();
	else drawFlowerRoom();

	drawRoomDecor();
	drawInteriorActors();
}

static void drawFlowerRoom(void) {
	// striped wallpaper and wainscot
	rect(ROOM_X - 3, ROOM_Y - 3, ROOM_W + 6, ROOM_H + 6, INK);
	rect(ROOM_X, ROOM_Y, ROOM_W, 64, RGB(250, 226, 222));
	for (int k = 0; k < ROOM_W; k += 16) rect(ROOM_X + k + 4, ROOM_Y, 6, 48, RGB(244, 210, 212));
	rect(ROOM_X, ROOM_Y + 48, ROOM_W, 16, SHOP_WOOD);
	rect(ROOM_X, ROOM_Y + 48, ROOM_W, 2, SHOP_WOOD_HI);
	rect(ROOM_X, FLOOR_Y - 2, ROOM_W, 2, SHOP_WOOD_DK);

	// plank floor
	rect(ROOM_X, FLOOR_Y, ROOM_W, ROOM_H - 64, DECK_HI);
	for (int row = 0; row * 12 < ROOM_H - 64; row++) {
		int y = FLOOR_Y + row * 12;
		rect(ROOM_X, y, ROOM_W, 1, DECK_MID);
		for (int x = ROOM_X + 10 + (row % 3) * 20; x < ROOM_X + ROOM_W; x += 60)
			rect(x, y + 1, 1, 11, DECK_MID);
	}

	// sunny windows
	for (int k = 0; k < 2; k++) {
		int wx = ROOM_X + 22 + k * 200;
		blob(wx, ROOM_Y + 8, 44, 34, SHOP_WOOD, INK);
		rect(wx + 4, ROOM_Y + 12, 36, 26, RGB(198, 228, 238));
		rect(wx + 21, ROOM_Y + 12, 2, 26, SHOP_WOOD);
		rect(wx + 4, ROOM_Y + 24, 36, 2, SHOP_WOOD);
		rect(wx - 2, ROOM_Y + 42, 48, 3, SHOP_WOOD_DK);
	}

	// wall shelf full of flowers
	rect(150, ROOM_Y + 36, 100, 4, SHOP_WOOD_DK);
	for (int k = 0; k < 6; k++)
		drawPot(154 + k * 16, ROOM_Y + 31, (k & 1) ? STONE_HI : RGB(196, 118, 90), flowerCols[k % 5]);
	drawHeartO(196, ROOM_Y + 8, 2, ROSE, PAPER);

	// round rug
	blob(160, 128, 80, 40, PINK_LT, PINK_DK);
	blob(168, 134, 64, 28, PINK_PALE, PINK_MID);
	drawHeart(197, 144, 1, PINK_MID);

	// exit doorway and mat
	rect(SCR_W / 2 - 26, ROOM_Y + ROOM_H, 52, 3, SHOP_GLOW);
	blob(SCR_W / 2 - 22, ROOM_Y + ROOM_H - 14, 44, 12, MINT_UI, INK);
	pixTextC(SCR_W / 2, ROOM_Y + ROOM_H - 11, 1, "EXIT", PAPER, 0, 0);

	drawOwner(); // drawn before the counter so it covers her lower half
}

static void drawInteriorActors(void) {
	// furniture and player, drawn back to front
	int pcxs = (int)inX + PLAYER_SIZE / 2, pfeet = (int)inY + PLAYER_SIZE;
	bool drawn = false;
	for (int i = 0; i < FUR_COUNT; i++) {
		if (!drawn && pfeet <= FURN[shopId][i][1] + FURN[shopId][i][3]) {
			drawPlayerEntity(pcxs, pfeet);
			drawn = true;
		}
		drawFurniture(i);
	}
	if (!drawn) drawPlayerEntity(pcxs, pfeet);

	if (!shopMenu && !dialogActive) { drawHotbar(pcxs, pfeet, true); drawFullnessHud(); }
	drawEatFx(pcxs, pfeet);

	bool greet = dialogLine < eventLines();
	int li = greet ? 0 : dialogLine - eventLines();
	if (shopMenu)
		drawBakeryMenu();
	else if (shopId == SCHOOL_ID && dialogActive && talkingSchool >= 0)
		drawDialogBox(SCHOOL_NPCS[talkingSchool].name, greet ? eventGreeting() : dialogSchool[talkingSchool][li]);
	else if (dialogActive && talkingToOwner) {
		const char *line = greet ? eventGreeting() :
		                   shopId >= 3 ? dialogStreet[shopId - 3][li] :
		                   shopId == 2 ? dialogSari[li] :
		                   shopId == 1 ? dialogBaker[li] : dialogOwner[li];
		drawDialogBox(SHOP_KEEPER[shopId], line);
	}
	else if (shopId == SCHOOL_ID && nearSchoolNpc() >= 0) {
		const SchoolNpc *sn = &SCHOOL_NPCS[nearSchoolNpc()];
		int sx = sn->x < 42 ? 42 : sn->x > SCR_W - 42 ? SCR_W - 42 : sn->x;
		int sy = sn->top - 26 < 4 ? 4 : sn->top - 26;
		drawPrompt(sx, sy, "A TALK");
	}
	else if (nearOwner())
		drawPrompt(npcX() + 70, 62, "A TALK");
	else if (atExit())
		drawPrompt(SCR_W / 2 + 70, SCR_H - 34, "A EXIT");
}

// soft pink journal colors
#define J_LINE   RGB(216, 128, 160) // outlines
#define J_TEXT   RGB(168, 76, 116)  // headings
#define J_FILL   RGB(240, 160, 188) // selected tab
#define J_CARD   RGB(255, 242, 246)
#define J_SELECT RGB(255, 214, 228)
#define J_RULE   RGB(246, 202, 216)

// a five-petal pixel cherry blossom, about 11*s pixels wide, centred on (x, y)
// Built from a few large blocks: the 3DS can only draw so many shapes per frame.
static void drawBlocks(int x, int y, int s, const signed char (*b)[4], int n, u32 c) {
	for (int i = 0; i < n; i++)
		rect(x + b[i][0] * s, y + b[i][1] * s, b[i][2] * s, b[i][3] * s, c);
}

static void drawSakura(int x, int y, int s, u32 petal, u32 edge) {
	// each block is { x, y, w, h } around the centre
	static const signed char PETALS[15][4] = {
		{ -5, -1, 11, 2 }, { -2, -5, 5, 3 }, { -3, -1, 3, 6 }, { 1, -1, 3, 6 }, { -6, -2, 4, 2 }, { 3, -2, 4, 2 }, { -1, -5, 3, 7 }, { -1, -6, 1, 12 }, { 1, -6, 1, 12 }, { -5, -3, 2, 1 }, { 4, -3, 2, 1 }, { -4, 3, 1, 2 }, { 1, 3, 4, 2 }, { -3, -2, 1, 8 }, { 3, -2, 1, 8 }
	};
	static const signed char SHADE[2][4] = {
		{ -1, -1, 3, 3 }, { -1, 0, 4, 1 }
	};
	static const signed char CENTRE[1][4] = {
		{ 0, 0, 1, 1 }
	};
	drawBlocks(x, y, s, PETALS, 15, petal);
	drawBlocks(x, y, s, SHADE, 2, edge);
	drawBlocks(x, y, s, CENTRE, 1, GOLD_UI);
}

// soft pink paper scattered with blossoms and loose petals
static void drawSakuraBg(int w) {
	for (int y = 0; y < 240; y += 6)
		rect(0, y, w, 6, mixc(RGB(250, 206, 222), RGB(255, 228, 236), y / 240.0f));
	for (int row = 0; row < 5; row++)
		for (int col = 0; col * 56 < w + 28; col++) {
			int x = col * 56 + ((row & 1) ? 28 : 0) + 6;
			int y = row * 54 + 12;
			drawSakura(x, y, 1, RGB(255, 246, 249), RGB(244, 160, 190));
			rect(x + 24, y + 22, 3, 2, RGB(244, 172, 198)); // loose petal
			rect(x + 25, y + 24, 2, 1, RGB(244, 172, 198));
		}
}

// larger blossom cluster for the corners of cards and screens
static void drawSakuraBunch(int x, int y) {
	drawSakura(x, y, 2, RGB(250, 172, 200), RGB(228, 112, 156));
	drawSakura(x + 17, y - 5, 1, WHITE, RGB(240, 140, 178));
	drawSakura(x + 6, y + 15, 1, RGB(255, 214, 228), RGB(236, 130, 170));
}

static void drawTextC(int cx, float y, float scale, u32 clr, const char *s) {
	C2D_Text t;
	float w = 0, h = 0;
	C2D_TextParse(&t, textBuf, s);
	C2D_TextOptimize(&t);
	C2D_TextGetDimensions(&t, scale, scale, &w, &h);
	C2D_DrawText(&t, C2D_WithColor, cx - w / 2, y, 0.5f, scale, scale, clr);
}

static void drawBigPlayer(int cx, int top, int scale, int dir, bool walking) {
	gsc = scale; gox = cx; goy = top;
	drawPlayerSprite(0, SPR_H, dir, walking,
	                 skinTones[skinIndex], hairColors[hairIndex], outfitColors[outfitIndex]);
	drawAccessory(0, SPR_H, dir);
	gsc = 1; gox = 0; goy = 0;
}

static void drawTape(int x, int y) {
	rect(x, y, 34, 9, RGBA(250, 176, 190, 215));
	for (int k = 3; k < 34; k += 8) rect(x + k, y, 3, 9, RGBA(255, 240, 236, 150));
}

// top screen: portrait, full outfit and profile card
static void drawJournalTop(void) {
	char date[32];
	drawSakuraBg(SCR_W);

	pill(110, 6, 180, 24, J_CARD, J_LINE);
	pixTextC(200, 11, 2, "MY JOURNAL", J_TEXT, 0, 0);

	// instant-photo portrait
	rect(15, 45, 120, 176, SHADOW);
	rect(12, 42, 120, 176, WHITE);
	rect(18, 48, 108, 134, RGB(252, 220, 232));
	spriteRows = 28;
	drawBigPlayer(72, 66, 4, 0, false);
	spriteRows = SPR_H;
	drawTextC(72, 190, 0.6f, INK, playerName);
	drawTape(55, 37);

	// full outfit, turning slowly
	static const int TURN[4] = { 0, 3, 1, 2 };
	blob(142, 42, 86, 176, J_CARD, J_LINE);
	blob(153, 188, 64, 12, J_RULE, J_RULE);
	drawBigPlayer(185, 72, 3, TURN[(frameCount / 90) % 4], (frameCount / 90) % 2 == 1);
	drawTape(168, 37);

	// profile card
	blob(238, 42, 150, 176, J_CARD, J_LINE);
	const char *heads[4] = { "PRONOUNS", "BIRTHDAY", "HOME", "STATUS" };
	snprintf(date, sizeof(date), "%s %d, %d", MONTH_NAMES[monthIndex], dayIndex + 1, FIRST_YEAR + yearIndex);
	const char *vals[4] = { PRONOUN_NAMES[pronounIndex], date, homeName, STATUS_NAMES[statusIndex] };
	for (int i = 0; i < 4; i++) {
		int y = 52 + i * 41;
		drawHeart(248, y, 1, ROSE);
		pixText(260, y, 1, heads[i], J_TEXT, 0, 0);
		drawText(248, y + 12, 0.46f, INK, vals[i]);
		if (i < 3) rect(248, y + 33, 130, 1, J_RULE);
	}
	drawTape(296, 37);

	drawSakuraBunch(14, 14);
	drawSakuraBunch(368, 18);
	drawSakuraBunch(14, 222);
	drawSakuraBunch(366, 224);
	drawSakura(92, 18, 1, WHITE, RGB(240, 140, 178));
	drawSakura(306, 18, 1, WHITE, RGB(240, 140, 178));
}

static void drawArrow(int x, int y, bool right) {
	for (int k = 0; k < 5; k++)
		rect(right ? x + k : x + 4 - k, y + k, 1, 9 - 2 * k, J_TEXT);
}

// bottom screen: touch tabs and option rows
static void drawJournalBottom(void) {
	char num[16];
	drawSakuraBg(BOT_W);

	for (int t = 0; t < JOURNAL_TABS; t++) {
		bool on = (t == journalTab);
		int x = 2 + t * (TAB_W + 2);
		pill(x, TAB_Y, TAB_W, TAB_H, on ? J_FILL : J_CARD, J_LINE);
		pixTextC(x + TAB_W / 2, TAB_Y + 10, 1, TAB_NAMES[t], on ? WHITE : J_TEXT, 0, 0);
	}

	for (int i = 0; i < TAB_ROWS[journalTab]; i++) {
		const JournalRow *r = &JOURNAL[journalTab][i];
		int y = ROW_Y + i * ROW_STEP;
		bool sel = (i == journalRow);
		pill(6, y, 308, ROW_H, sel ? J_SELECT : J_CARD, sel ? J_TEXT : J_LINE);
		pixText(16, y + 8, 1, r->label, INK, 0, 0);
		if (r->text) {
			drawTextC(210, y + 3, 0.5f, INK, r->text);
			pixText(272, y + 8, 1, "TYPE", J_TEXT, 0, 0);
			continue;
		}
		drawArrow(118, y + 6, false);
		drawArrow(298, y + 6, true);
		const char *val = num;
		if (r->names) val = r->names[*r->value];
		else snprintf(num, sizeof(num), "%d", *r->value + (r->value == &yearIndex ? FIRST_YEAR : 1));
		if (r->colors) blob(146, y + 4, 14, 14, r->colors[*r->value], INK);
		drawTextC(r->colors ? 224 : 210, y + 3, 0.5f, INK, val);
	}

	if (journalTab == 1) { // little mirror hint on the shorter page
		drawTextC(160, 140, 0.45f, INK, "Your new look is shown on the top screen.");
	}

	if (journalTab != 2) { // the About Me page fills this space with rows
		drawSakuraBunch(34, 174);
		drawSakuraBunch(272, 180);
	}

	pill(4, 214, 70, 20, J_CARD, J_LINE);
	pill(246, 214, 70, 20, J_CARD, J_LINE);

	pill(100, DONE_Y, 120, 24, J_FILL, J_TEXT);
	pixTextC(160, DONE_Y + 5, 2, "DONE", WHITE, 0, 0);
	drawText(14, 218, 0.4f, INK, "L/R: page");
	drawText(258, 218, 0.4f, INK, "B: close");
}

static void drawTopScreen(void) {
	if (showCustomize) {
		drawJournalTop();
	} else if (gameState == STATE_MENU) {
		drawTitleScene(true);
		if (activeEvent != EV_NONE) {
			bool hw = activeEvent == EV_HALLOWEEN;
			pill(70, 208, 260, 26, hw ? RGB(110, 62, 140) : RGB(196, 56, 78), PLUM);
			pixTextC(200, 214, 2, hw ? "HAPPY HALLOWEEN!" : "MALIGAYANG PASKO!",
			         hw ? RGB(255, 178, 80) : PAPER, 0, PLUM);
			if (hw) { drawPumpkin(52, 232, 2); drawPumpkin(348, 232, 2); }
			else { drawParol(52, 218, 0); drawParol(348, 218, 1); }
		}
	} else if (insideHouse) {
		drawInterior();
	} else {
		drawWorld();
	}
}

static void drawHowToBottom(void) {
	pixTextC(160, 12, 3, "HOW TO PLAY", WHITE, PINK_MID, MAUVE);
	pill(14, 48, 292, 148, PAPER, PINK_DK);
	u32 ink = INK;
	drawText(28, 58, 0.5f, ink, "Explore the garden at your own pace.");
	drawText(28, 76, 0.5f, ink, "Chat with Jay and visit the shops.");
	drawText(28, 94, 0.5f, ink, "Pet the askal and puspin with A!");
	drawText(28, 106, 0.45f, ink, "Hold food in the hotbar, press Y to eat!");
	drawText(28, 118, 0.45f, INK_SOFT, "Move: Circle Pad / D-Pad     Hotbar: L / R");
	drawText(28, 134, 0.45f, INK_SOFT, "Talk / shop: A    Journal: B    Eat: Y");
	drawText(28, 150, 0.45f, INK_SOFT, "Pause: SELECT / MENU     Weather: tap icon");
	drawText(28, 170, 0.45f, PINK_DK, "Mahal kita means \"I love you\" in Filipino.");
	drawText(100, 204, 0.45f, MAUVE, "Tap or press A to go back");
}

static void drawMenuBottom(void) {
	drawPinkBottomBg();
	if (showHowTo) {
		drawHowToBottom();
	} else {
		pixTextC(160, 12, 3, "MAHAL KITA", WHITE, PINK_MID, MAUVE);
		for (int i = 0; i < BTN_COUNT; i++) {
			const char *label = MENU_BTNS[i].label;
			if (i == BTN_PLAY && hasStartedOnce) label = "CONTINUE";
			if (i == BTN_SAVE && saveMsgTimer > 0 && saveMsg) label = saveMsg;
			drawMenuButton(&MENU_BTNS[i], label, i == selectedBtn);
		}
	}
	drawLace(212, BOT_W);
}

static const char *musicError = NULL; // shown on the bottom screen if music can't play

static void drawPlayBottom(void) {
	drawPinkBottomBg();
	blob(8, 8, 304, 184, PAPER, INK);
	rect(10, 10, 300, 3, PINK_PALE);
	pixTextC(160, 18, 3, "MAHAL KITA", PINK_DK, 0, PINK_PALE);
	drawHeartO(157, 54, 3, ROSE, PLUM);

	// clock: sun / moon icon, time of day and the name of this part of the day
	{
		int ph = dayPhase();
		int hh = (int)(dayTime * 24.0f) % 24;
		int mm = (int)((dayTime * 24.0f - (float)hh) * 60.0f);
		char tbuf[16];
		snprintf(tbuf, sizeof(tbuf), "%d:%02d %s", hh % 12 == 0 ? 12 : hh % 12, mm, hh < 12 ? "AM" : "PM");
		static const char *PH_NAME[4] = { "NIGHT", "SUNRISE", "DAY", "SUNSET" };
		int ix = CLK_X + 6, iy = CLK_Y + 6;
		if (ph == PH_NIGHT) {
			blob(ix, iy, 18, 18, RGB(206, 212, 255), PLUM);      // moon
			rect(ix + 5, iy + 5, 3, 3, RGB(170, 178, 232));
			rect(ix + 10, iy + 10, 4, 3, RGB(170, 178, 232));
			rect(ix + 22, iy + 2, 2, 2, GOLD_UI);                 // stars
			rect(ix + 26, iy + 9, 2, 2, GOLD_UI);
		} else if (ph == PH_DAY) {
			blob(ix + 2, iy + 2, 14, 14, GOLD_UI, PLUM);          // sun
			rect(ix + 8, iy - 3, 2, 3, GOLD_UI);
			rect(ix + 8, iy + 18, 2, 3, GOLD_UI);
			rect(ix - 3, iy + 8, 3, 2, GOLD_UI);
			rect(ix + 18, iy + 8, 3, 2, GOLD_UI);
		} else {
			blob(ix + 1, iy + 1, 16, 16, RGB(255, 150, 100), PLUM); // low sun
			rect(ix - 4, iy + 12, 26, 6, PAPER);                    // horizon
			rect(ix - 4, iy + 12, 26, 2, GRASS_B);
		}
		drawText(CLK_X + 34, CLK_Y + 4, 0.55f, INK, tbuf);
		drawText(CLK_X + 34, CLK_Y + 22, 0.45f, PINK_DK, PH_NAME[ph]);
	}

	// seasonal event button: shows which event is on and how many treats you have
	{
		static const char *MODE_NAME[EVM_COUNT] = { "AUTO", "HALLOWEEN", "PASKO", "OFF" };
		bool hw = activeEvent == EV_HALLOWEEN, xm = activeEvent == EV_PASKO;
		pill(EVB_X, EVB_Y, EVB_W, EVB_H, hw ? RGB(250, 206, 150) : xm ? RGB(196, 230, 200) : PINK_PALE,
		     PINK_DK);
		char l1[32], l2[32];
		snprintf(l1, sizeof(l1), "Event: %s", MODE_NAME[eventMode]);
		if (activeEvent == EV_NONE) snprintf(l2, sizeof(l2), "No event now");
		else snprintf(l2, sizeof(l2), "%s %d/%d", hw ? "Candy" : "Aguinaldo", giftsGot(), GIVER_COUNT);
		drawText(EVB_X + 10, EVB_Y + 5, 0.4f, INK, l1);
		drawText(EVB_X + 10, EVB_Y + 21, 0.4f, PLUM, l2);
	}

	const char *where = "You are in the village.";
	static const char *INSIDE[SHOP_COUNT] = {
		"You are inside the flower shop.", "You are inside the bakery.",
		"You are inside the sari-sari store.", "You are inside the ukay-ukay.",
		"You are inside the pet shop.", "You are inside the karenderya.",
		"You are inside the school."
	};
	if (insideHouse) where = INSIDE[shopId];
	else if (py >= 61 * TS) where = "You are at the beach. Ang ganda ng dagat!";
	else if (py >= 44 * TS) where = "You are on the street. Ingat sa kalsada!";
	drawText(22, 86, 0.5f, INK, where);
	char wallet[48];
	int bread = 0;
	for (int i = 0; i < ITEM_COUNT; i++) bread += itemTotal(i);
	snprintf(wallet, sizeof(wallet), "Coins: P%d     Items in bag: %d", coins, bread);
	drawText(22, 102, 0.45f, PINK_DK, wallet);
	drawText(22, 120, 0.45f, INK_SOFT, "Move: Circle Pad / D-Pad     Hotbar: L / R");
	drawText(22, 138, 0.45f, INK_SOFT, "Talk: A      Next line: A      Close: B");
	drawText(22, 156, 0.45f, INK_SOFT, "Enter/leave shop: A    Journal: B    Eat: Y");
	if (musicError) drawText(22, 174, 0.4f, ROSE_DK, musicError);

	pill(PLAY_MENU_X, PLAY_MENU_Y, PLAY_MENU_W, PLAY_MENU_H, PINK_LT, PINK_DK);
	pixTextC(PLAY_MENU_X + PLAY_MENU_W / 2, PLAY_MENU_Y + 8, 2, "MENU", WHITE, 0, PLUM);
	drawWeatherBtn();
	drawText(228, 203, 0.4f, MAUVE, "START:");
	drawText(228, 215, 0.4f, MAUVE, "quit game");
}

// ---------------------------------------------------------------- background music
// Loops a 16-bit PCM WAV file (mono or stereo) on NDSP channel 0.
#define MUSIC_CHANNEL 0
#define MUSIC_VOLUME 0.7f             // 0.0 = silent, 1.0 = full
#define MUSIC_MAX_BYTES (8 * 1024 * 1024)

static s16 *musicData = NULL;
static ndspWaveBuf musicBuf;
static bool ndspReady = false;

static u32 readLE(const u8 *p, int bytes) {
	u32 v = 0;
	for (int i = 0; i < bytes; i++) v |= (u32)p[i] << (8 * i);
	return v;
}

// Reads the samples into linear memory (which the DSP needs).
static bool loadWav(FILE *f, u32 *rate, u32 *channels, u32 *bytes) {
	u8 hdr[12], ck[8], fmt[16];
	bool haveFmt = false;
	if (fread(hdr, 1, 12, f) != 12 || memcmp(hdr, "RIFF", 4) || memcmp(hdr + 8, "WAVE", 4))
		return false;
	while (fread(ck, 1, 8, f) == 8) {
		u32 size = readLE(ck + 4, 4);
		long skip = (long)size + (size & 1); // chunks are padded to even sizes
		if (!memcmp(ck, "fmt ", 4) && size >= 16) {
			if (fread(fmt, 1, 16, f) != 16) return false;
			skip -= 16;
			*channels = readLE(fmt + 2, 2);
			*rate = readLE(fmt + 4, 4);
			if (readLE(fmt, 2) != 1 || readLE(fmt + 14, 2) != 16) return false; // PCM16 only
			if (*channels < 1 || *channels > 2 || *rate == 0) return false;
			haveFmt = true;
		} else if (!memcmp(ck, "data", 4)) {
			if (!haveFmt || size < 4 || size > MUSIC_MAX_BYTES) return false;
			musicData = (s16 *)linearAlloc(size);
			if (!musicData) return false;
			if (fread(musicData, 1, size, f) != size) {
				linearFree(musicData);
				musicData = NULL;
				return false;
			}
			*bytes = size;
			return true;
		}
		if (fseek(f, skip, SEEK_CUR)) return false;
	}
	return false;
}

static void initMusic(void) {
	if (R_FAILED(ndspInit())) {
		musicError = "No music: dspfirm.cdc not loaded";
		return;
	}
	ndspReady = true;

	static const char *paths[] = { "romfs:/audio.wav", "sdmc:/3ds/audio.wav", "audio.wav" };
	FILE *f = NULL;
	for (int i = 0; i < 3 && !f; i++) f = fopen(paths[i], "rb");
	if (!f) {
		musicError = "No music: audio.wav not found";
		return;
	}
	u32 rate = 0, channels = 0, bytes = 0;
	bool ok = loadWav(f, &rate, &channels, &bytes);
	fclose(f);
	if (!ok) {
		musicError = "No music: audio.wav must be 16-bit PCM";
		return;
	}

	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnReset(MUSIC_CHANNEL);
	ndspChnSetInterp(MUSIC_CHANNEL, NDSP_INTERP_LINEAR);
	ndspChnSetRate(MUSIC_CHANNEL, (float)rate);
	ndspChnSetFormat(MUSIC_CHANNEL, channels == 2 ? NDSP_FORMAT_STEREO_PCM16 : NDSP_FORMAT_MONO_PCM16);

	float mix[12] = { 0 };
	mix[0] = MUSIC_VOLUME; // front left
	mix[1] = MUSIC_VOLUME; // front right
	ndspChnSetMix(MUSIC_CHANNEL, mix);

	memset(&musicBuf, 0, sizeof(musicBuf));
	musicBuf.data_vaddr = musicData;
	musicBuf.nsamples = bytes / (2 * channels);
	musicBuf.looping = true;
	DSP_FlushDataCache(musicData, bytes);
	ndspChnWaveBufAdd(MUSIC_CHANNEL, &musicBuf);
}

static void exitMusic(void) {
	if (ndspReady) {
		ndspChnWaveBufClear(MUSIC_CHANNEL);
		ndspExit();
		ndspReady = false;
	}
	if (musicData) {
		linearFree(musicData);
		musicData = NULL;
	}
}

int main(int argc, char **argv) {
	(void)argc; (void)argv;
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(MAX_SHAPES);
	C2D_Prepare();

	C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	C3D_RenderTarget *bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	textBuf = C2D_TextBufNew(4096);

	initGame();
	if (loadGame()) hasStartedOnce = true; // pick up where the last save left off
	bool romfsReady = R_SUCCEEDED(romfsInit());
	initMusic();

	while (aptMainLoop()) {
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (kDown & KEY_START) break;

		updateGame(kDown, kHeld);

		C2D_TextBufClear(textBuf);
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

		C2D_TargetClear(top, RGB(208, 200, 228));
		C2D_SceneBegin(top);
		drawTopScreen();

		C2D_TargetClear(bot, RGB(248, 214, 232));
		C2D_SceneBegin(bot);
		if (showCustomize) drawJournalBottom();
		else if (gameState == STATE_MENU) drawMenuBottom();
		else drawPlayBottom();

		C3D_FrameEnd(0);
	}

	exitMusic();
	if (romfsReady) romfsExit();
	C2D_TextBufDelete(textBuf);
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	return 0;
}
