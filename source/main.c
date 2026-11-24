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

// A tile whose view is covered by a nearby tree canopy (used so gifts are never hidden).
static bool underCanopy(int x, int y) {
	for (int dy = 0; dy <= 2; dy++)
		for (int dx = -1; dx <= 1; dx++)
			if (map[y + dy][x + dx] == T_TREE) return true;
	return false;
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

	// Paved sidewalk strip along the shop front (Senpai patrols along it)
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
	if (t != T_PATH && t != T_ROOF && t != T_WALL && t != T_DOOR && t != T_WATER && t != T_BLANKET) {
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
	case T_ROOF: { // dark kawara roof tiles
		int row = ty - 13;
		rect(sx, sy, TS, TS, ROOF_MID);
		for (int k = 0; k < 4; k++) {
			int o = (k & 1) ? 4 : 0;
			rect(sx, sy + k * 4, TS, 1, ROOF_HI);
			rect(sx, sy + k * 4 + 3, TS, 1, ROOF_DK);
			rect(sx + o, sy + k * 4 + 1, 1, 2, ROOF_DK);
			rect(sx + o + 8, sy + k * 4 + 1, 1, 2, ROOF_DK);
		}
		if (row == 0) rect(sx, sy, TS, 2, ROOF_DK);
		if (row == 1) rect(sx, sy + TS - 3, TS, 3, SHOP_WOOD_DK);
		break;
	}
	case T_WALL:
		if (ty == 15) { // dark sign band (the TEA SHOP lettering is drawn in drawWorld)
			rect(sx, sy, TS, TS, SHOP_DARK);
			rect(sx, sy, TS, 1, SHOP_WOOD_DK);
			rect(sx, sy + TS - 1, TS, 1, SHOP_WOOD);
			if (tx == 28) rect(sx, sy, 2, TS, SHOP_WOOD_DK);
			if (tx == 33) rect(sx + TS - 2, sy, 2, TS, SHOP_WOOD_DK);
		} else if (tx == 28 || tx == 33) { // wooden posts
			rect(sx, sy, TS, TS, SHOP_WOOD);
			rect(sx + 4, sy, 2, TS, SHOP_WOOD_HI);
			rect(sx + 11, sy, 2, TS, SHOP_WOOD_DK);
		} else { // warm lattice window
			rect(sx, sy, TS, TS, SHOP_WOOD_DK);
			rect(sx + 1, sy + 1, 14, 12, SHOP_GLOW);
			rect(sx + 1, sy + 1, 14, 2, RGB(226, 186, 124));
			rect(sx + 5, sy + 1, 1, 12, SHOP_WOOD);
			rect(sx + 10, sy + 1, 1, 12, SHOP_WOOD);
			rect(sx + 1, sy + 5, 14, 1, SHOP_WOOD);
			rect(sx + 1, sy + 9, 14, 1, SHOP_WOOD);
			rect(sx, sy + 13, TS, 3, SHOP_WOOD);
		}
		break;
	case T_DOOR: // lattice door under a pink noren curtain
		rect(sx, sy, TS, TS, SHOP_WOOD_DK);
		rect(sx + 1, sy + 1, 14, 15, SHOP_DARK);
		rect(sx + 2, sy + 8, 12, 8, SHOP_GLOW);
		rect(sx + 7, sy + 8, 2, 8, SHOP_WOOD);
		rect(sx + 2, sy + 11, 12, 1, SHOP_WOOD);
		rect(sx + 2, sy + 14, 12, 1, SHOP_WOOD);
		rect(sx + 1, sy, 14, 8, NOREN);
		rect(sx + 1, sy + 7, 14, 1, NOREN_DK);
		rect(sx + 5, sy + 3, 1, 5, NOREN_DK);
		rect(sx + 10, sy + 3, 1, 5, NOREN_DK);
		rect(sx + 6, sy + 2, 4, 2, PAPER);
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
	sen.happy = 0;
	given = 0;
	caughtTimer = 0;
	dialogActive = false;
	dialogLine = 0;

	int placed = 0, tries = 0;
	while (placed < NUM_GIFTS && tries < 20000) {
		tries++;
		int x = 2 + rand() % (MAP_W - 4);
		int y = 2 + rand() % (MAP_H - 4);
		if (!reach[y][x] || solid(map[y][x]) || inYard(x, y)) continue;
		if (map[y][x] == T_PATH || map[y][x] == T_BRIDGE) continue;
		if (underCanopy(x, y)) continue;
		// keep gifts spread out: at least ~6 tiles from every gift already placed
		bool tooClose = false;
		for (int k = 0; k < placed; k++) {
			float gdx = gifts[k].x - (x * TS + TS / 2), gdy = gifts[k].y - (y * TS + TS / 2);
			if (gdx * gdx + gdy * gdy < 96.0f * 96.0f) { tooClose = true; break; }
		}
		if (tooClose) continue;
		gifts[placed].x = x * TS + TS / 2;
		gifts[placed].y = y * TS + TS / 2;
		gifts[placed].state = 0;
		placed++;
	}
	while (placed < NUM_GIFTS) { // fallback (should never happen)
		gifts[placed].x = (31 + placed) * TS + TS / 2;
		gifts[placed].y = 34 * TS + TS / 2;
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

static const char *getDialogue(int line) {
	if (line < 0 || line >= 4) return "...";
	if (given >= 6) return dialogLate[line];
	if (given >= 3) return dialogMid[line];
	return dialogEarly[line];
}

static void updateGame(u32 kDown, u32 kHeld) {
	frameCount++;

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
		gameState = STATE_MENU;
		return;
	}

	if (gameState == STATE_WIN) {
		if (kDown & KEY_A) {
			initGame();
			gameState = STATE_PLAYING;
		}
		return;
	}

	playerMoving = false;

	// Player movement and Senpai's patrol pause during dialogue.
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

	if (caughtTimer > 0) { caughtTimer--; return; }

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

	bool sneaking = (kHeld & KEY_L) != 0;
	playerSneak = sneaking;
	float speed = sneaking ? 0.8f : 1.7f;

	float nx = px + mx * speed;
	if (!blockedAt(nx, py)) px = nx;
	float ny = py + my * speed;
	if (!blockedAt(px, ny)) py = ny;

	float pcx = px + PLAYER_SIZE / 2.0f;
	float pcy = py + PLAYER_SIZE / 2.0f;

	// ---- pick up gifts
	for (int i = 0; i < NUM_GIFTS; i++) {
		if (gifts[i].state != 0) continue;
		float dx = gifts[i].x - pcx, dy = gifts[i].y - pcy;
		if (dx * dx + dy * dy < 100) gifts[i].state = 1;
	}

	float sdx = sen.x - pcx, sdy = sen.y - pcy;
	float sdist = sqrtf(sdx * sdx + sdy * sdy);

	// ---- give gifts (A) / talk (B)
	if ((kDown & KEY_A) && sdist < 24) {
		int n = 0;
		for (int i = 0; i < NUM_GIFTS; i++)
			if (gifts[i].state == 1) { gifts[i].state = 2; n++; }
		if (n > 0) {
			given += n;
			sen.happy = 120;
			sen.wait = 90;
			sen.sus = 0;
			if (given >= NUM_GIFTS) {
				gameState = STATE_WIN;
				return;
			}
		}
	}
	if ((kDown & KEY_B) && sdist < 30) {
		dialogActive = true;
		dialogLine = 0;
		sen.sus = 0;
		sen.wait = 30;
		return;
	}

	updateSenpai();

	// ---- vision cone
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

// ---------------------------------------------------------------- senpai sprite
// Same 24x40 size, chibi proportions and flat pixel-art style as the player.
// Hair a b c = dark / mid / light     s S p = skin / shade / blush     q e y = eye
// j J = blazer / shade   g G = tee / print   u = white cuff
// n N = trousers / shade   o O = white sneakers / sole
static const char SPR_SEN[SPR_H][SPR_W + 1] = {
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

static u32 senpaiColor(char ch) {
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
	case 'k': return RGB(112, 80, 64);
	case 'm': return RGB(170, 130, 108);
	case 'l': return RGB(204, 164, 142);
	case 'x': return RGB(250, 250, 250);
	case 'o': return RGB(40, 30, 30);
	case 'q': return RGB(10, 6, 6);
	case 'e': return RGB(150, 122, 100);
	case 'y': return RGB(246, 238, 230);
	default:  return RGB(255, 0, 255);
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
		// back view: no shirt front / bare neck (only senpai's sprite has g, G)
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
static void drawSpriteGen(int cx, int fy, int dir, bool walking, const char (*src)[SPR_W + 1], bool senpai,
                          u32 skin, u32 hair, u32 sweater) {
	char buf[SPR_H][SPR_W];
	buildSprite(buf, src, dir);
	int step = walking ? ((frameCount / 8) & 1) : 0;
	int x0 = cx - SPR_W / 2, y0 = fy - SPR_H;
	int mid = SPR_W / 2;

	rect(cx - 8, fy - 2, 16, 4, SHADOW);
	for (int r = 0; r < SPR_H; r++) {
		int c = 0;
		while (c < SPR_W) {
			char ch = buf[r][c];
			if (ch == '.') { c++; continue; }
			int e = c + 1;
			while (e < SPR_W && buf[r][e] == ch && !(r >= SPR_LEG_ROW && e == mid)) e++;
			int dy = 0;
			if (r >= SPR_LEG_ROW) dy = (c < mid) ? (step ? -1 : 0) : (step ? 0 : -1);
			rect(x0 + c, y0 + r + dy, e - c, 1, senpai ? senpaiColor(ch) : spriteColor(ch, skin, hair, sweater));
			c = e;
		}
	}
}

static void drawPlayerSprite(int cx, int fy, int dir, bool walking, u32 skin, u32 hair, u32 sweater) {
	drawSpriteGen(cx, fy, dir, walking, SPR, false, skin, hair, sweater);
}

static void drawSenpaiSprite(int cx, int fy, int dir, bool walking) {
	drawSpriteGen(cx, fy, dir, walking, SPR_SEN, true, 0, 0, 0);
}

static int dirFromFacing(float fx, float fy) {
	if (fabsf(fy) >= fabsf(fx)) return fy < 0 ? 1 : 0;
	return fx < 0 ? 2 : 3;
}

static void drawGift(int gx, int gy, int i) {
	int bob = (int)(sinf((frameCount + i * 13) * 0.1f) * 2.0f);
	rect(gx - 5, gy + 4, 10, 3, SHADOW);
	gy += bob;
	blob(gx - 5, gy - 5, 10, 10, RGB(255, 150, 190), INK);
	rect(gx - 1, gy - 4, 2, 8, GOLD_UI);
	rect(gx - 4, gy - 1, 8, 2, GOLD_UI);
	rect(gx - 3, gy - 7, 3, 2, GOLD_UI);
	rect(gx, gy - 7, 3, 2, GOLD_UI);
	if (((frameCount + i * 20) % 70) < 20) {
		u32 w = RGB(255, 255, 255);
		rect(gx + 6, gy - 9, 1, 5, w);
		rect(gx + 4, gy - 7, 5, 1, w);
	}
}

static void drawCone(int cx, int cy, float fx, float fy, float range, float sus) {
	float a = atan2f(fy, fx), half = 0.68f;
	u32 c0 = sus > 50 ? RGBA(255, 120, 120, 90) : RGBA(255, 244, 150, 70);
	u32 ce = sus > 50 ? RGBA(255, 120, 120, 0) : RGBA(255, 244, 150, 0);
	float ax = cx + cosf(a - half) * range, ay = cy + sinf(a - half) * range;
	float mx = cx + cosf(a) * range,        my = cy + sinf(a) * range;
	float bx = cx + cosf(a + half) * range, by = cy + sinf(a + half) * range;
	C2D_DrawTriangle(cx, cy, c0, ax, ay, ce, mx, my, ce, 0);
	C2D_DrawTriangle(cx, cy, c0, mx, my, ce, bx, by, ce, 0);
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

	// HUD hearts + bars (gifts given / gifts carried)
	int carried = 0;
	for (int i = 0; i < NUM_GIFTS; i++) if (gifts[i].state == 1) carried++;
	for (int k = 0; k < 2; k++) {
		int y = 8 + k * 20;
		drawHeartO(10, y, 2, k == 0 ? PINK_MID : PINK_LT, MAUVE);
		rect(34, y + 1, 104, 8, MAUVE);
		rect(35, y + 2, 102, 6, RGB(222, 214, 226));
		int val = k == 0 ? given : carried;
		rect(35, y + 2, 102 * val / NUM_GIFTS, 6, k == 0 ? PINK_MID : PINK_LT);
	}

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

static void drawSenpaiEntity(int sx, int sfeet, bool walking) {
	drawSenpaiSprite(sx, sfeet, dirFromFacing(sen.fx, sen.fy), walking);
	if (sen.sus > 0) {
		blob(sx - 13, sfeet - SPR_H - 8, 26, 6, RGB(240, 230, 230), INK);
		rect(sx - 11, sfeet - SPR_H - 6, (int)(22 * sen.sus / 100.0f), 2,
		     RGB(240, (int)(190 - sen.sus * 1.2f), 90));
	}
	if (sen.happy > 0) {
		int fl = (sen.happy % 24) / 6;
		drawHeart(sx - 3, sfeet - SPR_H - 12 - fl, 1, RGB(255, 90, 140));
		drawHeart(sx + 6, sfeet - SPR_H - 6 - ((fl + 2) & 3), 1, RGB(255, 150, 190));
	}
}

static void drawPlayerEntity(int pcxs, int pfeet) {
	drawPlayerSprite(pcxs, pfeet, dirFromFacing(pfx, pfy), playerMoving,
	                 skinTones[skinIndex], hairColors[hairIndex], outfitColors[outfitIndex]);
	int c = 0;
	for (int i = 0; i < NUM_GIFTS; i++) if (gifts[i].state == 1) c++;
	for (int i = 0; i < c; i++)
		blob(pcxs + i * 6 - (c * 6) / 2, pfeet - SPR_H - 8, 6, 6, RGB(255, 150, 190), INK);
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

	// "TEA SHOP" lettering on the sign band (tile row 15 of the cottage)
	{
		int bx = 28 * TS - camx, by = 15 * TS - camy;
		if (bx > -96 && bx < SCR_W && by > -16 && by < SCR_H)
			pixText(bx + 1, by + 1, 2, "TEA SHOP", PAPER, 0, 0);
	}

	int sx = (int)sen.x - camx, sy = (int)sen.y - camy;
	bool senActive = given < NUM_GIFTS;

	if (senActive && sen.happy == 0 && !dialogActive)
		drawCone(sx, sy, sen.fx, sen.fy, playerSneak ? 55.0f : 100.0f, sen.sus);

	for (int i = 0; i < NUM_GIFTS; i++)
		if (gifts[i].state == 0)
			drawGift((int)gifts[i].x - camx, (int)gifts[i].y - camy, i);

	int ppx = (int)px - camx, ppy = (int)py - camy;
	int pcxs = ppx + PLAYER_SIZE / 2, pfeet = ppy + PLAYER_SIZE;
	int sfeet = sy + PLAYER_SIZE / 2;
	bool senWalk = senActive && sen.wait == 0;

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
				if (kind[ei] == 0) drawSenpaiEntity(sx, sfeet, senWalk);
				else drawPlayerEntity(pcxs, pfeet);
				ei++;
			}
			drawTree(tsx, tsy, tx, ty);
		}
	}
	while (ei < 2) {
		if (kind[ei] == 0) drawSenpaiEntity(sx, sfeet, senWalk);
		else drawPlayerEntity(pcxs, pfeet);
		ei++;
	}

	drawVignette();

	if (caughtTimer > 0) {
		blob(60, 86, 280, 56, PAPER, INK);
		drawText(88, 94, 0.7f, RGB(214, 80, 110), "Caught staring!");
		drawText(80, 118, 0.5f, INK_SOFT, "He gave you a funny look... try again.");
	}

	if (dialogActive) {
		blob(16, 150, 368, 78, PAPER, INK);
		rect(18, 152, 364, 3, PINK_PALE);
		pill(28, 140, 84, 20, PINK_LT, PINK_DK);
		pixTextC(70, 144, 1, "SENPAI", WHITE, 0, PLUM);
		drawText(30, 168, 0.6f, INK, getDialogue(dialogLine));
		drawText(236, 206, 0.42f, INK_SOFT, "A: next    B: close");
		drawHeart(358, 208, 1, ROSE);
	}

	if (gameState == STATE_WIN) {
		blob(50, 60, 300, 110, PAPER, INK);
		rect(52, 62, 296, 4, PINK_PALE);
		pixTextC(200, 76, 4, "MAHAL KITA", WHITE, PINK_MID, MAUVE);
		drawText(98, 118, 0.6f, INK, "Senpai: \"Mahal din kita!\"");
		drawText(112, 142, 0.45f, INK_SOFT, "You gave him all the gifts.   A: play again");
	}
}

static void drawCharPreview(void) {
	// big preview of the player on the top screen while customizing
	pill(130, 22, 140, 208, PAPER, PINK_DK);
	pixTextC(200, 32, 2, "YOUR LOOK", PINK_DK, 0, PINK_PALE);
	gsc = 4; gox = 200; goy = 52;
	drawPlayerSprite(0, SPR_H, (frameCount / 90) % 4 == 1 ? 1 : 0, (frameCount / 24) & 1,
	                 skinTones[skinIndex], hairColors[hairIndex], outfitColors[outfitIndex]);
	gsc = 1; gox = 0; goy = 0;
}

static void drawTopScreen(void) {
	if (gameState == STATE_MENU) {
		drawTitleScene(!showCustomize);
		if (showCustomize) drawCharPreview();
	} else {
		drawWorld();
	}
}

static void drawCustomizeBottom(void) {
	pixTextC(160, 14, 3, "CUSTOMIZE", WHITE, PINK_MID, MAUVE);
	drawText(70, 52, 0.45f, MAUVE, "Tap a swatch, or use the D-Pad");

	const char *tabs[3] = { "SKIN", "HAIR", "OUTFIT" };
	for (int i = 0; i < 3; i++) {
		int x = 6 + i * 104;
		bool sel = (i == customCategory);
		pill(x, 78, 100, 40, sel ? PINK_LT : RGB(240, 228, 238), sel ? PINK_DK : RGB(190, 164, 184));
		pixTextC(x + 50, 91, 2, tabs[i], sel ? WHITE : RGB(156, 128, 150), 0, sel ? PLUM : 0);
	}

	int count = customCategory == 0 ? 5 : 4;
	const u32 *pal = customCategory == 0 ? skinTones : (customCategory == 1 ? hairColors : outfitColors);
	int cur = customCategory == 0 ? skinIndex : (customCategory == 1 ? hairIndex : outfitIndex);
	for (int i = 0; i < count; i++) {
		int x = customCategory == 0 ? 10 + i * 61 : 22 + i * 70;
		int w = customCategory == 0 ? 52 : 60;
		blob(x - 2, 136, w + 4, 44, i == cur ? PINK_DK : MAUVE, i == cur ? PINK_DK : MAUVE);
		blob(x, 138, w, 40, pal[i], INK);
		if (i == cur) drawHeartO(x + w / 2 - 3, 150, 1, WHITE, PLUM);
	}

	pill(100, 188, 120, 22, PINK_LT, PINK_DK);
	pixTextC(160, 192, 1, "DONE", WHITE, 0, PLUM);
}

static void drawHowToBottom(void) {
	pixTextC(160, 12, 3, "HOW TO PLAY", WHITE, PINK_MID, MAUVE);
	pill(14, 48, 292, 148, PAPER, PINK_DK);
	u32 ink = INK;
	drawText(28, 58, 0.5f, ink, "Collect the 8 gifts hidden around the garden.");
	drawText(28, 76, 0.5f, ink, "Give them to Senpai with A when you are close.");
	drawText(28, 94, 0.5f, ink, "Do not get caught staring at him!");
	drawText(28, 118, 0.45f, INK_SOFT, "Move: Circle Pad / D-Pad      Sneak: hold L");
	drawText(28, 134, 0.45f, INK_SOFT, "Talk to Senpai: B      Pause: SELECT or MENU");
	drawText(28, 150, 0.45f, INK_SOFT, "Sneak or come from behind to stay unseen.");
	drawText(28, 170, 0.45f, PINK_DK, "Mahal kita means \"I love you\" in Filipino.");
	drawText(100, 204, 0.45f, MAUVE, "Tap or press A to go back");
}

static void drawMenuBottom(void) {
	drawPinkBottomBg();
	if (showCustomize) {
		drawCustomizeBottom();
	} else if (showHowTo) {
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

static void drawPlayBottom(void) {
	int carried = 0;
	for (int i = 0; i < NUM_GIFTS; i++) if (gifts[i].state == 1) carried++;
	char buf[96];

	drawPinkBottomBg();
	blob(8, 8, 304, 184, PAPER, INK);
	rect(10, 10, 300, 3, PINK_PALE);
	pixTextC(160, 18, 3, "MAHAL KITA", PINK_DK, 0, PINK_PALE);

	for (int i = 0; i < 8; i++)
		drawHeartO(24 + i * 35, 56, 3, i < given ? RGB(240, 80, 130) : RGB(236, 220, 214), i < given ? PLUM : RGB(200, 180, 176));
	snprintf(buf, sizeof(buf), "Gifts given: %d / %d    Carrying: %d", given, NUM_GIFTS, carried);
	drawText(22, 82, 0.5f, INK, buf);

	drawText(22, 104, 0.5f, INK, "Suspicion");
	blob(22, 122, 276, 14, RGB(232, 216, 208), INK);
	rect(24, 124, (int)(272 * sen.sus / 100.0f), 10, RGB(240, (int)(190 - sen.sus * 1.2f), 90));

	drawText(22, 146, 0.42f, INK_SOFT, "Move: Circle Pad / D-Pad    Sneak: hold L");
	drawText(22, 160, 0.42f, INK_SOFT, "A: give gifts    B: talk to Senpai");
	drawText(22, 174, 0.42f, PINK_DK, "Tip: sneak up from behind!");

	pill(PLAY_MENU_X, PLAY_MENU_Y, PLAY_MENU_W, PLAY_MENU_H, PINK_LT, PINK_DK);
	pixTextC(PLAY_MENU_X + PLAY_MENU_W / 2, PLAY_MENU_Y + 8, 2, "MENU", WHITE, 0, PLUM);
	drawText(176, 210, 0.4f, MAUVE, "START: quit game");
}

int main(int argc, char **argv) {
	(void)argc; (void)argv;
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(8192);
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

		C2D_TargetClear(top, RGB(208, 200, 228));
		C2D_SceneBegin(top);
		drawTopScreen();

		C2D_TargetClear(bot, RGB(248, 214, 232));
		C2D_SceneBegin(bot);
		if (gameState == STATE_MENU) drawMenuBottom();
		else drawPlayBottom();

		C3D_FrameEnd(0);
	}

	C2D_TextBufDelete(textBuf);
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	return 0;
}
