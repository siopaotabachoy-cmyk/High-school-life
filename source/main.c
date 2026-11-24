// Mossy Hollow - Cozy pixel-art stealth game for Nintendo 3DS (libctru + citro2d)
// Controls: Circle Pad / D-Pad = move | L = sneak | A = give gift / advance dialogue | B = talk / close dialogue | Touch = menu | START = quit

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
	{ 36, 62,  248, 34, "PLAY GAME" },
	{ 36, 102, 248, 34, "CUSTOMIZE" },
	{ 36, 142, 248, 34, "HOW TO PLAY" },
	{ 36, 182, 248, 34, "NEW RUN" }
};

static u8 map[MAP_H][MAP_W];
static u32 flowerCols[5];
static int frameCount = 0;
static int gameState = STATE_MENU;
static bool hasStartedOnce = false;
static bool showHowTo = false;
static bool showCustomize = false;
static int customCategory = 0;
static int skinIndex = 3;
static int hairIndex = 0;
static int outfitIndex = 0;
static const u32 skinTones[] = { RGB(255, 226, 198), RGB(232, 181, 139), RGB(190, 128, 86), RGB(135, 82, 55), RGB(92, 55, 42) };
static const u32 hairColors[] = { RGB(54, 38, 48), RGB(110, 66, 48), RGB(190, 132, 72), RGB(190, 80, 90) };
static const u32 outfitColors[] = { RGB(112, 148, 190), RGB(166, 126, 176), RGB(122, 170, 142), RGB(190, 112, 118) };

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

// ---------------------------------------------------------------- drawing helpers

UNUSED_FN static void rect(int x, int y, int w, int h, u32 c) {
	if (w <= 0 || h <= 0) return;
	C2D_DrawRectSolid(x, y, 0, w, h, c);
}

UNUSED_FN static void card(int x, int y, int w, int h, u32 fill, u32 bevel, u32 border) {
	rect(x + 1, y, w - 2, h, border);
	rect(x, y + 1, w, h - 2, border);
	rect(x + 1, y + 1, w - 2, h - 2, fill);
	rect(x + 1, y + h - 3, w - 2, 2, bevel);
	rect(x + w - 3, y + 1, 2, h - 2, bevel);
}

UNUSED_FN static void blob(int x, int y, int w, int h, u32 fill, u32 out) {
	rect(x + 1, y, w - 2, h, out);
	rect(x, y + 1, w, h - 2, out);
	rect(x + 1, y + 1, w - 2, h - 2, fill);
}

UNUSED_FN static void drawHeart(int x, int y, int s, u32 c) {
	rect(x, y, 2 * s, 2 * s, c);
	rect(x + 4 * s, y, 2 * s, 2 * s, c);
	rect(x - s, y + s, 8 * s, 2 * s, c);
	rect(x, y + 3 * s, 6 * s, s, c);
	rect(x + s, y + 4 * s, 4 * s, s, c);
	rect(x + 2 * s, y + 5 * s, 2 * s, s, c);
}

static u32 shadeColor(u32 c, int dr, int dg, int db) {
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
		// Bench in front
		blob(sx + 2, sy + 12, 12, 3, WOOD_HI, INK);
		// Little book / teacup prop on alternating table tiles
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

static const int WP[4][2] = { {26, 13}, {34, 13}, {34, 19}, {26, 19} };

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
	flowerCols[0] = RGB(164, 186, 252); // soft periwinkle hydrangea
	flowerCols[1] = RGB(252, 218, 102); // warm buttercup
	flowerCols[2] = RGB(254, 154, 188); // soft blossom
	flowerCols[3] = RGB(204, 174, 252); // pastel lilac
	flowerCols[4] = RGB(255, 250, 240); // cream daisy

	genMap();
	resetPlayer();
	pfx = 0;
	pfy = 1;

	sen.x = WP[3][0] * TS + TS / 2;
	sen.y = WP[3][1] * TS + TS / 2;
	sen.fx = 0; sen.fy = -1;
	sen.wp = 0;
	sen.wait = 60;
	sen.sus = 0;
	sen.happy = 0;
	given = 0;
	caughtTimer = 0;
	dialogActive = false;
	dialogLine = 0;

	int placed = 0, tries = 0;
	while (placed < NUM_GIFTS && tries < 5000) {
		tries++;
		int x = 2 + rand() % (MAP_W - 4);
		int y = 2 + rand() % (MAP_H - 4);
		if (solid(map[y][x]) || inYard(x, y) || map[y][x] == T_PATH || map[y][x] == T_BRIDGE) continue;
		gifts[placed].x = x * TS + TS / 2;
		gifts[placed].y = y * TS + TS / 2;
		gifts[placed].state = 0;
		placed++;
	}
}

static bool blockedAt(float x, float y) {
	return solid(tileAt(x, y)) || solid(tileAt(x + PLAYER_SIZE - 1, y)) ||
	       solid(tileAt(x, y + PLAYER_SIZE - 1)) ||
	       solid(tileAt(x + PLAYER_SIZE - 1, y + PLAYER_SIZE - 1));
}

static bool los(float x0, float y0, float x1, float y1) {
	float dx = x1 - x0, dy = y1 - y0;
	int n = (int)(sqrtf(dx * dx + dy * dy) / 4.0f) + 1;
	for (int i = 1; i < n; i++) {
		float sx = x0 + dx * i / n;
		float sy = y0 + dy * i / n;
		if (solid(tileAt(sx, sy))) return false;
	}
	return true;
}

static void updateSenpai(void) {
	if (sen.happy > 0) sen.happy--;
	if (sen.wait > 0) { sen.wait--; return; }

	float tx = WP[sen.wp][0] * TS + TS / 2;
	float ty = WP[sen.wp][1] * TS + TS / 2;
	float dx = tx - sen.x, dy = ty - sen.y;
	float d = sqrtf(dx * dx + dy * dy);
	if (d < 1.5f) {
		sen.wp = (sen.wp + 1) % 4;
		sen.wait = 60;
		return;
	}
	sen.fx = dx / d;
	sen.fy = dy / d;
	sen.x += sen.fx * 0.55f;
	sen.y += sen.fy * 0.55f;
}

static void caught(void) {
	caughtTimer = 150;
	resetPlayer();
	sen.sus = 0;
	for (int i = 0; i < NUM_GIFTS; i++)
		if (gifts[i].state == 1) gifts[i].state = 0;
}

static void activateMenuBtn(int idx) {
	if (idx == BTN_PLAY) {
		showHowTo = false;
		hasStartedOnce = true;
		gameState = (given >= NUM_GIFTS) ? STATE_WIN : STATE_PLAYING;
	} else if (idx == BTN_CUSTOMIZE) {
        showCustomize = true;
        showHowTo = false;
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
    if (showCustomize) {
        if (kDown & (KEY_B | KEY_SELECT)) { showCustomize = false; return; }
        if (kDown & KEY_DUP) customCategory = (customCategory + 2) % 3;
        if (kDown & KEY_DDOWN) customCategory = (customCategory + 1) % 3;
        if (kDown & (KEY_DLEFT | KEY_DRIGHT)) {
            int dir = (kDown & KEY_DRIGHT) ? 1 : -1;
            if (customCategory == 0) skinIndex = (skinIndex + 5 + dir) % 5;
            if (customCategory == 1) hairIndex = (hairIndex + 4 + dir) % 4;
            if (customCategory == 2) outfitIndex = (outfitIndex + 4 + dir) % 4;
        }
        if (kDown & KEY_A) showCustomize = false;
        if (kDown & KEY_TOUCH) {
            touchPosition tp; hidTouchRead(&tp);
            if (tp.py > 75 && tp.py < 125) customCategory = (tp.px < 105) ? 0 : ((tp.px < 210) ? 1 : 2);
            else if (tp.py >= 130 && tp.py < 185) {
                int v = customCategory == 0 ? (tp.px - 10) / 61 : (tp.px - 22) / 70;
                if (v < 0) v = 0;
                int maxV = customCategory == 0 ? 4 : 3;
                if (v > maxV) v = maxV;
                if (customCategory == 0) skinIndex = v;
                else if (customCategory == 1) hairIndex = v;
                else outfitIndex = v;
            } else if (tp.py >= 190) showCustomize = false;
        }
        return;
    }
    if (showHowTo) {
        if (kDown & (KEY_A | KEY_B | KEY_SELECT)) showHowTo = false;
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

static const char *getDialogue(int line) {
    if (line < 0 || line >= 4) return "...";
    if (given >= 6) return dialogLate[line];
    if (given >= 3) return dialogMid[line];
    return dialogEarly[line];
}

static void updateGame(u32 kDown, u32 kHeld) {
	frameCount++;

	if (kDown & KEY_SELECT) {
		showHowTo = false;
        showCustomize = false;
		dialogActive = false;
		dialogLine = 0;
		gameState = STATE_MENU;
		return;
	}

	if (gameState == STATE_MENU) {
		updateSenpai();
		updateMenu(kDown);
		return;
	}

	if (gameState == STATE_WIN) {
		if (kDown & KEY_A) {
			initGame();
			gameState = STATE_PLAYING;
		}
		return;
	}

	// Pause player movement and Senpai's patrol during dialogue.
	if (dialogActive) {
		if (kDown & KEY_A) {
			dialogLine++;
			if (dialogLine >= 4) {
				dialogActive = false;
				dialogLine = 0;
			}
		} else if (kDown & KEY_B) {
			dialogActive = false;
			dialogLine = 0;
		}
		return;
	}

	playerMoving = false;
	if (caughtTimer > 0) { caughtTimer--; return; }

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

	bool sneaking = (kHeld & KEY_L) != 0;
	playerSneak = sneaking;
	float speed = sneaking ? 0.8f : 1.7f;

	float nx = px + mx * speed;
	if (!blockedAt(nx, py)) px = nx;
	float ny = py + my * speed;
	if (!blockedAt(px, ny)) py = ny;

	float pcx = px + PLAYER_SIZE / 2.0f;
	float pcy = py + PLAYER_SIZE / 2.0f;

	// Press B near Senpai to start a conversation.
	float talkDx = sen.x - pcx;
	float talkDy = sen.y - pcy;
	float talkDist = sqrtf(talkDx * talkDx + talkDy * talkDy);
	if ((kDown & KEY_B) && talkDist < 40.0f && sen.happy == 0) {
		dialogActive = true;
		dialogLine = 0;
		playerMoving = false;
		return;
	}

	for (int i = 0; i < NUM_GIFTS; i++) {
		if (gifts[i].state != 0) continue;
		float dx = gifts[i].x - pcx, dy = gifts[i].y - pcy;
		if (dx * dx + dy * dy < 100) gifts[i].state = 1;
	}

	float sdx = sen.x - pcx, sdy = sen.y - pcy;
	float sdist = sqrtf(sdx * sdx + sdy * sdy);
	if ((kDown & KEY_A) && sdist < 24) {
		int n = 0;
		for (int i = 0; i < NUM_GIFTS; i++)
			if (gifts[i].state == 1) { gifts[i].state = 2; n++; }
		if (n > 0) {
			given += n;
			sen.happy = 120;
			sen.wait = 90;
			sen.sus = 0;
			if (given >= NUM_GIFTS) gameState = STATE_WIN;
		}
	}

	updateSenpai();

	float range = sneaking ? 55.0f : 100.0f;
	bool seen = false;
	if (sen.happy == 0 && sdist < range && sdist > 0.1f) {
		float dot = (-sdx * sen.fx + -sdy * sen.fy) / sdist;
		if (dot > 0.77f && los(sen.x, sen.y, pcx, pcy)) seen = true;
	}
	if (seen) {
		sen.sus += 0.6f + 1.2f * (1.0f - sdist / range);
		if (sen.sus >= 100.0f) caught();
	} else {
		sen.sus -= 0.5f;
		if (sen.sus < 0) sen.sus = 0;
	}
}

// ---------------------------------------------------------------- top screen drawing

static void drawChar(int cx, int fy, float fx, float fyv, bool walking, bool crouch,
                     u32 hair, u32 body, u32 accent, bool girl) {
	u32 playerSkin = girl ? skinTones[skinIndex] : SKIN;
	u32 playerSkinShadow = shadeColor(playerSkin, -28, -24, -20);
	int stride = walking ? ((frameCount / 7) & 1) : 0;
	int bob = walking ? stride : (crouch ? 1 : 0);
	int yOff = bob + (crouch ? 1 : 0);

	u32 hairHi  = shadeColor(hair, 36, 32, 36);
	u32 hairDk  = shadeColor(hair, -32, -28, -26);
	u32 bodyDk  = shadeColor(body, -34, -30, -28);
	u32 legCol  = girl ? playerSkinShadow : shadeColor(body, -48, -42, -32);
	u32 shoeCol = WOOD_DK;

	rect(cx - 7, fy - 2, 14, 4, SHADOW);

	int lLegLift = walking ? (stride ? 2 : 0) : 0;
	int rLegLift = walking ? (stride ? 0 : 2) : 0;
	rect(cx - 4, fy - 5 - lLegLift, 3, 5 - lLegLift + (lLegLift ? 1 : 0), girl ? RGB(248, 244, 232) : legCol);
	rect(cx + 1, fy - 5 - rLegLift, 3, 5 - rLegLift + (rLegLift ? 1 : 0), girl ? RGB(248, 244, 232) : legCol);
	rect(cx - 4, fy - 2 - lLegLift, 3, 2, girl ? RGB(48, 43, 52) : shoeCol);
	rect(cx + 1, fy - 2 - rLegLift, 3, 2, girl ? RGB(48, 43, 52) : shoeCol);

	int torsoY = fy - 13 - yOff;
	if (girl) {
		// Cream cardigan over a dark top, with a little blue plaid skirt.
		blob(cx - 5, torsoY, 10, 6, RGB(246, 232, 210), INK);
		rect(cx - 4, torsoY + 1, 2, 4, RGB(255, 248, 232));
		rect(cx + 3, torsoY + 1, 1, 4, RGB(218, 198, 174));
		rect(cx - 1, torsoY + 1, 3, 4, RGB(48, 48, 62));
		rect(cx, torsoY + 2, 1, 2, ROSE);
		// Skirt silhouette and tartan checks.
		blob(cx - 7, torsoY + 5, 14, 5, RGB(112, 148, 190), INK);
		rect(cx - 5, torsoY + 6, 1, 3, RGB(218, 232, 242));
		rect(cx - 1, torsoY + 6, 1, 3, RGB(218, 232, 242));
		rect(cx + 3, torsoY + 6, 1, 3, RGB(218, 232, 242));
		rect(cx - 6, torsoY + 8, 12, 1, RGB(75, 112, 158));
	} else {
		blob(cx - 6, torsoY, 12, 8, body, INK);
		rect(cx - 5, torsoY + 6, 10, 2, bodyDk);
		rect(cx - 3, torsoY, 6, 2, PAPER);
		rect(cx - 1, torsoY + 1, 2, 5, accent);
		rect(cx - 1, torsoY + 6, 2, 2, shadeColor(accent, -30, -30, -30));
	}

	int armSwing = walking ? (stride ? 1 : -1) : 0;
	rect(cx - 7, torsoY + 1 - armSwing, 2, 5, bodyDk);
	rect(cx + 5, torsoY + 1 + armSwing, 2, 5, bodyDk);
	rect(cx - 7, torsoY + 5 - armSwing, 2, 2, playerSkin);
	rect(cx + 5, torsoY + 5 + armSwing, 2, 2, playerSkin);

	int headY = torsoY - 12;
	blob(cx - 6, headY, 12, 12, playerSkin, INK);
	rect(cx - 5, headY + 9, 10, 2, playerSkinShadow);

	bool facingUp = fyv < -0.4f;
	if (facingUp) {
		blob(cx - 7, headY - 1, 14, 12, hair, INK);
		rect(cx - 5, headY, 10, 2, hairHi);
		rect(cx - 6, headY + 8, 12, 2, hairDk);
	} else {
		rect(cx - 6, headY - 1, 12, 3, hair);
		rect(cx - 4, headY, 8, 1, hairHi);
		rect(cx - 6, headY + 1, 2, 6, hairDk);
		rect(cx + 4, headY + 1, 2, 6, hairDk);
		rect(cx - 4, headY + 2, 3, 2, hair);
		rect(cx - 1, headY + 2, 2, 1, hair);
		rect(cx + 1, headY + 2, 3, 2, hair);

		int ex = (int)lroundf(fx * 1.5f);
		if (ex > 1) ex = 1;
		if (ex < -1) ex = -1;

		rect(cx - 3 + ex, headY + 5, 2, 3, INK);
		rect(cx + 1 + ex, headY + 5, 2, 3, INK);
		rect(cx - 3 + ex, headY + 5, 1, 1, PAPER);
		rect(cx + 1 + ex, headY + 5, 1, 1, PAPER);

		rect(cx - 5 + ex, headY + 8, 2, 1, RGBA(242, 118, 142, 210));
		rect(cx + 3 + ex, headY + 8, 2, 1, RGBA(242, 118, 142, 210));
		rect(cx - 1 + ex, headY + 9, 2, 1, INK_SOFT);
	}

	if (girl) {
		// Long chestnut side locks and unmistakable cream headphones.
		rect(cx - 8, headY + 2 - bob, 3, 12, hairDk);
		rect(cx + 5, headY + 2 - bob, 3, 12, hairDk);
		rect(cx - 7, headY + 3 - bob, 1, 6, hairHi);
		rect(cx + 6, headY + 3 - bob, 1, 6, hairHi);
		// Headband arcs over the hair; ear pads sit at both sides.
		rect(cx - 5, headY - 4, 10, 2, RGB(250, 244, 226));
		rect(cx - 6, headY - 3, 2, 4, RGB(250, 244, 226));
		rect(cx + 4, headY - 3, 2, 4, RGB(250, 244, 226));
		blob(cx - 8, headY + 1, 3, 5, RGB(248, 238, 218), INK);
		blob(cx + 5, headY + 1, 3, 5, RGB(248, 238, 218), INK);
		rect(cx - 7, headY + 2, 1, 3, SKY_UI);
		rect(cx + 6, headY + 2, 1, 3, SKY_UI);
		// Tiny rose hair clip.
		blob(cx + 2, headY - 1, 4, 3, ROSE, INK);
		rect(cx + 3, headY, 1, 1, PAPER);
	} else {
		rect(cx - 2, headY - 3, 4, 2, hairHi);
		rect(cx - 1, headY - 2, 2, 1, hair);
	}
}

static void drawGift(int gx, int gy, int i) {
	int bob = (int)(sinf((frameCount + i * 13) * 0.1f) * 2.0f);
	rect(gx - 5, gy + 4, 10, 3, SHADOW);
	gy += bob;
	blob(gx - 5, gy - 5, 10, 10, RGB(250, 148, 188), INK);
	rect(gx - 1, gy - 4, 2, 8, GOLD_UI);
	rect(gx - 4, gy - 1, 8, 2, GOLD_UI);
	rect(gx - 3, gy - 7, 3, 2, GOLD_UI);
	rect(gx, gy - 7, 3, 2, GOLD_UI);
}

static void drawCone(int cx, int cy, float fx, float fy, float range, float sus) {
	float a = atan2f(fy, fx), half = 0.68f;
	u32 c0 = sus > 50 ? RGBA(245, 110, 120, 95) : RGBA(250, 232, 130, 75);
	u32 ce = sus > 50 ? RGBA(245, 110, 120, 0)  : RGBA(250, 232, 130, 0);
	float ax = cx + cosf(a - half) * range, ay = cy + sinf(a - half) * range;
	float mx = cx + cosf(a) * range,        my = cy + sinf(a) * range;
	float bx = cx + cosf(a + half) * range, by = cy + sinf(a + half) * range;
	C2D_DrawTriangle(cx, cy, c0, ax, ay, ce, mx, my, ce, 0);
	C2D_DrawTriangle(cx, cy, c0, mx, my, ce, bx, by, ce, 0);
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

	// Gentle drifting pollen/fireflies add life without extra assets.
	for (int i = 0; i < 14; i++) {
		int wx = (int)((i * 97 + frameCount / 3) % (MAP_W * TS));
		int wy = (int)((i * 53 + frameCount / 5) % (MAP_H * TS));
		int ax = wx - camx, ay = wy - camy;
		if (ax >= 2 && ax < SCR_W - 2 && ay >= 2 && ay < SCR_H - 2 &&
		    map[wy / TS][wx / TS] != T_WATER) {
			u32 mote = (i & 1) ? RGBA(255, 246, 190, 170) : RGBA(255, 224, 222, 150);
			rect(ax, ay, 2, 2, mote);
		}
	}

	int sx = (int)sen.x - camx, sy = (int)sen.y - camy;
	bool senActive = (gameState == STATE_PLAYING || gameState == STATE_MENU) && given < NUM_GIFTS;

	if (senActive && sen.happy == 0 && gameState == STATE_PLAYING)
		drawCone(sx, sy, sen.fx, sen.fy, playerSneak ? 55.0f : 100.0f, sen.sus);

	for (int i = 0; i < NUM_GIFTS; i++)
		if (gifts[i].state == 0)
			drawGift((int)gifts[i].x - camx, (int)gifts[i].y - camy, i);

	int ppx = (int)px - camx, ppy = (int)py - camy;
	int pcxs = ppx + PLAYER_SIZE / 2, pfeet = ppy + PLAYER_SIZE;
	int sfeet = sy + 7;
	bool senWalk = senActive && sen.wait == 0;

	for (int pass = 0; pass < 2; pass++) {
		bool drawSenpai = (sfeet <= pfeet) ? (pass == 0) : (pass == 1);
		if (drawSenpai) {
			drawChar(sx, sfeet, sen.fx, sen.fy, senWalk, false,
			         RGB(62, 50, 92), SKY_UI, ROSE, false);
			if (sen.sus > 0) {
				card(sx - 13, sfeet - 36, 26, 6, PAPER, PAPER_DK, INK);
				rect(sx - 11, sfeet - 34, (int)(22 * sen.sus / 100.0f), 2, ROSE);
			}
			if (sen.happy > 0) {
				int fl = (sen.happy % 24) / 6;
				drawHeart(sx - 3, sfeet - 40 - fl, 1, ROSE);
				drawHeart(sx + 6, sfeet - 34 - ((fl + 2) & 3), 1, RGB(252, 152, 188));
			}
		} else {
            drawChar(pcxs, pfeet, pfx, pfy, playerMoving, playerSneak,
                     hairColors[hairIndex], outfitColors[outfitIndex], GOLD_UI, true);
			int c = 0;
			for (int i = 0; i < NUM_GIFTS; i++) if (gifts[i].state == 1) c++;
			for (int i = 0; i < c; i++)
				blob(pcxs + i * 6 - (c * 6) / 2, pfeet - 37, 6, 6, RGB(252, 148, 188), INK);
		}
	}

	if (gameState == STATE_MENU) {
		int bob = (int)(sinf(frameCount * 0.08f) * 2.0f);
		card(54, 24 + bob, 292, 62, PAPER, PAPER_DK, INK);
		drawText(74, 34 + bob, 0.85f, ROSE, "MOSSY HOLLOW");
		drawText(74, 62 + bob, 0.44f, INK_SOFT, "Collect gifts & sneak up behind Senpai!");
		drawHeart(324, 40 + bob, 2, ROSE);
		drawHeart(342, 52 + bob, 1, RGB(250, 164, 184));
	} else if (caughtTimer > 0) {
		card(56, 84, 288, 60, PAPER, PAPER_DK, INK);
		drawText(76, 92, 0.72f, ROSE, "Caught staring!");
		drawText(76, 118, 0.48f, INK_SOFT, "He gave you a funny look... try again.");
	}
}

// ---------------------------------------------------------------- bottom screen UI

static void drawMenuScreen(void) {
	card(8, 8, 304, 48, PAPER, PAPER_DK, INK);
	drawText(20, 16, 0.68f, ROSE, "MAIN MENU");
	drawText(20, 38, 0.42f, INK_SOFT, "Tap a button or press A to start");

    if (showCustomize) {
        card(10, 8, 300, 224, RGB(247, 232, 242), PAPER_DK, INK);
        drawText(24, 16, 0.62f, ROSE, "CHARACTER CUSTOMIZATION");
        drawText(24, 48, 0.48f, INK, "Choose a style");
        const char *cats[3] = { "SKIN", "HAIR", "OUTFIT" };
        for (int i = 0; i < 3; i++) {
            card(18 + i * 98, 70, 90, 30, customCategory == i ? GOLD_UI : PAPER, PAPER_DK, INK);
            drawText(35 + i * 98, 80, 0.46f, INK, cats[i]);
        }
        drawText(24, 116, 0.43f, INK_SOFT, "Tap a swatch or use LEFT / RIGHT");
        int swatches = customCategory == 0 ? 5 : 4;
        for (int i = 0; i < swatches; i++) {
            u32 c = customCategory == 0 ? skinTones[i] : (customCategory == 1 ? hairColors[i] : outfitColors[i]);
            int chosen = customCategory == 0 ? skinIndex : (customCategory == 1 ? hairIndex : outfitIndex);
            int sx = customCategory == 0 ? 10 + i * 61 : 22 + i * 70;
            card(sx, 138, customCategory == 0 ? 54 : 58, 38, chosen == i ? GOLD_UI : PAPER, PAPER_DK, INK);
            rect(sx + 16, 147, 22, 20, c);
        }
        drawText(24, 190, 0.42f, INK, "UP / DOWN: category   LEFT / RIGHT: change");
        card(104, 207, 112, 20, MINT_UI, PAPER_DK, INK);
        drawText(137, 211, 0.40f, INK, "DONE (A / B)");
        return;
    }
    if (showHowTo) {
        card(16, 64, 288, 148, PAPER, PAPER_DK, INK);
		drawText(28, 74, 0.56f, ROSE, "HOW TO PLAY");
		drawText(28, 98, 0.44f, INK, "1. Walk around to pick up gifts.");
		drawText(28, 116, 0.44f, INK, "2. Hold L to sneak & lower vision.");
		drawText(28, 134, 0.44f, INK, "3. Approach from behind & press A.");
		drawText(28, 152, 0.44f, INK, "4. Press SELECT anytime for menu.");
		card(96, 178, 128, 26, MINT_UI, PAPER_DK, INK);
		drawText(134, 185, 0.48f, INK, "BACK (A)");
		return;
	}

	for (int i = 0; i < BTN_COUNT; i++) {
		const MenuBtn *b = &MENU_BTNS[i];
		bool sel = (i == selectedBtn);
		u32 fill = sel ? GOLD_UI : PAPER;
		if (i == BTN_PLAY && hasStartedOnce && given < NUM_GIFTS) {
			card(b->x, b->y, b->w, b->h, fill, PAPER_DK, INK);
			drawText(b->x + 16, b->y + 13, 0.54f, INK, sel ? "> CONTINUE GAME" : "CONTINUE GAME");
		} else {
			card(b->x, b->y, b->w, b->h, fill, PAPER_DK, INK);
			char lbl[32];
			snprintf(lbl, sizeof(lbl), "%s%s", sel ? "> " : "", b->label);
			drawText(b->x + 16, b->y + 13, 0.54f, INK, lbl);
		}
	}

	drawText(16, 218, 0.40f, PAPER, "START : Quit game");
}

static void drawHudScreen(void) {
	int carried = 0;
	for (int i = 0; i < NUM_GIFTS; i++) if (gifts[i].state == 1) carried++;
	char buf[96];

	card(8, 8, 304, 30, PAPER, PAPER_DK, INK);
	drawText(18, 14, 0.64f, ROSE, "MOSSY HOLLOW");
	snprintf(buf, sizeof(buf), "Bag: %d", carried);
	drawText(246, 16, 0.50f, INK_SOFT, buf);

	card(8, 44, 152, 122, PAPER, PAPER_DK, INK);
	drawText(16, 50, 0.48f, INK, "GIFTS");
	for (int i = 0; i < 8; i++) {
		int hx = 18 + (i % 4) * 32;
		int hy = 68 + (i / 4) * 20;
		drawHeart(hx, hy, 2, i < given ? ROSE : PAPER_DK);
	}
	snprintf(buf, sizeof(buf), "Given %d/%d", given, NUM_GIFTS);
	drawText(16, 112, 0.46f, INK_SOFT, buf);

	drawText(16, 130, 0.48f, INK, "SUSPICION");
	card(16, 146, 136, 12, PAPER_DK, PAPER_DK, INK);
	rect(18, 148, (int)(132 * sen.sus / 100.0f), 8,
	     sen.sus > 60 ? ROSE : (playerSneak ? MINT_UI : GOLD_UI));

	card(166, 44, 146, 40, playerSneak ? MINT_UI : SKY_UI, PAPER_DK, INK);
	drawText(176, 56, 0.52f, INK, playerSneak ? "MODE: SNEAK" : "MODE: WALK");

	card(166, 90, 146, 76, PAPER, PAPER_DK, INK);
	drawText(174, 96, 0.44f, INK, "Pad : Move");
	drawText(174, 112, 0.44f, INK, "L   : Sneak");
	drawText(174, 128, 0.44f, INK, "A   : Give / Next");
	drawText(174, 140, 0.40f, INK, "B   : Talk");
	drawText(174, 154, 0.40f, ROSE, "SELECT : Menu");

	card(8, 172, 304, 60, PAPER, PAPER_DK, INK);
	drawText(18, 180, 0.46f, INK, "Approach Senpai from behind & press A.");
	drawText(18, 204, 0.42f, INK_SOFT, "START : Quit game");

	if (gameState == STATE_WIN) {
		card(26, 66, 268, 98, RGB(255, 228, 238), PAPER_DK, INK);
		drawText(44, 76, 0.68f, ROSE, "Senpai noticed you!");
		drawText(44, 106, 0.48f, INK, "All 8 gifts delivered. Great job!");
		drawText(44, 130, 0.42f, INK_SOFT, "Press A for a new run, or SELECT for menu.");
	}

	// Draw dialogue last so it overlays the normal HUD.
	if (dialogActive) {
		card(12, 150, 296, 78, RGB(255, 238, 224), PAPER_DK, INK);
		drawText(22, 158, 0.52f, ROSE, "SENPAI");
		drawText(22, 180, 0.43f, INK, getDialogue(dialogLine));
		drawText(22, 210, 0.38f, INK_SOFT, "A: Next       B: Close");
	}
}

static void drawBottom(void) {
	if (gameState == STATE_MENU) drawMenuScreen();
	else drawHudScreen();
}

int main(int argc, char **argv) {
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

	C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	C3D_RenderTarget *bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	textBuf = C2D_TextBufNew(4096);

	initGame();

	while (aptMainLoop()) {
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (kDown & KEY_START) break;

		updateGame(kDown, kHeld);

		C2D_TextBufClear(textBuf);
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

		C2D_TargetClear(top, GRASS_A);
		C2D_SceneBegin(top);
		drawWorld();

		C2D_TargetClear(bot, PAPER_DK);
		C2D_SceneBegin(bot);
		drawBottom();

		C3D_FrameEnd(0);
	}

	C2D_TextBufDelete(textBuf);
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	return 0;
}
