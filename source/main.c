// Mossy Hollow - a cute-but-obsessive top-down stealth game for the Nintendo 3DS
// Goal: collect little gifts and give them to Senpai without being caught staring.
// Sneak (hold L) or approach from behind to avoid raising his suspicion.
// Built with libctru + citro2d. Art style: cozy storybook pastels, all drawn
// with rectangles (no image assets needed).
//
// Controls:  Circle Pad / D-Pad = move   L = sneak   A = give gift   START = quit

#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SCR_W 400
#define SCR_H 240
#define TS 16
#define MAP_W 64
#define MAP_H 48
#define RGB(r, g, b) C2D_Color32((r), (g), (b), 255)
#define RGBA(r, g, b, a) C2D_Color32((r), (g), (b), (a))

#define NUM_GIFTS 8
#define PLAYER_SIZE 10

// ---- palette
#define OUT    RGB(62, 44, 66)      // soft plum outline
#define SHAD   RGBA(30, 60, 40, 70) // ground shadow
#define SKIN   RGB(255, 224, 194)
#define CREAM  RGB(255, 243, 226)
#define GOLD   RGB(255, 208, 90)

enum {
	T_GRASS, T_PATH, T_TREE, T_APPLE, T_ROCK, T_ROOF,
	T_WALL, T_DOOR, T_FENCE, T_MUSH, T_FLOWER
};

static u8 map[MAP_H][MAP_W];
static int frameCount = 0;

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
		if (x >= 27 && x <= 36 && y >= 33 && y <= 37) continue; // keep spawn clear
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

// ---------------------------------------------------------------- art helpers

static void rect(int x, int y, int w, int h, u32 c) {
	C2D_DrawRectSolid(x, y, 0, w, h, c);
}

// rounded, outlined blob
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

static const u32 flowerCols[5] = {
	RGB(255, 140, 180), RGB(255, 226, 110), RGB(255, 250, 245),
	RGB(190, 160, 255), RGB(255, 170, 100)
};

static void drawTile(u8 t, int sx, int sy, int tx, int ty) {
	u32 h = hash2(tx, ty);
	float sw = sinf(frameCount * 0.05f + tx * 0.7f + ty * 0.4f);
	int swi = sw > 0.35f ? 1 : (sw < -0.35f ? -1 : 0);

	// soft two-tone grass under everything except path and buildings
	if (t != T_PATH && t != T_ROOF && t != T_WALL && t != T_DOOR) {
		rect(sx, sy, TS, TS, ((tx + ty) & 1) ? RGB(131, 199, 100) : RGB(122, 190, 93));
		if ((h & 3) < 2) {
			int ox = ((h >> 4) % 10) + 3, oy = ((h >> 8) % 9) + 4;
			u32 d = RGB(92, 162, 74);
			rect(sx + ox, sy + oy, 1, 2, d);
			rect(sx + ox + 1, sy + oy - 1, 1, 2, d);
			rect(sx + ox + 2, sy + oy, 1, 2, d);
		}
		if ((h & 31) == 7)
			rect(sx + ((h >> 6) % 12) + 2, sy + ((h >> 10) % 12) + 2, 2, 2, RGB(160, 220, 120));
	}

	switch (t) {
	case T_PATH: {
		u32 edge = RGB(184, 162, 124);
		rect(sx, sy, TS, TS, RGB(224, 204, 164));
		blob(sx + 2 + (h & 3), sy + 3 + ((h >> 2) & 3), 5, 4, RGB(238, 223, 188), RGB(196, 174, 134));
		blob(sx + 9 - ((h >> 4) & 1), sy + 9 - ((h >> 5) & 1), 5, 4, RGB(238, 223, 188), RGB(196, 174, 134));
		rect(sx + 3, sy + 12, 3, 2, RGB(204, 184, 146));
		if (!isPathTile(tx, ty - 1)) rect(sx, sy, TS, 2, edge);
		if (!isPathTile(tx, ty + 1)) rect(sx, sy + TS - 2, TS, 2, edge);
		if (!isPathTile(tx - 1, ty)) rect(sx, sy, 2, TS, edge);
		if (!isPathTile(tx + 1, ty)) rect(sx + TS - 2, sy, 2, TS, edge);
		break;
	}
	case T_TREE:
	case T_APPLE:
		rect(sx + 1, sy + 12, 14, 3, SHAD);
		blob(sx + 6, sy + 8, 4, 8, RGB(150, 100, 70), OUT);
		blob(sx - 1, sy - 5, 18, 15, RGB(76, 160, 96), OUT);
		rect(sx + 2, sy + 5, 12, 3, RGB(58, 138, 82));          // underside shade
		blob(sx + 3, sy - 3, 8, 5, RGB(112, 196, 112), RGB(112, 196, 112)); // highlight
		rect(sx + 5, sy - 2, 3, 1, RGB(160, 226, 140));
		if (t == T_APPLE) {
			u32 a = RGB(236, 64, 72);
			rect(sx + 3, sy + 2, 3, 3, a);   rect(sx + 3, sy + 2, 1, 1, RGB(255, 190, 190));
			rect(sx + 10, sy, 3, 3, a);      rect(sx + 10, sy, 1, 1, RGB(255, 190, 190));
			rect(sx + 7, sy + 5, 3, 3, a);   rect(sx + 7, sy + 5, 1, 1, RGB(255, 190, 190));
		}
		break;
	case T_ROCK:
		rect(sx + 1, sy + 12, 14, 3, SHAD);
		blob(sx + 2, sy + 5, 12, 9, RGB(168, 168, 182), OUT);
		blob(sx + 4, sy + 3, 8, 5, RGB(196, 196, 210), OUT);
		rect(sx + 5, sy + 4, 3, 2, RGB(232, 232, 244));
		rect(sx + 4, sy + 11, 8, 2, RGB(140, 140, 156));
		break;
	case T_ROOF: {
		int row = ty - 14;
		u32 base = row == 0 ? RGB(214, 112, 88) : RGB(200, 94, 74);
		u32 dark = RGB(150, 62, 56);
		rect(sx, sy, TS, TS, base);
		rect(sx, sy + 7, TS, 1, dark);
		rect(sx, sy + 15, TS, 1, dark);
		int o = (row & 1) ? 4 : 10;
		rect(sx + o, sy, 1, 7, dark);
		rect(sx + ((o + 8) & 15), sy + 8, 1, 7, dark);
		rect(sx, sy + 1, TS, 1, RGB(232, 140, 108));
		rect(sx, sy + 9, TS, 1, RGB(222, 124, 98));
		if (row == 0) { rect(sx, sy, TS, 2, RGB(150, 62, 56)); }
		if (row == 1) { rect(sx, sy + 13, TS, 3, dark); }
		if (tx == 32 && row == 0) { // chimney
			blob(sx + 3, sy - 7, 10, 12, RGB(172, 108, 92), OUT);
			rect(sx + 4, sy - 5, 8, 1, RGB(140, 84, 72));
			rect(sx + 4, sy - 1, 8, 1, RGB(140, 84, 72));
			int p = (frameCount / 10) % 6;
			rect(sx + 6 + (p & 1), sy - 11 - p, 3, 3, RGBA(255, 255, 255, 150 - p * 20));
		}
		break;
	}
	case T_WALL: {
		rect(sx, sy, TS, TS, RGB(248, 232, 202));
		if (tx == 28 || tx == 33) rect(sx + (tx == 28 ? 0 : 14), sy, 2, TS, RGB(176, 124, 90));
		if (ty == 16) rect(sx, sy, TS, 3, RGBA(120, 70, 60, 60)); // eave shadow
		if (ty == 17) {
			rect(sx, sy + 11, TS, 5, RGB(176, 168, 164));
			rect(sx, sy + 11, TS, 1, RGB(150, 142, 140));
			rect(sx + 5, sy + 12, 1, 4, RGB(150, 142, 140));
			rect(sx + 11, sy + 12, 1, 4, RGB(150, 142, 140));
		}
		if (ty == 16 && (tx == 29 || tx == 33 || tx == 31)) {
			blob(sx + 2, sy + 3, 12, 10, RGB(176, 124, 90), OUT);
			rect(sx + 4, sy + 5, 8, 6, RGB(176, 224, 244));
			rect(sx + 4, sy + 5, 3, 2, RGB(255, 255, 255));
			rect(sx + 7, sy + 5, 2, 6, RGB(176, 124, 90));
			rect(sx + 4, sy + 8, 8, 1, RGB(176, 124, 90));
			rect(sx + 4, sy + 5, 1, 6, RGB(255, 170, 190)); // curtains
			rect(sx + 11, sy + 5, 1, 6, RGB(255, 170, 190));
			rect(sx + 1, sy + 13, 14, 3, RGB(150, 100, 70));
			rect(sx + 3, sy + 12, 2, 2, RGB(255, 120, 160));
			rect(sx + 7, sy + 12, 2, 2, RGB(255, 226, 110));
			rect(sx + 11, sy + 12, 2, 2, RGB(190, 160, 255));
		}
		break;
	}
	case T_DOOR:
		rect(sx, sy, TS, TS, RGB(248, 232, 202));
		rect(sx, sy + 11, TS, 5, RGB(176, 168, 164));
		blob(sx + 2, sy + 1, 12, 15, RGB(170, 112, 72), OUT);
		rect(sx + 7, sy + 2, 1, 13, RGB(130, 84, 56));
		rect(sx + 4, sy + 3, 2, 4, RGB(190, 134, 90));
		rect(sx + 10, sy + 3, 2, 4, RGB(190, 134, 90));
		rect(sx + 5, sy + 9, 2, 2, GOLD);
		rect(sx + 1, sy + 14, 14, 2, RGB(190, 182, 176));
		break;
	case T_FENCE: {
		u32 cr = RGB(250, 244, 230), dk = RGB(190, 176, 160);
		rect(sx, sy + 5, TS, 2, cr);  rect(sx, sy + 7, TS, 1, dk);
		rect(sx, sy + 10, TS, 2, cr); rect(sx, sy + 12, TS, 1, dk);
		for (int k = 0; k < 2; k++) {
			int x = sx + 2 + k * 8;
			rect(x, sy + 2, 4, 12, OUT);
			rect(x + 1, sy + 3, 2, 10, cr);
			rect(x + 1, sy + 1, 2, 1, OUT);
		}
		rect(sx, sy + 14, TS, 2, SHAD);
		break;
	}
	case T_MUSH:
		rect(sx + 3, sy + 12, 10, 3, SHAD);
		blob(sx + 6, sy + 8, 4, 6, RGB(250, 240, 224), OUT);
		blob(sx + 3, sy + 3, 10, 7, RGB(236, 84, 92), OUT);
		rect(sx + 5, sy + 4, 2, 2, RGB(255, 255, 255));
		rect(sx + 9, sy + 5, 2, 2, RGB(255, 255, 255));
		rect(sx + 7, sy + 7, 1, 1, RGB(255, 255, 255));
		break;
	case T_FLOWER:
		for (int k = 0; k < 2; k++) {
			int fx = sx + 4 + k * 7 + ((h >> (k * 4)) & 1);
			int fy = sy + 4 + ((h >> (k * 3 + 2)) & 3);
			u32 c = flowerCols[(h >> (k * 5 + 1)) % 5];
			rect(fx, fy + 3, 1, 6, RGB(70, 150, 70));
			rect(fx - 1 + swi, fy - 2, 3, 5, c);
			rect(fx - 2 + swi, fy - 1, 5, 3, c);
			rect(fx + swi, fy, 1, 1, RGB(255, 190, 70));
		}
		break;
	default:
		break;
	}
}

// ---------------------------------------------------------------- game state

typedef struct {
	float x, y;      // center, world pixels
	float fx, fy;    // facing (unit vector)
	int wp;
	int wait;
	float sus;       // suspicion 0..100
	int happy;
} Senpai;

typedef struct {
	float x, y;
	int state;       // 0 = in world, 1 = carried, 2 = given
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
	genMap();
	resetPlayer();

	sen.x = WP[3][0] * TS + TS / 2;
	sen.y = WP[3][1] * TS + TS / 2;
	sen.fx = 0; sen.fy = -1;
	sen.wp = 0;
	sen.wait = 60;
	sen.sus = 0;
	sen.happy = 0;

	int placed = 0, tries = 0;
	while (placed < NUM_GIFTS && tries < 5000) {
		tries++;
		int x = 2 + rand() % (MAP_W - 4);
		int y = 2 + rand() % (MAP_H - 4);
		if (solid(map[y][x]) || inYard(x, y)) continue;
		if (map[y][x] == T_PATH) continue;
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

static void updateGame(u32 kDown, u32 kHeld) {
	frameCount++;
	playerMoving = false;
	if (caughtTimer > 0) { caughtTimer--; return; }
	if (given >= NUM_GIFTS) return;

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
			sen.happy = 120;
			sen.wait = 90;
			sen.sus = 0;
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

// ---------------------------------------------------------------- drawing

// chibi character; (cx, fy) = horizontal center and feet line on screen
static void drawChar(int cx, int fy, float fx, float fyv, bool walking,
                     u32 hair, u32 body, u32 accent, bool girl) {
	int step = walking ? ((frameCount / 8) & 1) : 0;
	int b = step; // little bounce while walking
	u32 legc = girl ? SKIN : RGB(84, 72, 104);
	u32 shoe = RGB(110, 70, 70);

	rect(cx - 7, fy - 2, 14, 4, SHAD);
	rect(cx - 3, fy - 4 - (step ? 1 : 0), 2, 4, legc);
	rect(cx + 1, fy - 4 - (step ? 0 : 1), 2, 4, legc);
	rect(cx - 3, fy - 1 - (step ? 1 : 0), 2, 1, shoe);
	rect(cx + 1, fy - 1 - (step ? 0 : 1), 2, 1, shoe);

	blob(cx - 5, fy - 12 - b, 10, 9, body, OUT);
	if (girl) blob(cx - 6, fy - 7 - b, 12, 4, body, OUT);
	rect(cx - 3, fy - 11 - b, 6, 2, accent);
	if (girl) rect(cx - 1, fy - 10 - b, 2, 3, RGB(255, 120, 160)); // ribbon on dress

	blob(cx - 6, fy - 24 - b, 12, 12, SKIN, OUT);

	bool up = fyv < -0.4f;
	if (up) {
		blob(cx - 6, fy - 24 - b, 12, 11, hair, OUT);
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
		rect(cx - 8, fy - 20 - b, 2, 7, hair);   // pigtails
		rect(cx + 6, fy - 20 - b, 2, 7, hair);
		blob(cx + 1, fy - 26 - b, 6, 4, RGB(255, 120, 160), OUT); // bow
		rect(cx + 3, fy - 25 - b, 2, 2, GOLD);
	} else {
		rect(cx - 1, fy - 26 - b, 4, 2, hair);   // hair tuft
	}
}

static void drawGift(int gx, int gy, int i) {
	int bob = (int)(sinf((frameCount + i * 13) * 0.1f) * 2.0f);
	rect(gx - 5, gy + 4, 10, 3, SHAD);
	gy += bob;
	blob(gx - 5, gy - 5, 10, 10, RGB(255, 150, 190), OUT);
	rect(gx - 1, gy - 4, 2, 8, GOLD);
	rect(gx - 4, gy - 1, 8, 2, GOLD);
	rect(gx - 3, gy - 7, 3, 2, GOLD);
	rect(gx, gy - 7, 3, 2, GOLD);
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
		u32 c = RGBA(20, 24, 40, (6 - i) * 11);
		rect(0, i * 4, SCR_W, 4, c);
		rect(0, SCR_H - 4 - i * 4, SCR_W, 4, c);
		rect(i * 4, 0, 4, SCR_H, c);
		rect(SCR_W - 4 - i * 4, 0, 4, SCR_H, c);
	}
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

	int sx = (int)sen.x - camx, sy = (int)sen.y - camy;
	bool senActive = given < NUM_GIFTS;

	if (senActive && sen.happy == 0)
		drawCone(sx, sy, sen.fx, sen.fy, playerSneak ? 55.0f : 100.0f, sen.sus);

	for (int i = 0; i < NUM_GIFTS; i++)
		if (gifts[i].state == 0)
			drawGift((int)gifts[i].x - camx, (int)gifts[i].y - camy, i);

	int ppx = (int)px - camx, ppy = (int)py - camy;
	int pcxs = ppx + PLAYER_SIZE / 2, pfeet = ppy + PLAYER_SIZE;
	int sfeet = sy + 7;
	bool senWalk = senActive && sen.wait == 0;

	// draw whoever is further back first
	for (int pass = 0; pass < 2; pass++) {
		bool drawSenpai = (sfeet <= pfeet) ? (pass == 0) : (pass == 1);
		if (drawSenpai) {
			drawChar(sx, sfeet, sen.fx, sen.fy, senWalk,
			         RGB(58, 46, 80), RGB(96, 140, 220), RGB(255, 255, 255), false);
			if (sen.sus > 0) {
				blob(sx - 13, sfeet - 36, 26, 6, RGB(240, 230, 230), OUT);
				rect(sx - 11, sfeet - 34, (int)(22 * sen.sus / 100.0f), 2,
				     RGB(240, (int)(190 - sen.sus * 1.2f), 90));
			}
			if (sen.happy > 0) {
				int fl = (sen.happy % 24) / 6;
				drawHeart(sx - 3, sfeet - 40 - fl, 1, RGB(255, 90, 140));
				drawHeart(sx + 6, sfeet - 34 - ((fl + 2) & 3), 1, RGB(255, 150, 190));
			}
		} else {
			drawChar(pcxs, pfeet, pfx, pfy, playerMoving,
			         RGB(150, 92, 72), RGB(255, 160, 196), RGB(255, 255, 255), true);
			int c = 0;
			for (int i = 0; i < NUM_GIFTS; i++) if (gifts[i].state == 1) c++;
			for (int i = 0; i < c; i++)
				blob(pcxs + i * 6 - (c * 6) / 2, pfeet - 36, 6, 6, RGB(255, 150, 190), OUT);
		}
	}

	drawVignette();

	if (caughtTimer > 0) {
		blob(60, 86, 280, 56, CREAM, OUT);
		drawText(88, 94, 0.7f, RGB(214, 80, 110), "Caught staring!");
		drawText(80, 118, 0.5f, RGB(92, 64, 60), "He gave you a funny look... try again.");
	}
}

static void drawBottom(void) {
	int carried = 0;
	for (int i = 0; i < NUM_GIFTS; i++) if (gifts[i].state == 1) carried++;
	u32 ink = RGB(92, 64, 60);
	char buf[96];

	blob(8, 8, 304, 224, CREAM, OUT);
	drawText(22, 14, 0.8f, RGB(214, 90, 120), "Mossy Hollow");

	for (int i = 0; i < 8; i++)
		drawHeart(24 + i * 35, 54, 3, i < given ? RGB(240, 80, 130) : RGB(230, 212, 204));
	snprintf(buf, sizeof(buf), "Gifts given: %d / %d    Carrying: %d", given, NUM_GIFTS, carried);
	drawText(22, 82, 0.5f, ink, buf);

	drawText(22, 106, 0.5f, ink, "Suspicion");
	blob(22, 124, 276, 14, RGB(232, 216, 208), OUT);
	rect(24, 126, (int)(272 * sen.sus / 100.0f), 10, RGB(240, (int)(190 - sen.sus * 1.2f), 90));

	drawText(22, 154, 0.45f, ink, "Circle Pad / D-Pad : move");
	drawText(22, 170, 0.45f, ink, "Hold L : sneak (he sees you less)");
	drawText(22, 186, 0.45f, ink, "A near Senpai : give gifts");
	drawText(22, 202, 0.45f, RGB(214, 90, 120), "Tip: approach from behind!");
	drawText(22, 216, 0.38f, RGB(160, 140, 136), "START : quit");

	if (given >= NUM_GIFTS) {
		blob(30, 84, 260, 70, RGB(255, 226, 236), OUT);
		drawText(52, 94, 0.75f, RGB(214, 80, 110), "Senpai noticed you!");
		drawText(52, 124, 0.5f, ink, "You gave him all the gifts.");
	}
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

		C2D_TargetClear(top, RGB(60, 100, 70));
		C2D_SceneBegin(top);
		drawWorld();

		C2D_TargetClear(bot, RGB(86, 64, 108));
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
