// Mahal Kita - Cozy pixel-art stealth game for Nintendo 3DS (libctru + citro2d)
// Controls: Circle Pad / D-Pad = move | L = sneak | A = give gift / advance dialogue
//           B = talk to Senpai / close dialogue | Touch = menu | SELECT = pause menu | START = quit

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

#define NUM_GIFTS 8
#define PLAYER_SIZE 10

// ---- Cozy pastel garden & stream palette
#define INK        RGB(54, 38, 48)
#define INK_SOFT   RGB(102, 76, 72)
#define PAPER      RGB(254, 244, 224)
#define PAPER_DK   RGB(226, 202, 168)
#define WOOD_HI    RGB(232, 180, 118)
#define WOOD_MID   RGB(190, 132, 84)
#define WOOD_DK    RGB(128, 82, 54)
#define GRASS_A    RGB(168, 206, 134)
#define GRASS_B    RGB(156, 196, 124)
#define PATH_HI    RGB(248, 226, 182)
#define PATH_MID   RGB(232, 202, 154)
#define PATH_EDGE  RGB(186, 154, 114)
#define WATER_HI   RGB(178, 228, 238)
#define WATER_MID  RGB(142, 204, 222)
#define WATER_DK   RGB(108, 174, 198)
#define SHADOW     RGBA(44, 52, 54, 64)
#define SKIN       RGB(255, 226, 198)
#define SKIN_SHD   RGB(228, 186, 158)
#define ROSE       RGB(240, 112, 140)
#define SKY_UI     RGB(118, 168, 214)
#define MINT_UI    RGB(122, 196, 148)
#define GOLD_UI    RGB(248, 202, 88)

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
	STATE_PLAYING,
	STATE_WIN
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
	{ 36, 166, 248, 34, "NEW RUN" }
};

// "MENU" button shown on the bottom screen while playing
#define PLAY_MENU_X 10
#define PLAY_MENU_Y 200
#define PLAY_MENU_W 100
#define PLAY_MENU_H 30

static u8 map[MAP_H][MAP_W];
static u8 reach[MAP_H][MAP_W];
static u32 flowerCols[5];
static int frameCount = 0;
static int gameState = STATE_MENU;
static bool hasStartedOnce = false;
static bool showHowTo = false;
static bool showCustomize = false;
static int customCategory = 0;
static int skinIndex = 2;
static int hairIndex = 0;
static int outfitIndex = 0;
// C2D_Color32 is a function-like color constructor, so initialize these at runtime.
static u32 skinTones[5];
static u32 hairColors[4];
static u32 outfitColors[4];

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

static void genMap(void) {
	srand(1234);
	memset(map, T_GRASS, sizeof(map));

	// Main horizontal garden path
	for (int x = 0; x < MAP_W; x++)
		for (int y = 37; y <= 38; y++) map[y][x] = T_PATH;

	// Cozy cottage / garden pavilion in clearing
	const int hx = 28, hy = 13;
	for (int x = hx; x < hx + 6; x++) {
		map[hy][x] = T_ROOF;
		map[hy + 1][x] = T_ROOF;
		map[hy + 2][x] = T_WALL;
		map[hy + 3][x] = T_WALL;
	}
	map[hy + 3][hx + 3] = T_DOOR;

	// Path connecting clearing down to the stream bridge
	for (int y = hy + 4; y < 44; y++) {
		map[y][hx + 3] = T_PATH;
		map[y][hx + 4] = T_PATH;
	}

	// White picket fence around picnic garden
	for (int x = 24; x <= 35; x++) {
		map[11][x] = T_FENCE;
		if (x != hx + 3 && x != hx + 4) map[22][x] = T_FENCE;
	}
	for (int y = 11; y <= 22; y++) {
		map[y][24] = T_FENCE;
		map[y][35] = T_FENCE;
	}

	// Picnic table, benches & checkered picnic blanket near the clearing
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

	// Border trees
	for (int x = 0; x < MAP_W; x++) { map[0][x] = T_TREE; map[MAP_H - 1][x] = T_TREE; }
	for (int y = 0; y < MAP_H; y++) { map[y][0] = T_TREE; map[y][MAP_W - 1] = T_TREE; }

	// Scatter garden props, flowers & trees outside protected areas
	for (int i = 0; i < 900; i++) {
		int x = 1 + rand() % (MAP_W - 2);
		int y = 1 + rand() % (MAP_H - 2);
		if (map[y][x] != T_GRASS) continue;
		if (inYard(x, y)) continue;
		if (x >= 27 && x <= 36 && y >= 32 && y <= 36) continue;
		int r = rand() % 100;
		if (r < 34) map[y][x] = T_TREE;
		else if (r < 42) map[y][x] = T_APPLE;
		else if (r < 50) map[y][x] = T_ROCK;
		else if (r < 62) map[y][x] = T_SUNFLOWER;
		else if (r < 76) map[y][x] = T_MUSH;
		else map[y][x] = T_FLOWER;
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
}

// Flood fill from the spawn tile so gifts are never placed where the player can't reach.
static void computeReach(int sx, int sy) {
	static int q[MAP_W * MAP_H];
	static const int dx[4] = { 1, -1, 0, 0 };
	static const int dy[4] = { 0, 0, 1, -1 };
	memset(reach, 0, sizeof(reach));
	int head = 0, tail = 0;
	q[tail++] = sy * MAP_W + sx;
	reach[sy][sx] = 1;
	while (head < tail) {
		int c = q[head++];
		int x = c % MAP_W, y = c / MAP_W;
		for (int k = 0; k < 4; k++) {
			int nx = x + dx[k], ny = y + dy[k];
			if (nx < 0 || ny < 0 || nx >= MAP_W || ny >= MAP_H) continue;
			if (reach[ny][nx] || solid(map[ny][nx])) continue;
			reach[ny][nx] = 1;
			q[tail++] = ny * MAP_W + nx;
		}
	}
}

// ---------------------------------------------------------------- drawing helpers
// Everything is drawn through rect(), which applies an optional integer scale/offset.
// That lets the same character code draw big on the customize preview.

static int gsc = 1, gox = 0, goy = 0;

static void rect(int x, int y, int w, int h, u32 c) {
	if (w <= 0 || h <= 0) return;
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

static void drawTile(u8 t, int sx, int sy, int tx, int ty) {
	u32 h = hash2(tx, ty);
	int swi = ((frameCount / 18 + tx + ty) % 3) - 1;

	if (t != T_PATH && t != T_ROOF && t != T_WALL && t != T_DOOR && t != T_WATER) {
		rect(sx, sy, TS, TS, ((tx + ty) & 1) ? GRASS_A : GRASS_B);
		if ((h & 3) == 0) {
			int ox = ((h >> 4) % 9) + 3;
			int oy = ((h >> 8) % 9) + 3;
			rect(sx + ox, sy + oy + 1, 3, 1, RGB(120, 174, 96));
			rect(sx + ox + 1, sy + oy, 1, 2, RGB(120, 174, 96));
		}
	}

	switch (t) {
	case T_PATH: {
		rect(sx, sy, TS, TS, PATH_MID);
		rect(sx + 2 + (h & 3), sy + 3 + ((h >> 2) & 3), 4, 3, PATH_HI);
		rect(sx + 9 - ((h >> 4) & 2), sy + 9 - ((h >> 5) & 2), 4, 3, PATH_HI);
		if (!isPathOrBridge(tx, ty - 1)) rect(sx, sy, TS, 2, PATH_EDGE);
		if (!isPathOrBridge(tx, ty + 1)) rect(sx, sy + TS - 2, TS, 2, PATH_EDGE);
		if (!isPathOrBridge(tx - 1, ty)) rect(sx, sy, 2, TS, PATH_EDGE);
		if (!isPathOrBridge(tx + 1, ty)) rect(sx + TS - 2, sy, 2, TS, PATH_EDGE);
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
	case T_BRIDGE: {
		rect(sx, sy, TS, TS, WATER_MID);
		blob(sx, sy + 1, TS, TS - 2, WOOD_MID, INK);
		rect(sx + 1, sy + 2, TS - 2, 2, WOOD_HI);
		rect(sx + 3, sy + 2, 1, TS - 4, WOOD_DK);
		rect(sx + 8, sy + 2, 1, TS - 4, WOOD_DK);
		rect(sx + 12, sy + 2, 1, TS - 4, WOOD_DK);
		break;
	}
	case T_TABLE: {
		rect(sx + 1, sy + 12, 14, 3, SHADOW);
		blob(sx + 1, sy + 3, 14, 8, WOOD_MID, INK);
		rect(sx + 2, sy + 4, 12, 2, WOOD_HI);
		rect(sx + 2, sy + 9, 12, 2, WOOD_DK);
		blob(sx + 2, sy + 12, 12, 3, WOOD_HI, INK);
		if ((tx & 1) == 0) {
			rect(sx + 4, sy + 5, 4, 3, PAPER);
			rect(sx + 5, sy + 5, 2, 3, SKY_UI);
		} else {
			rect(sx + 9, sy + 5, 3, 3, PAPER);
			rect(sx + 10, sy + 6, 1, 1, ROSE);
		}
		break;
	}
	case T_BLANKET: {
		rect(sx + 1, sy + 13, 14, 2, SHADOW);
		blob(sx + 1, sy + 3, 14, 11, PAPER, INK);
		for (int gx = 2; gx < 14; gx += 4) {
			for (int gy = 4; gy < 13; gy += 4) {
				rect(sx + gx, sy + gy, 2, 2, RGBA(242, 134, 158, 180));
			}
		}
		if ((tx & 1) == 0) {
			blob(sx + 5, sy + 5, 6, 5, GOLD_UI, INK);
			rect(sx + 7, sy + 4, 2, 2, PAPER);
		}
		break;
	}
	case T_SUNFLOWER: {
		rect(sx + 4, sy + 13, 8, 2, SHADOW);
		rect(sx + 7, sy + 6 + swi / 2, 2, 8, RGB(88, 156, 76));
		rect(sx + 5, sy + 10, 2, 2, RGB(108, 174, 88));
		rect(sx + 9, sy + 9, 2, 2, RGB(108, 174, 88));
		int hy = sy + 2 + swi / 2;
		blob(sx + 4 + swi / 2, hy, 8, 8, GOLD_UI, INK);
		rect(sx + 6 + swi / 2, hy + 2, 4, 4, WOOD_DK);
		break;
	}
	case T_TREE:
	case T_APPLE:
		rect(sx + 2, sy + 12, 12, 3, SHADOW);
		blob(sx + 6, sy + 8, 4, 7, WOOD_MID, INK);
		blob(sx, sy - 4, 16, 13, RGB(98, 178, 112), INK);
		rect(sx + 2, sy + 5, 12, 3, RGB(72, 148, 92));
		rect(sx + 3, sy - 2, 7, 4, RGB(142, 214, 132));
		if (t == T_APPLE) {
			rect(sx + 3, sy + 2, 3, 3, ROSE);
			rect(sx + 10, sy + 1, 3, 3, ROSE);
			rect(sx + 7, sy + 5, 3, 3, ROSE);
		}
		break;
	case T_ROCK:
		rect(sx + 2, sy + 12, 12, 3, SHADOW);
		blob(sx + 2, sy + 5, 12, 9, RGB(172, 178, 196), INK);
		rect(sx + 4, sy + 4, 6, 3, RGB(212, 218, 232));
		rect(sx + 4, sy + 11, 8, 2, RGB(138, 144, 164));
		break;
	case T_ROOF: {
		int row = ty - 13;
		u32 rFill = row == 0 ? RGB(224, 124, 102) : RGB(204, 104, 84);
		rect(sx, sy, TS, TS, rFill);
		rect(sx, sy + 1, TS, 2, RGB(242, 154, 124));
		rect(sx, sy + 14, TS, 2, RGB(148, 66, 58));
		rect(sx + 7, sy, 1, TS, RGB(158, 72, 62));
		if (tx == 32 && row == 0) {
			blob(sx + 3, sy - 6, 10, 10, WOOD_MID, INK);
			rect(sx + 4, sy - 5, 8, 2, WOOD_HI);
		}
		break;
	}
	case T_WALL:
		rect(sx, sy, TS, TS, PAPER);
		if (tx == 28 || tx == 33) rect(sx + (tx == 28 ? 0 : 14), sy, 2, TS, WOOD_MID);
		if (ty == 15) rect(sx, sy, TS, 3, SHADOW);
		if (ty == 15 && (tx == 29 || tx == 31 || tx == 33)) {
			blob(sx + 2, sy + 4, 12, 9, WOOD_MID, INK);
			rect(sx + 4, sy + 6, 8, 5, WATER_HI);
			rect(sx + 4, sy + 6, 3, 2, PAPER);
			rect(sx + 7, sy + 6, 2, 5, WOOD_DK);
		}
		break;
	case T_DOOR:
		rect(sx, sy, TS, TS, PAPER);
		blob(sx + 2, sy + 1, 12, 15, WOOD_MID, INK);
		rect(sx + 4, sy + 3, 3, 5, WOOD_HI);
		rect(sx + 9, sy + 3, 3, 5, WOOD_HI);
		rect(sx + 10, sy + 9, 2, 2, GOLD_UI);
		break;
	case T_FENCE:
		rect(sx, sy + 13, TS, 3, SHADOW);
		rect(sx, sy + 5, TS, 2, PAPER);
		rect(sx, sy + 7, TS, 1, PAPER_DK);
		rect(sx, sy + 10, TS, 2, PAPER);
		for (int k = 0; k < 2; k++) {
			int x = sx + 2 + k * 8;
			blob(x, sy + 2, 4, 12, PAPER, INK);
		}
		break;
	case T_MUSH:
		rect(sx + 3, sy + 12, 10, 3, SHADOW);
		blob(sx + 6, sy + 8, 4, 6, PAPER, INK);
		blob(sx + 3, sy + 3, 10, 7, ROSE, INK);
		rect(sx + 5, sy + 5, 2, 2, PAPER);
		rect(sx + 9, sy + 6, 2, 2, PAPER);
		break;
	case T_FLOWER:
		for (int k = 0; k < 2; k++) {
			int fx = sx + 4 + k * 7 + ((h >> (k * 4)) & 1);
			int fy = sy + 5 + ((h >> (k * 3 + 2)) & 2);
			u32 c = flowerCols[(h >> (k * 5 + 1)) % 5];
			rect(fx, fy + 2, 1, 5, RGB(84, 156, 82));
			blob(fx - 2 + swi, fy - 2, 5, 5, c, INK);
			rect(fx + swi, fy, 1, 1, GOLD_UI);
		}
		break;
	default:
		break;
	}
}

// ---------------------------------------------------------------- game state

typedef struct {
	float x, y;
	float fx, fy;
	int wp;
	int wait;
	float sus;
	int happy;
} Senpai;

typedef struct {
	float x, y;
	int state; // 0 = world, 1 = carried, 2 = given
} Gift;

// Senpai walks a loop around the cottage, crossing the front path below the door.
static const int WP[4][2] = { {26, 12}, {34, 12}, {34, 17}, {26, 17} };

static float px, py;
static float pfx = 0, pfy = 1;
static bool playerMoving = false, playerSneak = false;
static Senpai sen;
static Gift gifts[NUM_GIFTS];
static int given = 0;
static int caughtTimer = 0;

// ---- NPC dialogue
static bool dialogActive = false;
static int dialogLine = 0;
static const char *dialogEarly[] = {
	"Oh, hello there!", "It is a lovely day in the garden.",
	"Are those gifts for me?", "I will be waiting here!"
};
static const char *dialogMid[] = {
	"You are back!", "Thank you for the lovely gifts.",
	"You really are thoughtful.", "I will treasure them!"
};
static const char *dialogLate[] = {
	"You have brought me so many gifts!", "You always make me smile.",
	"Thank you for everything!", "You are wonderful!"
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

	flowerCols[0] = RGB(164, 186, 252); // soft periwinkle hydrangea
	flowerCols[1] = RGB(252, 218, 102); // warm buttercup
	flowerCols[2] = RGB(254, 154, 188); // soft blossom
	flowerCols[3] = RGB(204, 174, 252); // pastel lilac
	flowerCols[4] = RGB(255, 250, 240); // cream daisy

	genMap();
	resetPlayer();
	computeReach((int)(px / TS), (int)(py / TS));
	pfx = 0;
	pfy = 1;

	sen.x = WP[3][0] * TS + TS / 2;
	sen.y = WP[3][1] * TS + TS / 2;
	sen.fx = 0; sen.fy = -1;
	sen.wp = 0;
	sen.wait = 60;
	sen.sus = 0;
	sen.
