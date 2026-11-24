// Mossy Hollow - Cozy pixel-art stealth game for Nintendo 3DS (libctru + citro2d)
// Controls: Circle Pad / D-Pad = move | L = sneak | A = give gift / confirm | Touch = menu | START = quit

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

// ---- Cohesive cozy palette
#define INK       RGB(48, 34, 46)
#define INK_SOFT  RGB(96, 72, 68)
#define PAPER     RGB(252, 241, 218)
#define PAPER_DK  RGB(224, 198, 162)
#define WOOD_HI   RGB(226, 174, 114)
#define WOOD_MID  RGB(186, 128, 82)
#define WOOD_DK   RGB(126, 80, 54)
#define GRASS_A   RGB(142, 198, 114)
#define GRASS_B   RGB(132, 188, 106)
#define PATH_HI   RGB(242, 224, 184)
#define PATH_MID  RGB(222, 198, 154)
#define PATH_EDGE RGB(176, 148, 110)
#define SHADOW    RGBA(36, 44, 48, 68)
#define SKIN      RGB(255, 224, 196)
#define SKIN_SHD  RGB(226, 184, 156)
#define ROSE      RGB(238, 102, 134)
#define SKY_UI    RGB(118, 168, 214)
#define MINT_UI   RGB(122, 196, 148)
#define GOLD_UI   RGB(246, 196, 86)

enum {
	T_GRASS, T_PATH, T_TREE, T_APPLE, T_ROCK, T_ROOF,
	T_WALL, T_DOOR, T_FENCE, T_MUSH, T_FLOWER
};

enum {
	STATE_MENU,
	STATE_PLAYING,
	STATE_WIN
};

enum {
	BTN_PLAY,
	BTN_HOWTO,
	BTN_RESTART,
	BTN_COUNT
};

typedef struct {
	int x, y, w, h;
	const char *label;
} MenuBtn;

static const MenuBtn MENU_BTNS[BTN_COUNT] = {
	{ 36, 66,  248, 42, "PLAY GAME" },
	{ 36, 116, 248, 42, "HOW TO PLAY" },
	{ 36, 166, 248, 42, "NEW RUN" }
};

static u8 map[MAP_H][MAP_W];
static u32 flowerCols[5];
static int frameCount = 0;
static int gameState = STATE_MENU;
static bool hasStartedOnce = false;
static bool showHowTo = false;
static int selectedBtn = BTN_PLAY;

static bool solid(u8 t) {
	return t == T_TREE || t == T_APPLE || t == T_ROCK || t == T_ROOF ||
	       t == T_WALL || t == T_DOOR || t == T_FENCE;
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
	return x >= 22 && x <= 37 && y >= 10 && y <= 24;
}

static void genMap(void) {
	srand(1234);
	memset(map, T_GRASS, sizeof(map));

	for (int x = 0; x < MAP_W; x++)
		for (int y = 38; y <= 39; y++) map[y][x] = T_PATH;

	const int hx = 28, hy = 14;
	for (int x = hx; x < hx + 6; x++) {
		map[hy][x] = T_ROOF;
		map[hy + 1][x] = T_ROOF;
		map[hy + 2][x] = T_WALL;
		map[hy + 3][x] = T_WALL;
	}
	map[hy + 3][hx + 3] = T_DOOR;

	for (int y = hy + 4; y < 38; y++) {
		map[y][hx + 3] = T_PATH;
		map[y][hx + 4] = T_PATH;
	}

	for (int x = 24; x <= 35; x++) {
		map[12][x] = T_FENCE;
		if (x != hx + 3 && x != hx + 4) map[22][x] = T_FENCE;
	}
	for (int y = 12; y <= 22; y++) {
		map[y][24] = T_FENCE;
		map[y][35] = T_FENCE;
	}

	for (int x = 0; x < MAP_W; x++) { map[0][x] = T_TREE; map[MAP_H - 1][x] = T_TREE; }
	for (int y = 0; y < MAP_H; y++) { map[y][0] = T_TREE; map[y][MAP_W - 1] = T_TREE; }

	for (int i = 0; i < 900; i++) {
		int x = 1 + rand() % (MAP_W - 2);
		int y = 1 + rand() % (MAP_H - 2);
		if (map[y][x] != T_GRASS) continue;
		if (inYard(x, y)) continue;
		if (x >= 27 && x <= 36 && y >= 33 && y <= 37) continue;
		int r = rand() % 100;
		if (r < 40) map[y][x] = T_TREE;
		else if (r < 50) map[y][x] = T_APPLE;
		else if (r < 60) map[y][x] = T_ROCK;
		else if (r < 80) map[y][x] = T_MUSH;
		else map[y][x] = T_FLOWER;
	}

	for (int i = 0; i < 14; i++) {
		int x = 25 + rand() % 10;
		int y = 18 + rand() % 4;
		if (map[y][x] == T_GRASS) map[y][x] = T_FLOWER;
	}
}

// ---------------------------------------------------------------- drawing helpers

static void rect(int x, int y, int w, int h, u32 c) {
	if (w <= 0 || h <= 0) return;
	C2D_DrawRectSolid(x, y, 0, w, h, c);
}

static void card(int x, int y, int w, int h, u32 fill, u32 bevel, u32 border) {
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

static void drawHeart(int x, int y, int s, u32 c) {
	rect(x, y, 2 * s, 2 * s, c);
	rect(x + 4 * s, y, 2 * s, 2 * s, c);
	rect(x - s, y + s, 8 * s, 2 * s, c);
	rect(x, y + 3 * s, 6 * s, s, c);
	rect(x + s, y + 4 * s, 4 * s, s, c);
	rect(x + 2 * s, y + 5 * s, 2 * s, s, c);
}

static bool isPathTile(int tx, int ty) {
	if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) return false;
	return map[ty][tx] == T_PATH;
}

static void drawTile(u8 t, int sx, int sy, int tx, int ty) {
	u32 h = hash2(tx, ty);
	int swi = ((frameCount / 18 + tx + ty) % 3) - 1;

	if (t != T_PATH && t != T_ROOF && t != T_WALL && t != T_DOOR) {
		rect(sx, sy, TS, TS, ((tx + ty) & 1) ? GRASS_A : GRASS_B);
		if ((h & 3) == 0) {
			int ox = ((h >> 4) % 9) + 3;
			int oy = ((h >> 8) % 9) + 3;
			rect(sx + ox, sy + oy + 1, 3, 1, RGB(104, 162, 82));
			rect(sx + ox + 1, sy + oy, 1, 2, RGB(104, 162, 82));
		}
	}

	switch (t) {
	case T_PATH: {
		rect(sx, sy, TS, TS, PATH_MID);
		rect(sx + 2 + (h & 3), sy + 3 + ((h >> 2) & 3), 4, 3, PATH_HI);
		rect(sx + 9 - ((h >> 4) & 2), sy + 9 - ((h >> 5) & 2), 4, 3, PATH_HI);
		if (!isPathTile(tx, ty - 1)) rect(sx, sy, TS, 2, PATH_EDGE);
		if (!isPathTile(tx, ty + 1)) rect(sx, sy + TS - 2, TS, 2, PATH_EDGE);
		if (!isPathTile(tx - 1, ty)) rect(sx, sy, 2, TS, PATH_EDGE);
		if (!isPathTile(tx + 1, ty)) rect(sx + TS - 2, sy, 2, TS, PATH_EDGE);
		break;
	}
	case T_TREE:
	case T_APPLE:
		rect(sx + 2, sy + 12, 12, 3, SHADOW);
		blob(sx + 6, sy + 8, 4, 7, WOOD_MID, INK);
		blob(sx, sy - 4, 16, 13, RGB(82, 164, 98), INK);
		rect(sx + 2, sy + 5, 12, 3, RGB(60, 136, 82));
		rect(sx + 3, sy - 2, 7, 4, RGB(126, 204, 118));
		if (t == T_APPLE) {
			rect(sx + 3, sy + 2, 3, 3, ROSE);
			rect(sx + 10, sy + 1, 3, 3, ROSE);
			rect(sx + 7, sy + 5, 3, 3, ROSE);
		}
		break;
	case T_ROCK:
		rect(sx + 2, sy + 12, 12, 3, SHADOW);
		blob(sx + 2, sy + 5, 12, 9, RGB(164, 168, 184), INK);
		rect(sx + 4, sy + 4, 6, 3, RGB(204, 208, 224));
		rect(sx + 4, sy + 11, 8, 2, RGB(132, 136, 154));
		break;
	case T_ROOF: {
		int row = ty - 14;
		u32 rFill = row == 0 ? RGB(218, 114, 92) : RGB(198, 96, 78);
		rect(sx, sy, TS, TS, rFill);
		rect(sx, sy + 1, TS, 2, RGB(238, 146, 116));
		rect(sx, sy + 14, TS, 2, RGB(144, 62, 54));
		rect(sx + 7, sy, 1, TS, RGB(154, 68, 58));
		if (tx == 32 && row == 0) {
			blob(sx + 3, sy - 6, 10, 10, WOOD_MID, INK);
			rect(sx + 4, sy - 5, 8, 2, WOOD_HI);
		}
		break;
	}
	case T_WALL:
		rect(sx, sy, TS, TS, PAPER);
		if (tx == 28 || tx == 33) rect(sx + (tx == 28 ? 0 : 14), sy, 2, TS, WOOD_MID);
		if (ty == 16) rect(sx, sy, TS, 3, SHADOW);
		if (ty == 16 && (tx == 29 || tx == 31 || tx == 33)) {
			blob(sx + 2, sy + 4, 12, 9, WOOD_MID, INK);
			rect(sx + 4, sy + 6, 8, 5, RGB(176, 224, 246));
			rect(sx + 4, sy + 6, 3, 2, RGB(236, 250, 255));
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
			rect(fx, fy + 2, 1, 5, RGB(74, 148, 74));
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

static C2D_TextBuf textBuf;

static void drawText(float x, float y, float scale, u32 clr, const char *s) {
	C2D_Text t;
	C2D_TextParse(&t, textBuf, s);
	C2D_TextOptimize(&t);
	C2D_DrawText(&t, C2D_WithColor, x, y, 0.5f, scale, scale, clr);
}

static void resetPlayer(void) {
	px = 31 * TS + 3;
	py = 36 * TS + 3;
}

static void initGame(void) {
	flowerCols[0] = RGB(252, 138, 176);
	flowerCols[1] = RGB(252, 216, 98);
	flowerCols[2] = RGB(255, 250, 240);
	flowerCols[3] = RGB(184, 154, 248);
	flowerCols[4] = RGB(252, 166, 98);

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

	int placed = 0, tries = 0;
	while (placed < NUM_GIFTS && tries < 5000) {
		tries++;
		int x = 2 + rand() % (MAP_W - 4);
		int y = 2 + rand() % (MAP_H - 4);
		if (solid(map[y][x]) || inYard(x, y) || map[y][x] == T_PATH) continue;
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

	if (kDown & KEY_SELECT) {
		showHowTo = false;
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
