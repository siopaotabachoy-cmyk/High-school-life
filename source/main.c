// Mahal Kita - Cozy pixel-art village game for Nintendo 3DS (libctru + citro2d)
// Controls: Circle Pad / D-Pad = move | A = enter / leave the cottage
//           A near Jay, Aling Rosa or Ate Mila (bakery) = talk | A = next line | B = close dialogue
//           Bakery menu: D-Pad Up/Down = pick | A = buy | B = leave
//           B = open / close My Journal (character customization)
//           In the journal: L/R = page | D-Pad = pick and change | A = change / type
//           L/R = pick a hotbar slot (bread you buy goes into the hotbar)
//           Touch = menu | SELECT = pause menu | START = quit
// Music: put audio.wav in romfs/ (or on the SD card at /3ds/audio.wav).
//        Sound also needs the DSP firmware file at sdmc:/3ds/dspfirm.cdc

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

#define RGB(r, g, b)    C2D_Color32((r), (g), (b), 255)
#define RGBA(r, g, b, a) C2D_Color32((r), (g), (b), (a))

#define PLAYER_SIZE 10
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
#define BAKE_COUNT 4

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
	T_WATER, T_BRIDGE, T_TABLE, T_BLANKET, T_SUNFLOWER
};

enum {
	STATE_MENU,
	STATE_PLAYING
};

enum {
	BTN_PLAY,
	BTN_CUSTOMIZE,
	BTN_HOWTO,
	BTN_RESTART,
	BTN_COUNT
};

typedef struct {
	int x, y, w, h;
	const char *label;
} MenuBtn;

static const MenuBtn MENU_BTNS[BTN_COUNT] = {
	{ 36,  46, 248, 34, "START" },
	{ 36,  86, 248, 34, "CUSTOMIZE" },
	{ 36, 126, 248, 34, "HOW TO PLAY" },
	{ 36, 166, 248, 34, "NEW GAME" }
};

// "MENU" button shown on the bottom screen while playing
#define PLAY_MENU_X 10
#define PLAY_MENU_Y 200
#define PLAY_MENU_W 100
#define PLAY_MENU_H 30

static u8 map[MAP_H][MAP_W];
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
	       t == T_TABLE;
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

// Keep sakura trees sparse and out of the way of paths, water and each other.
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
	for (int y = hy + 4; y < 44; y++) {
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
		int y = 1 + rand() % (MAP_H - 2);
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

	// Bakery on the west side of the village. It is stamped last (after the random
	// scatter) so the rest of the village looks exactly as before.
	for (int y = BAK_TY - 2; y <= BAK_TY + 6; y++)
		for (int x = BAK_TX - 2; x <= BAK_TX + 7; x++)
			map[y][x] = T_GRASS;
	for (int x = BAK_TX; x < BAK_TX + 6; x++) {
		map[BAK_TY][x] = T_ROOF;
		map[BAK_TY + 1][x] = T_ROOF;
		map[BAK_TY + 2][x] = T_WALL;
		map[BAK_TY + 3][x] = T_WALL;
	}
	map[BDOOR_TY][BDOOR_TX] = T_DOOR;
	for (int x = BAK_TX - 1; x <= BAK_TX + 6; x++) map[BAK_TY + 4][x] = T_PATH;
	for (int y = BAK_TY + 4; y <= 37; y++) {   // path down to the main road
		map[y][BDOOR_TX] = T_PATH;
		map[y][BDOOR_TX + 1] = T_PATH;
	}
}

// ---------------------------------------------------------------- drawing helpers
// Everything is drawn through rect(), which applies an optional integer scale/offset.
// That lets the same character code draw big on the customize preview.

static int gsc = 1, gox = 0, goy = 0;
static int gAlpha = 255; // 255 = opaque; the hotbar lowers this to fade out

static void rect(int x, int y, int w, int h, u32 c) {
	if (w <= 0 || h <= 0) return;
	if (gAlpha < 255) c = (c & 0x00FFFFFFu) | ((u32)((int)(c >> 24) * gAlpha / 255) << 24);
	C2D_DrawRectSolid(gox + x * gsc, goy + y * gsc, 0, w * gsc, h * gsc, c);
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

static void drawTile(u8 t, int sx, int sy, int tx, int ty) {
	u32 h = hash2(tx, ty);
	int swi = ((frameCount / 18 + tx + ty) % 3) - 1;

	// flat sage-green grass with soft patches, tufts and tiny daisies
	if (t != T_PATH && t != T_WATER && t != T_BLANKET) {
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
	case T_PATH: { // paved stone sidewalk with slab seams and a curb
		rect(sx, sy, TS, TS, PATH_MID);
		int seam = 3 + (int)((h >> 4) & 3) * 2;
		rect(sx, sy + 7, TS, 1, PATH_SEAM);
		rect(sx + seam, sy, 1, 7, PATH_SEAM);
		rect(sx + (seam + 8) % 16, sy + 8, 1, 8, PATH_SEAM);
		rect(sx + 2 + (h & 3), sy + 2, 5, 1, PATH_HI);
		rect(sx + 9 - ((h >> 5) & 3), sy + 10, 4, 1, PATH_HI);
		if (!isPathOrBridge(tx, ty - 1)) rect(sx, sy, TS, 1, PATH_EDGE);
		if (!isPathOrBridge(tx, ty + 1)) rect(sx, sy + TS - 2, TS, 2, PATH_EDGE);
		if (!isPathOrBridge(tx - 1, ty)) rect(sx, sy, 1, TS, PATH_EDGE);
		if (!isPathOrBridge(tx + 1, ty)) rect(sx + TS - 1, sy, 1, TS, PATH_EDGE);
		break;
	}
	case T_WATER: {
		rect(sx, sy, TS, TS, WATER_MID);
		rect(sx, sy, TS, 2, WATER_HI);
		rect(sx, sy + TS - 2, TS, 2, WATER_DK);
		int wave = ((frameCount / 12 + tx * 2 + ty) % 8) - 2;
		rect(sx + 3 + wave, sy + 6, 5, 1, WATER_HI);
		rect(sx + 7 - wave, sy + 11, 4, 1, WATER_HI);
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
	case T_TREE:
	case T_APPLE: // sakura trees are drawn in their own pass by drawTree()
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

// ---------------------------------------------------------------- game state

typedef struct {
	float x, y;
	float fx, fy;
	int wp;
	int wait;
} Npc;

// Jay strolls a loop around the cottage, crossing the front path below the door.
static const int WP[4][2] = { {26, 12}, {34, 12}, {34, 17}, {26, 17} };

static float px, py;
static float pfx = 0, pfy = 1;
static bool playerMoving = false;
static Npc jay;
static bool insideHouse = false;

// ---- bakery shop state
static int shopId = 0;                 // which shop we are inside: 0 = flower shop, 1 = bakery
static bool shopMenu = false;          // the bakery buy menu is open
static int shopSel = 0;
static const char *shopMsg = NULL;
static int coins = 100;
static const char *BAKE_NAMES[BAKE_COUNT] = { "Pandesal", "Ensaymada", "Pan de Coco", "Hopia" };
static const int BAKE_PRICE[BAKE_COUNT] = { 5, 15, 10, 8 };

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

static C2D_TextBuf textBuf;

static void drawText(float x, float y, float scale, u32 clr, const char *s) {
	C2D_Text t;
	C2D_TextParse(&t, textBuf, s);
	C2D_TextOptimize(&t);
	C2D_DrawText(&t, C2D_WithColor, x, y, 0.5f, scale, scale, clr);
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
	shopSel = 0;
	shopMsg = NULL;
	coins = 100;
	for (int i = 0; i < HOTBAR_SLOTS; i++) { invType[i] = -1; invCount[i] = 0; }
	hotbarSel = 0;
	hbNameTimer = 0;
	hbFade = 255;
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
static const int FURN[2][FUR_COUNT][4] = {
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

// true when the player is standing on the doormat indoors
static bool atExit(void) {
	float cx = inX + PLAYER_SIZE / 2.0f;
	return fabsf(cx - SCR_W / 2) < 24.0f && inY + PLAYER_SIZE > ROOM_Y + ROOM_H - 22;
}

// where the shopkeeper of the current shop stands (screen x)
static int npcX(void) { return shopId ? BAKER_X : OWNER_X; }

static int dialogCount(void) {
	if (!talkingToOwner) return JAY_LINES;
	return shopId ? BAKER_LINES : OWNER_LINES;
}

// true when the player is standing in front of the counter, near the shopkeeper
static bool nearOwner(void) {
	float cx = inX + PLAYER_SIZE / 2.0f;
	return fabsf(cx - npcX()) < 34.0f &&
	       inY >= FURN[shopId][FUR_COUNTER][1] && inY < FURN[shopId][FUR_COUNTER][1] + FURN[shopId][FUR_COUNTER][3] + 14;
}

static void enterHouse(int id) {
	shopId = id;
	shopMenu = false;
	insideHouse = true;
	inX = SCR_W / 2 - PLAYER_SIZE / 2;
	inY = ROOM_Y + ROOM_H - PLAYER_SIZE - 6;
	pfx = 0; pfy = -1;
	playerMoving = false;
}

static void leaveHouse(void) {
	insideHouse = false;
	int dtx = shopId ? BDOOR_TX : DOOR_TX, dty = shopId ? BDOOR_TY : DOOR_TY;
	px = dtx * TS + (TS - PLAYER_SIZE) / 2;
	py = (dty + 1) * TS + 1;
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

static void activateMenuBtn(int idx) {
	if (idx == BTN_PLAY) {
		showHowTo = false;
		hasStartedOnce = true;
		gameState = STATE_PLAYING;
	} else if (idx == BTN_CUSTOMIZE) {
		openJournal();
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

static void updateGame(u32 kDown, u32 kHeld) {
	frameCount++;
	if (hbNameTimer > 0) hbNameTimer--;

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
	}
	if (toMenu) {
		showHowTo = false;
		showCustomize = false;
		dialogActive = false;
		dialogLine = 0;
		shopMenu = false;
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
			if (coins >= BAKE_PRICE[shopSel]) {
				if (addItem(shopSel)) {
					coins -= BAKE_PRICE[shopSel];
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

	// Player movement and Jay's stroll pause during dialogue.
	if (dialogActive) {
		if (kDown & KEY_A) {
			dialogLine++;
			if (dialogLine >= dialogCount()) {
				bool toShop = talkingToOwner && shopId == 1;
				dialogActive = false;
				dialogLine = 0;
				if (toShop) { shopMenu = true; shopSel = 0; shopMsg = NULL; }
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

		if ((kDown & KEY_A) && nearOwner()) {
			talkingToOwner = true;
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
		updateJay(); // Jay keeps strolling outside
		return;
	}

	float nx = px + mx * speed;
	if (!blockedAt(nx, py)) px = nx;
	float ny = py + my * speed;
	if (!blockedAt(px, ny)) py = ny;

	// ---- enter the cottage (A) / talk to Jay (A). At the door, A always enters.
	if ((kDown & KEY_A) && atDoor()) {
		enterHouse(0);
		return;
	}
	if ((kDown & KEY_A) && atBakeryDoor()) {
		enterHouse(1);
		return;
	}
	if ((kDown & KEY_A) && nearJay()) {
		talkingToOwner = false;
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

	updateJay();
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

static u32 jayColor(char ch) {
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

// Big sakura tree. (sx, sy) is the top-left of its one-tile trunk; the canopy overhangs
// the tiles around it, so trees are drawn in their own pass after the ground.
static void drawTree(int sx, int sy, int tx, int ty) {
	u32 h = hash2(tx, ty);
	static const signed char BL[7][2] = {
		{ -2, -19 }, { 12, -16 }, { -7, -9 }, { 5, -8 }, { 18, -9 }, { 9, -3 }, { -1, -2 }
	};

	// ground shadow + fallen petals
	rect(sx - 4, sy + 11, 24, 5, SHADOW);
	for (int k = 0; k < 5; k++) {
		u32 q = hash2(tx * 7 + k, ty * 13 + k);
		rect(sx - 6 + (int)(q % 28), sy + 12 + (int)((q >> 8) % 6), 2, 1,
		     (k & 1) ? BLOSSOM_HI : BLOSSOM_PALE);
	}

	// trunk, root flare and branch stubs
	rect(sx + 6, sy + 4, 4, 12, BARK);
	rect(sx + 9, sy + 4, 1, 12, BARK_DK);
	rect(sx + 4, sy + 13, 8, 3, BARK);
	rect(sx + 2, sy + 15, 4, 1, BARK_DK);
	rect(sx + 10, sy + 15, 4, 1, BARK_DK);
	rect(sx + 2, sy + 7, 4, 2, BARK);
	rect(sx + 10, sy + 7, 4, 2, BARK);
	rect(sx - 1, sy + 8, 4, 2, BARK_DK);
	rect(sx + 13, sy + 8, 4, 2, BARK_DK);

	// shaded underside of the canopy
	blob(sx - 9, sy - 6, 34, 13, BLOSSOM_DK, BLOSSOM_DK);
	rect(sx - 7, sy + 6, 4, 3, BLOSSOM_DK);
	rect(sx + 19, sy + 5, 4, 4, BLOSSOM_DK);
	rect(sx + 3, sy + 6, 3, 2, BLOSSOM_DK);
	rect(sx + 12, sy + 6, 3, 2, BLOSSOM_DK);

	// main pink mass
	blob(sx - 10, sy - 15, 36, 17, BLOSSOM_MID, BLOSSOM_MID);
	blob(sx - 5, sy - 22, 26, 12, BLOSSOM_MID, BLOSSOM_MID);

	// sunlit clumps
	blob(sx - 4, sy - 21, 13, 7, BLOSSOM_HI, BLOSSOM_HI);
	blob(sx + 11, sy - 18, 11, 7, BLOSSOM_HI, BLOSSOM_HI);
	blob(sx - 9, sy - 12, 10, 7, BLOSSOM_HI, BLOSSOM_HI);
	blob(sx + 17, sy - 11, 8, 6, BLOSSOM_HI, BLOSSOM_HI);
	blob(sx + 3, sy - 10, 12, 6, BLOSSOM_HI, BLOSSOM_HI);

	// deeper pockets for depth
	rect(sx - 4, sy - 3, 8, 3, BLOSSOM_DK);
	rect(sx + 12, sy - 4, 7, 3, BLOSSOM_DK);
	rect(sx - 1 + (int)(h & 3), sy - 14, 4, 2, BLOSSOM_DK);

	// individual blossoms with golden centres
	for (int k = 0; k < 7; k++) {
		int bx = sx + BL[k][0] + (int)((h >> k) & 1);
		int by = sy + BL[k][1];
		rect(bx - 1, by, 3, 1, BLOSSOM_PALE);
		rect(bx, by - 1, 1, 3, BLOSSOM_PALE);
		rect(bx, by, 1, 1, BLOSSOM_GOLD);
	}
}

static void drawJayEntity(int sx, int sfeet, bool walking) {
	drawJaySprite(sx, sfeet, dirFromFacing(jay.fx, jay.fy), walking);
}

static void drawPlayerEntity(int pcxs, int pfeet) {
	drawPlayerSprite(pcxs, pfeet, dirFromFacing(pfx, pfy), playerMoving,
	                 skinTones[skinIndex], hairColors[hairIndex], outfitColors[outfitIndex]);
	drawAccessory(pcxs, pfeet, dirFromFacing(pfx, pfy));
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
		bun(sx + 3, sy + 5, 16, 12, breadIconColor(invType[i]), BREAD_DK);
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
		const char *nm = BAKE_NAMES[invType[hotbarSel]];
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

static void drawWorld(void) {
	float pcx = px + PLAYER_SIZE / 2.0f;
	float pcy = py + PLAYER_SIZE / 2.0f;
	int camx = (int)(pcx - SCR_W / 2);
	int camy = (int)(pcy - SCR_H / 2);
	if (camx < 0) camx = 0;
	if (camy < 0) camy = 0;
	if (camx > MAP_W * TS - SCR_W) camx = MAP_W * TS - SCR_W;
	if (camy > MAP_H * TS - SCR_H) camy = MAP_H * TS - SCR_H;

	int tx0 = camx / TS, ty0 = camy / TS;
	for (int ty = ty0; ty <= ty0 + SCR_H / TS + 1 && ty < MAP_H; ty++)
		for (int tx = tx0; tx <= tx0 + SCR_W / TS + 1 && tx < MAP_W; tx++)
			drawTile(map[ty][tx], tx * TS - camx, ty * TS - camy, tx, ty);

	// only draw a building when it is actually on screen (saves citro2d shapes)
	int cbx = COTTAGE_TX * TS - camx, cby = COTTAGE_TY * TS - camy;
	if (cbx > -110 && cbx < SCR_W + 10 && cby > -30 && cby < SCR_H + 10) drawCottage(cbx, cby);
	int bbx = BAK_TX * TS - camx, bby = BAK_TY * TS - camy;
	if (bbx > -110 && bbx < SCR_W + 10 && bby > -30 && bby < SCR_H + 10) drawBakery(bbx, bby);

	int sx = (int)jay.x - camx, sy = (int)jay.y - camy;

	int ppx = (int)px - camx, ppy = (int)py - camy;
	int pcxs = ppx + PLAYER_SIZE / 2, pfeet = ppy + PLAYER_SIZE;
	int sfeet = sy + PLAYER_SIZE / 2;
	bool jayWalk = jay.wait == 0 && !dialogActive;

	// Characters and tree canopies are drawn back-to-front by the line they stand on,
	// so you can walk behind a sakura tree as well as in front of it.
	int feet[2], kind[2], ei = 0;
	if (sfeet <= pfeet) { feet[0] = sfeet; kind[0] = 0; feet[1] = pfeet; kind[1] = 1; }
	else                { feet[0] = pfeet; kind[0] = 1; feet[1] = sfeet; kind[1] = 0; }

	for (int ty = ty0; ty <= ty0 + SCR_H / TS + 2 && ty < MAP_H; ty++) {
		for (int tx = tx0 - 1; tx <= tx0 + SCR_W / TS + 1; tx++) {
			if (tx < 0 || tx >= MAP_W) continue;
			u8 tt = map[ty][tx];
			if (tt != T_TREE && tt != T_APPLE) continue;
			int tsx = tx * TS - camx, tsy = ty * TS - camy;
			while (ei < 2 && feet[ei] <= tsy + TS) {
				if (kind[ei] == 0) drawJayEntity(sx, sfeet, jayWalk);
				else drawPlayerEntity(pcxs, pfeet);
				ei++;
			}
			drawTree(tsx, tsy, tx, ty);
		}
	}
	while (ei < 2) {
		if (kind[ei] == 0) drawJayEntity(sx, sfeet, jayWalk);
		else drawPlayerEntity(pcxs, pfeet);
		ei++;
	}

	drawVignette();
	if (!dialogActive) drawHotbar(pcxs, pfeet, false);

	if (!dialogActive) {
		if (atDoor())
			drawPrompt(DOOR_TX * TS + TS / 2 - camx, (DOOR_TY + 1) * TS + 18 - camy, "A ENTER");
		else if (atBakeryDoor())
			drawPrompt(BDOOR_TX * TS + TS / 2 - camx, (BDOOR_TY + 1) * TS + 18 - camy, "A ENTER");
		else if (nearJay())
			drawPrompt(sx, sfeet - SPR_H - 22, "A TALK");
	}

	if (dialogActive) drawDialogBox("JAY", dialogJay[dialogLine]);
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
static void drawBakeryMenu(void) {
	char buf[32];

	blob(60, 18, 280, 204, PAPER, INK);
	rect(62, 20, 276, 3, PINK_PALE);
	pill(120, 8, 160, 22, PINK_LT, PINK_DK);
	pixTextC(200, 12, 2, "BAKERY", WHITE, 0, PLUM);

	for (int i = 0; i < BAKE_COUNT; i++) {
		int y = 40 + i * 34;
		bool sel = (i == shopSel);
		blob(72, y, 256, 30, sel ? PINK_PALE : PAPER, sel ? PINK_DK : PAPER_DK);
		if (sel) drawHeart(78, y + 11, 1, ROSE);
		bun(94, y + 9, 16, 12, breadIconColor(i), BREAD_DK);
		drawText(118, y + 6, 0.6f, INK, BAKE_NAMES[i]);
		snprintf(buf, sizeof(buf), "P%d", BAKE_PRICE[i]);
		drawText(240, y + 6, 0.6f, PINK_DK, buf);
		snprintf(buf, sizeof(buf), "x%d", itemTotal(i));
		drawText(292, y + 8, 0.5f, INK_SOFT, buf);
	}

	snprintf(buf, sizeof(buf), "Coins: P%d", coins);
	drawText(74, 178, 0.55f, PLUM, buf);
	drawText(238, 182, 0.42f, INK_SOFT, "A: buy    B: leave");
	if (shopMsg) drawText(74, 198, 0.5f, ROSE_DK, shopMsg);
}

static void drawFurniture(int i) {
	if (shopId == 1) { drawBakeryFurniture(i); return; }
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

	if (shopId == 1) drawBakeryRoom();
	else drawFlowerRoom();

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

	if (!shopMenu && !dialogActive) drawHotbar(pcxs, pfeet, true);

	if (shopMenu)
		drawBakeryMenu();
	else if (dialogActive && talkingToOwner)
		drawDialogBox(shopId ? "ATE MILA" : "ALING ROSA", shopId ? dialogBaker[dialogLine] : dialogOwner[dialogLine]);
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
	drawText(28, 76, 0.5f, ink, "Chat with Jay while he takes his stroll.");
	drawText(28, 94, 0.5f, ink, "Visit the flower shop and the bakery!");
	drawText(28, 118, 0.45f, INK_SOFT, "Move: Circle Pad / D-Pad     Hotbar: L / R");
	drawText(28, 134, 0.45f, INK_SOFT, "Talk / enter shop: A      My Journal: B");
	drawText(28, 150, 0.45f, INK_SOFT, "Pause: SELECT or MENU");
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

	drawText(22, 86, 0.5f, INK, insideHouse ? (shopId ? "You are inside the bakery." : "You are inside the flower shop.") : "You are in the garden.");
	char wallet[48];
	int bread = 0;
	for (int i = 0; i < BAKE_COUNT; i++) bread += itemTotal(i);
	snprintf(wallet, sizeof(wallet), "Coins: P%d     Bread in bag: %d", coins, bread);
	drawText(22, 102, 0.45f, PINK_DK, wallet);
	drawText(22, 120, 0.45f, INK_SOFT, "Move: Circle Pad / D-Pad     Hotbar: L / R");
	drawText(22, 138, 0.45f, INK_SOFT, "Talk: A      Next line: A      Close: B");
	drawText(22, 156, 0.45f, INK_SOFT, "Enter or leave shop: A      My Journal: B");
	if (musicError) drawText(22, 174, 0.4f, ROSE_DK, musicError);

	pill(PLAY_MENU_X, PLAY_MENU_Y, PLAY_MENU_W, PLAY_MENU_H, PINK_LT, PINK_DK);
	pixTextC(PLAY_MENU_X + PLAY_MENU_W / 2, PLAY_MENU_Y + 8, 2, "MENU", WHITE, 0, PLUM);
	drawText(176, 210, 0.4f, MAUVE, "START: quit game");
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
