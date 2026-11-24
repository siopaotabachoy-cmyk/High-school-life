// Mossy Hollow - a cute-but-obsessive top-down stealth game for the Nintendo 3DS
// Goal: collect little gifts and give them to Senpai without being caught staring.
// Sneak (hold L) or approach from behind to avoid raising his suspicion.
// Built with libctru + citro2d. All art is drawn with colored rectangles,
// so no image assets are needed. Swap in sprite sheets later if you like.

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

enum {
	T_GRASS, T_PATH, T_TREE, T_APPLE, T_ROCK, T_ROOF,
	T_WALL, T_DOOR, T_FENCE, T_MUSH, T_FLOWER
};

static u8 map[MAP_H][MAP_W];

static bool solid(u8 t) {
	return t == T_TREE || t == T_APPLE || t == T_ROCK || t == T_ROOF ||
	       t == T_WALL || t == T_DOOR || t == T_FENCE;
}

static u32 hash2(int x, int y) {
	u32 h = (u32)x * 374761393u + (u32)y * 668265263u;
	h = (h ^ (h >> 13)) * 1274126177u;
	return h ^ (h >> 16);
}

static void genMap(void) {
	srand(1234);
	memset(map, T_GRASS, sizeof(map));

	// Dirt road along the bottom
	for (int x = 0; x < MAP_W; x++)
		for (int y = 38; y <= 39; y++) map[y][x] = T_PATH;

	// Farmhouse
	const int hx = 28, hy = 14;
	for (int x = hx; x < hx + 6; x++) {
		map[hy][x] = T_ROOF;
		map[hy + 1][x] = T_ROOF;
		map[hy + 2][x] = T_WALL;
		map[hy + 3][x] = T_WALL;
	}
	map[hy + 3][hx + 3] = T_DOOR;

	// Stone path from the door down to the road
	for (int y = hy + 4; y < 38; y++) {
		map[y][hx + 3] = T_PATH;
		map[y][hx + 4] = T_PATH;
	}

	// Fence around the yard, with a gap where the path passes
	for (int x = 24; x <= 35; x++) {
		map[12][x] = T_FENCE;
		if (x != hx + 3 && x != hx + 4) map[22][x] = T_FENCE;
	}
	for (int y = 12; y <= 22; y++) {
		map[y][24] = T_FENCE;
		map[y][35] = T_FENCE;
	}

	// Border of trees so the player can't walk off the map
	for (int x = 0; x < MAP_W; x++) { map[0][x] = T_TREE; map[MAP_H - 1][x] = T_TREE; }
	for (int y = 0; y < MAP_H; y++) { map[y][0] = T_TREE; map[y][MAP_W - 1] = T_TREE; }

	// Scatter trees, rocks, mushrooms and flowers outside the yard
	for (int i = 0; i < 900; i++) {
		int x = 1 + rand() % (MAP_W - 2);
		int y = 1 + rand() % (MAP_H - 2);
		if (map[y][x] != T_GRASS) continue;
		if (x >= 22 && x <= 37 && y >= 10 && y <= 24) continue;
		int r = rand() % 100;
		if (r < 40) map[y][x] = T_TREE;
		else if (r < 50) map[y][x] = T_APPLE;
		else if (r < 60) map[y][x] = T_ROCK;
		else if (r < 80) map[y][x] = T_MUSH;
		else map[y][x] = T_FLOWER;
	}

	// A few flowers inside the yard
	for (int i = 0; i < 14; i++) {
		int x = 25 + rand() % 10;
		int y = 18 + rand() % 4;
		if (map[y][x] == T_GRASS) map[y][x] = T_FLOWER;
	}
}

static u32 tileColor(u8 t) {
	switch (t) {
		case T_PATH:   return RGB(200, 190, 160);
		case T_TREE:   return RGB(40, 110, 50);
		case T_APPLE:  return RGB(60, 130, 50);
		case T_ROCK:   return RGB(150, 150, 150);
		case T_ROOF:   return RGB(170, 40, 40);
		case T_WALL:   return RGB(90, 100, 120);
		case T_DOOR:   return RGB(130, 80, 40);
		case T_FENCE:  return RGB(110, 70, 40);
		case T_MUSH:   return RGB(200, 50, 50);
		case T_FLOWER: return RGB(230, 120, 170);
		default:       return RGB(90, 170, 60);
	}
}

static void drawTile(u8 t, int sx, int sy, int tx, int ty) {
	u32 h = hash2(tx, ty);
	// Grass underneath everything except path and buildings
	if (t != T_PATH && t != T_ROOF && t != T_WALL && t != T_DOOR) {
		C2D_DrawRectSolid(sx, sy, 0, TS, TS, RGB(90, 170, 60));
		if ((h & 3) == 0)
			C2D_DrawRectSolid(sx + ((h >> 4) & 11) + 2, sy + ((h >> 8) & 11) + 2, 0, 1, 2, RGB(65, 140, 45));
		if ((h & 15) == 5)
			C2D_DrawRectSolid(sx + ((h >> 6) & 11) + 2, sy + ((h >> 10) & 11) + 2, 0, 2, 2, RGB(120, 195, 80));
	}

	switch (t) {
	case T_PATH:
		C2D_DrawRectSolid(sx, sy, 0, TS, TS, RGB(200, 190, 160));
		C2D_DrawRectSolid(sx + 2, sy + 3, 0, 5, 4, RGB(225, 218, 195));
		C2D_DrawRectSolid(sx + 9, sy + 8, 0, 5, 5, RGB(225, 218, 195));
		C2D_DrawRectSolid(sx + 3, sy + 11, 0, 4, 3, RGB(170, 160, 135));
		break;
	case T_TREE:
	case T_APPLE:
		C2D_DrawRectSolid(sx + 6, sy + 10, 0, 4, 6, RGB(110, 70, 40));
		C2D_DrawRectSolid(sx + 1, sy + 1, 0, 14, 11, RGB(40, 110, 50));
		C2D_DrawRectSolid(sx + 3, sy + 2, 0, 8, 5, RGB(70, 150, 65));
		if (t == T_APPLE) {
			C2D_DrawRectSolid(sx + 4, sy + 6, 0, 2, 2, RGB(220, 40, 40));
			C2D_DrawRectSolid(sx + 10, sy + 4, 0, 2, 2, RGB(220, 40, 40));
			C2D_DrawRectSolid(sx + 8, sy + 9, 0, 2, 2, RGB(220, 40, 40));
		}
		break;
	case T_ROCK:
		C2D_DrawRectSolid(sx + 2, sy + 5, 0, 12, 9, RGB(130, 130, 135));
		C2D_DrawRectSolid(sx + 4, sy + 3, 0, 8, 4, RGB(165, 165, 170));
		C2D_DrawRectSolid(sx + 4, sy + 5, 0, 3, 3, RGB(190, 190, 195));
		break;
	case T_ROOF:
		C2D_DrawRectSolid(sx, sy, 0, TS, TS, RGB(170, 40, 40));
		C2D_DrawRectSolid(sx, sy + 5, 0, TS, 1, RGB(120, 25, 25));
		C2D_DrawRectSolid(sx, sy + 11, 0, TS, 1, RGB(120, 25, 25));
		C2D_DrawRectSolid(sx + ((ty & 1) ? 4 : 10), sy, 0, 1, 5, RGB(120, 25, 25));
		break;
	case T_WALL:
		C2D_DrawRectSolid(sx, sy, 0, TS, TS, RGB(90, 100, 120));
		C2D_DrawRectSolid(sx + 1, sy + 1, 0, 6, 6, RGB(110, 120, 140));
		C2D_DrawRectSolid(sx + 9, sy + 9, 0, 6, 6, RGB(110, 120, 140));
		if (tx % 2 == 0 && ty % 2 == 0) // little window
			C2D_DrawRectSolid(sx + 4, sy + 3, 0, 8, 8, RGB(120, 140, 220));
		break;
	case T_DOOR:
		C2D_DrawRectSolid(sx, sy, 0, TS, TS, RGB(90, 100, 120));
		C2D_DrawRectSolid(sx + 2, sy, 0, 12, TS, RGB(130, 80, 40));
		C2D_DrawRectSolid(sx + 11, sy + 8, 0, 2, 2, RGB(240, 200, 80));
		break;
	case T_FENCE:
		C2D_DrawRectSolid(sx, sy + 5, 0, TS, 2, RGB(110, 70, 40));
		C2D_DrawRectSolid(sx, sy + 10, 0, TS, 2, RGB(110, 70, 40));
		C2D_DrawRectSolid(sx + 2, sy + 2, 0, 3, 12, RGB(130, 85, 50));
		C2D_DrawRectSolid(sx + 11, sy + 2, 0, 3, 12, RGB(130, 85, 50));
		break;
	case T_MUSH:
		C2D_DrawRectSolid(sx + 6, sy + 9, 0, 4, 5, RGB(235, 225, 200));
		C2D_DrawRectSolid(sx + 3, sy + 4, 0, 10, 6, RGB(200, 50, 50));
		C2D_DrawRectSolid(sx + 5, sy + 5, 0, 2, 2, RGB(255, 255, 255));
		C2D_DrawRectSolid(sx + 9, sy + 6, 0, 2, 2, RGB(255, 255, 255));
		break;
	case T_FLOWER:
		C2D_DrawRectSolid(sx + 7, sy + 8, 0, 1, 5, RGB(50, 130, 45));
		C2D_DrawRectSolid(sx + 6, sy + 6, 0, 3, 3, ((h >> 3) & 1) ? RGB(235, 120, 170) : RGB(250, 220, 90));
		break;
	default:
		break;
	}
}

static bool blocked(float px, float py) {
	const float hw = 5, hh = 3;
	float xs[2] = { px - hw, px + hw };
	float ys[2] = { py - hh, py + hh };
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			if (xs[i] < 0 || ys[j] < 0) return true;
			int tx = (int)(xs[i] / TS), ty = (int)(ys[j] / TS);
			if (tx >= MAP_W || ty >= MAP_H) return true;
			if (solid(map[ty][tx])) return true;
		}
	}
	return false;
}

static void drawText(C2D_TextBuf buf, const char *s, float x, float y, float sc, u32 col) {
	C2D_Text t;
	C2D_TextParse(&t, buf, s);
	C2D_TextOptimize(&t);
	C2D_DrawText(&t, C2D_WithColor, x, y, 0.5f, sc, sc, col);
}

static void drawHeart(int x, int y, float z) {
	u32 c = RGB(255, 90, 150);
	C2D_DrawRectSolid(x, y, z, 2, 2, c);
	C2D_DrawRectSolid(x + 3, y, z, 2, 2, c);
	C2D_DrawRectSolid(x, y + 1, z, 5, 2, c);
	C2D_DrawRectSolid(x + 1, y + 3, z, 3, 1, c);
	C2D_DrawRectSolid(x + 2, y + 4, z, 1, 1, c);
}

// Small chibi-style character; (sx, sy) is the point between the feet.
static void drawChar(int sx, int sy, int fx, int fy, u32 hair, u32 top, u32 bottom, bool bow) {
	C2D_DrawRectSolid(sx - 5, sy - 1, 0.1f, 10, 3, RGB(60, 140, 50));      // shadow
	C2D_DrawRectSolid(sx - 4, sy - 4, 0.2f, 3, 5, RGB(240, 200, 160));     // left leg
	C2D_DrawRectSolid(sx + 1, sy - 4, 0.2f, 3, 5, RGB(240, 200, 160));     // right leg
	C2D_DrawRectSolid(sx - 5, sy - 8, 0.2f, 10, 5, bottom);                // skirt / pants
	C2D_DrawRectSolid(sx - 4, sy - 11, 0.2f, 8, 4, top);                   // top
	C2D_DrawRectSolid(sx - 5, sy - 19, 0.2f, 10, 8, RGB(245, 210, 175));   // big head
	C2D_DrawRectSolid(sx - 6, sy - 20, 0.3f, 12, 4, hair);                 // hair top
	C2D_DrawRectSolid(sx - 6, sy - 17, 0.3f, 2, 6, hair);                  // hair left
	C2D_DrawRectSolid(sx + 4, sy - 17, 0.3f, 2, 6, hair);                  // hair right
	if (fy >= 0) { // face visible unless facing away
		C2D_DrawRectSolid(sx - 3 + fx, sy - 15, 0.4f, 2, 3, RGB(30, 30, 50));
		C2D_DrawRectSolid(sx + 1 + fx, sy - 15, 0.4f, 2, 3, RGB(30, 30, 50));
		C2D_DrawRectSolid(sx - 3 + fx, sy - 15, 0.5f, 1, 1, RGB(255, 255, 255));
		C2D_DrawRectSolid(sx + 1 + fx, sy - 15, 0.5f, 1, 1, RGB(255, 255, 255));
		C2D_DrawRectSolid(sx - 5, sy - 13, 0.4f, 2, 1, RGB(255, 170, 170)); // blush
		C2D_DrawRectSolid(sx + 3, sy - 13, 0.4f, 2, 1, RGB(255, 170, 170));
	}
	if (bow) {
		C2D_DrawRectSolid(sx + 3, sy - 22, 0.5f, 3, 3, RGB(220, 40, 70));
		C2D_DrawRectSolid(sx + 6, sy - 21, 0.5f, 2, 1, RGB(220, 40, 70));
		C2D_DrawRectSolid(sx + 3, sy - 24, 0.5f, 3, 1, RGB(220, 40, 70));
	}
}

int main(int argc, char **argv) {
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(8192);
	C2D_Prepare();

	C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	C3D_RenderTarget *bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	C2D_TextBuf buf = C2D_TextBufNew(1024);

	genMap();

	float px = 31 * TS + 8, py = 19 * TS + 8;
	int fx = 0, fy = 1; // facing direction
	int gifts = 0;
	float affection = 0, suspicion = 0;
	char msg[64] = "Collect little gifts for Senpai... <3";
	// Senpai walks a loop: yard gate -> road -> east -> road -> west -> road -> gate
	const float wpx[6] = { 512, 512, 720, 512, 240, 512 };
	const float wpy[6] = { 376, 624, 624, 624, 624, 624 };
	int wp = 1, pause = 0;
	float cx = 512, cy = 376, cfx = 0, cfy = 1;
	bool won = false;

	while (aptMainLoop()) {
		hidScanInput();
		u32 kd = hidKeysDown(), kh = hidKeysHeld();
		if (kd & KEY_START) break;

		// --- Input ---
		float dx = 0, dy = 0;
		if (kh & KEY_DLEFT)  dx -= 1;
		if (kh & KEY_DRIGHT) dx += 1;
		if (kh & KEY_DUP)    dy -= 1;
		if (kh & KEY_DDOWN)  dy += 1;
		circlePosition cp;
		hidCircleRead(&cp);
		if (abs(cp.dx) > 20) dx += cp.dx / 156.0f;
		if (abs(cp.dy) > 20) dy -= cp.dy / 156.0f;
		float len = sqrtf(dx * dx + dy * dy);
		if (len > 1.0f) { dx /= len; dy /= len; }

		const float speed = (kh & KEY_B) ? 2.4f : 1.5f; // hold B to run
		if (dx != 0 || dy != 0) {
			if (fabsf(dx) > fabsf(dy)) { fx = dx > 0 ? 1 : -1; fy = 0; }
			else { fy = dy > 0 ? 1 : -1; fx = 0; }
		}
		if (!blocked(px + dx * speed, py)) px += dx * speed;
		if (!blocked(px, py + dy * speed)) py += dy * speed;

		// --- Interact (A) ---
		if (kd & KEY_A) {
			int ttx = (int)((px + fx * 12) / TS);
			int tty = (int)((py - 3 + fy * 12) / TS);
			int otx = (int)(px / TS), oty = (int)(py / TS);
			if (map[oty][otx] == T_MUSH) {
				map[oty][otx] = T_GRASS; gifts++;
				snprintf(msg, sizeof(msg), "Picked a mushroom!");
			} else if (ttx >= 0 && tty >= 0 && ttx < MAP_W && tty < MAP_H) {
				u8 t = map[tty][ttx];
				if (t == T_MUSH) {
					map[tty][ttx] = T_GRASS; gifts++;
					snprintf(msg, sizeof(msg), "Picked a mushroom!");
				} else if (t == T_APPLE) {
					map[tty][ttx] = T_TREE; gifts++;
					snprintf(msg, sizeof(msg), "Shook down an apple tree!");
				} else if (t == T_DOOR) {
					snprintf(msg, sizeof(msg), "Home sweet home.");
				} else if (t == T_FLOWER) {
					map[tty][ttx] = T_GRASS; gifts++;
					snprintf(msg, sizeof(msg), "A pretty flower. He'll love it.");
				}
			}
		}

		// --- Senpai wanders; don't get caught staring! ---
		if (!won) {
			if (pause > 0) {
				pause--;
			} else {
				float ddx = wpx[wp] - cx, ddy = wpy[wp] - cy;
				float dl = sqrtf(ddx * ddx + ddy * ddy);
				if (dl < 1.5f) { wp = (wp + 1) % 6; pause = 90; }
				else { cfx = ddx / dl; cfy = ddy / dl; cx += cfx * 0.7f; cy += cfy * 0.7f; }
			}

			float ex = px - cx, ey = (py - 8) - cy;
			float dist = sqrtf(ex * ex + ey * ey);
			bool sneaking = (kh & KEY_L) != 0;
			float range = sneaking ? 40.0f : 100.0f;
			bool inFront = (ex * cfx + ey * cfy) > 0;
			if (dist < range && inFront) {
				suspicion += 0.5f;
				snprintf(msg, sizeof(msg), "He's looking... smile. Act natural.");
			} else if (suspicion > 0) {
				suspicion -= 0.3f;
			}

			if ((kd & KEY_A) && dist < 28.0f) {
				if (gifts > 0) {
					gifts--;
					affection += 10;
					snprintf(msg, sizeof(msg), "He took your gift. Hehe... soon.");
					if (affection >= 100) {
						won = true;
						snprintf(msg, sizeof(msg), "Senpai noticed you. Forever <3");
					}
				} else {
					snprintf(msg, sizeof(msg), "Bring him something sweet first.");
				}
			}

			if (suspicion >= 100) {
				px = 31 * TS + 8; py = 19 * TS + 8;
				gifts = 0; suspicion = 0;
				snprintf(msg, sizeof(msg), "He got creeped out! Gifts lost...");
			}
		}

		// --- Camera ---
		int camx = (int)px - SCR_W / 2;
		int camy = (int)py - SCR_H / 2;
		if (camx < 0) camx = 0;
		if (camy < 0) camy = 0;
		if (camx > MAP_W * TS - SCR_W) camx = MAP_W * TS - SCR_W;
		if (camy > MAP_H * TS - SCR_H) camy = MAP_H * TS - SCR_H;

		// --- Draw ---
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TextBufClear(buf);

		C2D_TargetClear(top, RGB(90, 170, 60));
		C2D_SceneBegin(top);
		int tx0 = camx / TS, ty0 = camy / TS;
		int tx1 = (camx + SCR_W) / TS + 1, ty1 = (camy + SCR_H) / TS + 1;
		if (tx1 > MAP_W - 1) tx1 = MAP_W - 1;
		if (ty1 > MAP_H - 1) ty1 = MAP_H - 1;
		for (int ty = ty0; ty <= ty1; ty++)
			for (int tx = tx0; tx <= tx1; tx++)
				drawTile(map[ty][tx], tx * TS - camx, ty * TS - camy, tx, ty);

		// Characters (the one further up the screen is drawn first)
		int sx = (int)px - camx, sy = (int)py - camy;
		int ccx = (int)cx - camx, ccy = (int)cy - camy;
		int cdy = cfy > 0.3f ? 1 : (cfy < -0.3f ? -1 : 0);
		int cdx = cfx > 0.3f ? 1 : (cfx < -0.3f ? -1 : 0);
		u32 pHair = RGB(245, 140, 190), pTop = RGB(255, 235, 245), pSkirt = RGB(240, 110, 160);
		u32 sHair = RGB(40, 40, 60), sTop = RGB(70, 90, 160), sPants = RGB(50, 50, 70);
		if (cy < py) {
			drawChar(ccx, ccy, cdx, cdy, sHair, sTop, sPants, false);
			drawChar(sx, sy, fx, fy, pHair, pTop, pSkirt, true);
		} else {
			drawChar(sx, sy, fx, fy, pHair, pTop, pSkirt, true);
			drawChar(ccx, ccy, cdx, cdy, sHair, sTop, sPants, false);
		}
		drawHeart(sx - 2, sy - 32, 0.6f);                   // her little heart
		if (suspicion > 0)                                  // his "!" when he's suspicious
			C2D_DrawRectSolid(ccx - 1, ccy - 32, 0.6f, 3, 8, RGB(230, 40, 40));
		if (won) drawHeart(ccx - 2, ccy - 32, 0.6f);

		// Bottom screen: status + minimap
		C2D_TargetClear(bot, RGB(30, 40, 35));
		C2D_SceneBegin(bot);
		char line[64];
		snprintf(line, sizeof(line), "Gifts: %d", gifts);
		drawText(buf, line, 8, 4, 0.55f, RGB(255, 255, 255));
		drawText(buf, "<3", 214, 3, 0.4f, RGB(255, 120, 170));
		drawText(buf, "!", 218, 15, 0.4f, RGB(240, 70, 70));
		C2D_DrawRectSolid(232, 6, 0, 80, 7, RGB(60, 40, 50));
		C2D_DrawRectSolid(232, 6, 0.1f, affection * 0.8f, 7, RGB(255, 110, 170));
		C2D_DrawRectSolid(232, 18, 0, 80, 7, RGB(60, 40, 50));
		C2D_DrawRectSolid(232, 18, 0.1f, suspicion * 0.8f, 7, RGB(230, 50, 50));
		drawText(buf, msg, 8, 20, 0.45f, RGB(255, 230, 150));
		for (int ty = 0; ty < MAP_H; ty += 2)
			for (int tx = 0; tx < MAP_W; tx += 2)
				C2D_DrawRectSolid(32 + tx * 4, 40 + ty * 4, 0, 8, 8, tileColor(map[ty][tx]));
		C2D_DrawRectSolid(32 + (int)(px / TS) * 4 - 1, 40 + (int)(py / TS) * 4 - 1, 0.5f, 5, 5, RGB(255, 255, 0));
		drawText(buf, "Pad: move  B: run  L: sneak  A: pick/give  START: quit", 4, 226, 0.38f, RGB(180, 190, 180));

		C3D_FrameEnd(0);
	}

	C2D_TextBufDelete(buf);
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	return 0;
}
