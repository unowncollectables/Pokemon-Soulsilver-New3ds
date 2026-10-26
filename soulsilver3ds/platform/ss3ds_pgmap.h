/* ss3ds run 10: the Pokegear's Johto/Kanto map in the JOHTO bottom layout (bottom_layout=1).
   The art is the user's own ROM data, read on the game thread by pspoke hook 0081 (pgmap_gra NARC members 10 NCGR 8bpp
   tiles, 11 NSCR 64x64 tile sheet whose top-left 47x20 tiles are the whole map, 20 NCLR palette) and decoded here; nothing
   from the ROM is ever written to the SD card or committed. Locations are the game's own sLocationSpecs (x/y/size in map
   tiles). Visited locations are kept in sdmc:/3ds/soulsilver3ds/visited.bin ("VB01", trainer ID, 128-bit set), never in
   the save; a different trainer ID (new game) starts empty. Pure pixel/state code except the file I/O. */
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ss3ds_settings_ui.h"

extern "C" {
struct SS3DS_PgSpec { uint16_t mapId; uint8_t x, y; uint16_t bits; uint8_t flavorText, block, u8_, u9, uA, uB, srcX, srcY, dw, dh; };
extern const SS3DS_PgSpec sLocationSpecs[100];
volatile int P3DS_PgReady, P3DS_PgLoc = -1, P3DS_PgShoes = -1; volatile uint32_t P3DS_PgTid;
char P3DS_PgName[100][20]; char P3DS_PgText[100][100];
static uint8_t *pgRaw[3]; static uint32_t pgRawSz[3];
void *P3DS_PgAlloc(int member, uint32_t size) {
  const int k = member == 10 ? 0 : member == 11 ? 1 : 2; if (size < 16 || size > 128 * 1024) return 0;
  free(pgRaw[k]); pgRaw[k] = (uint8_t *)malloc(size); pgRawSz[k] = pgRaw[k] ? size : 0; return pgRaw[k]; }
}

enum { kPgW = 47 * 8, kPgH = 20 * 8, kPgN = 100 };
struct SPg {
  uint32_t img[kPgW * kPgH]; uint8_t lit[47 * 20]; bool decoded, failed;
  uint8_t visited[16]; uint32_t tid; bool tidKnown, dirty;
  int sel = -1; int viewX = -1; int lastLoc = -2;
};
static inline bool pgVis(const SPg &p, int i) { return i >= 0 && i < kPgN && (p.visited[i >> 3] >> (i & 7)) & 1; }
static inline void pgSpecRect(int i, int *x, int *y, int *w, int *h) {
  const SS3DS_PgSpec &l = sLocationSpecs[i]; *x = l.x; *y = l.y; *w = l.bits & 15; *h = (l.bits >> 4) & 15; if (*w < 1) *w = 1; if (*h < 1) *h = 1; }

/* find a Nitro section by its 4-byte magic (stored reversed in the file) */
static const uint8_t *pgSect(const uint8_t *d, uint32_t n, const char *m) {
  for (uint32_t i = 16; i + 8 <= n; i += 4) if (!memcmp(d + i, m, 4)) return d + i; return 0; }
static inline uint32_t pgU32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
static inline uint16_t pgU16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }

/* decode once the hook has delivered the three members; returns true when the picture is ready */
static bool pgDecode(SPg &p) {
  if (p.decoded) return true; if (p.failed || !P3DS_PgReady) return false;
  const uint8_t *chr = pgSect(pgRaw[0], pgRawSz[0], "RAHC"), *scr = pgSect(pgRaw[1], pgRawSz[1], "NRCS"), *pal = pgSect(pgRaw[2], pgRawSz[2], "TTLP");
  if (!chr || !scr || !pal) { p.failed = true; return false; }
  const uint8_t *C = chr + 8 + pgU32(chr + 28); const uint32_t cn = pgU32(chr + 24);
  const int sw = pgU16(scr + 8) / 8; const uint8_t *S = scr + 20;
  const uint8_t *P = pal + 8 + pgU32(pal + 20); const uint32_t pn = pgU32(pal + 16) / 2;
  if (sw < 47 || cn < 64 || pn < 16 || C + cn > pgRaw[0] + pgRawSz[0]) { p.failed = true; return false; }
  for (int ty = 0; ty < 20; ty++) for (int tx = 0; tx < 47; tx++) {
    const uint16_t e = pgU16(S + 2 * (ty * sw + tx)); const uint32_t t = e & 1023; const bool fx = e >> 10 & 1, fy = e >> 11 & 1;
    for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) {
      const int sx = fx ? 7 - x : x, sy = fy ? 7 - y : y; const uint32_t o = t * 64 + sy * 8 + sx; const int v = o < cn ? C[o] : 0;
      uint32_t c = SUI_RGB(16, 32, 72); /* index 0 = clear: the Pokegear's dark backdrop */
      if (v && (uint32_t)v < pn) { const uint16_t k = pgU16(P + 2 * v); c = SUI_RGB((k & 31) * 255 / 31, (k >> 5 & 31) * 255 / 31, (k >> 10 & 31) * 255 / 31); }
      p.img[(ty * 8 + y) * kPgW + tx * 8 + x] = c; } }
  p.decoded = true; return true; }

static void pgRelight(SPg &p) {
  memset(p.lit, 0, sizeof p.lit);
  for (int i = 0; i < kPgN; i++) if (pgVis(p, i)) { int x, y, w, h; pgSpecRect(i, &x, &y, &w, &h);
    for (int yy = y; yy < y + h && yy < 20; yy++) for (int xx = x; xx < x + w && xx < 47; xx++) if (xx >= 0 && yy >= 0) p.lit[yy * 47 + xx] = 1; } }

static void pgLoadVisited(SPg &p, uint32_t tid, const char *path) {
  memset(p.visited, 0, sizeof p.visited); p.tid = tid; p.tidKnown = true;
  FILE *f = fopen(path, "rb"); uint8_t b[24];
  if (f) { if (fread(b, 1, 24, f) == 24 && !memcmp(b, "VB01", 4) && pgU32(b + 4) == tid) memcpy(p.visited, b + 8, 16); fclose(f); }
  pgRelight(p); }
static bool pgSaveVisited(SPg &p, const char *path) {
  FILE *f = fopen(path, "wb"); if (!f) return false; uint8_t b[24]; memcpy(b, "VB01", 4);
  for (int i = 0; i < 4; i++) b[4 + i] = (uint8_t)(p.tid >> (8 * i)); memcpy(b + 8, p.visited, 16);
  const bool ok = fwrite(b, 1, 24, f) == 24; fclose(f); if (ok) p.dirty = false; return ok; }
/* returns 1 when a location was lit for the first time */
static int pgMark(SPg &p, int loc) {
  if (loc < 0 || loc >= kPgN || pgVis(p, loc)) return 0; p.visited[loc >> 3] |= 1 << (loc & 7); p.dirty = true; pgRelight(p); return 1; }
static int pgCount(const SPg &p) { int n = 0; for (int i = 0; i < kPgN; i++) n += pgVis(p, i); return n; }

/* location under map pixel (mx,my), smallest rect wins */
static int pgHitLoc(int mx, int my) {
  int best = -1, ba = 999; const int tx = mx / 8, ty = my / 8;
  for (int i = 0; i < kPgN; i++) { int x, y, w, h; pgSpecRect(i, &x, &y, &w, &h);
    if (tx >= x && tx < x + w && ty >= y && ty < y + h && w * h < ba) { ba = w * h; best = i; } }
  return best; }

/* simple word wrap into lines of at most `room` pixels */
static int pgWrap(const SUiText &T, const char *s, int room, char out[][64], int maxLines) {
  int n = 0; char line[64] = ""; const char *p = s;
  while (*p && n < maxLines) { char word[40]; int k = 0; while (*p == ' ') p++; while (*p && *p != ' ' && k < 39) word[k++] = *p++; word[k] = 0; if (!k) break;
    char tryl[100]; snprintf(tryl, sizeof tryl, "%s%s%s", line, line[0] ? " " : "", word);
    if (T.width(tryl) <= room || !line[0]) snprintf(line, sizeof line, "%s", tryl);
    else { snprintf(out[n++], 64, "%s", line); snprintf(line, sizeof line, "%s", word); } }
  if (line[0] && n < maxLines) snprintf(out[n++], 64, "%s", line); return n; }

/* draw the map into the panel interior (X0,Y0)-(X1,Y1); the view follows the player's location unless panned */
static void pgDraw(uint32_t *b, const SUiText &T, SPg &p, int X0, int Y0, int X1, int Y1, bool blink) {
  const int aw = X1 - X0, ah = Y1 - Y0;
  int cx = kPgW / 2; if (P3DS_PgLoc >= 0) { int x, y, w, h; pgSpecRect(P3DS_PgLoc, &x, &y, &w, &h); cx = x * 8 + w * 4; }
  if (p.viewX < 0) { p.viewX = cx - aw / 2; }
  if (p.viewX > kPgW - aw) p.viewX = kPgW - aw; if (p.viewX < 0) p.viewX = 0;
  const int oy = Y0 + (ah - kPgH) / 2;
  const uint32_t dimTo = SUI_RGB(14, 22, 48);
  for (int y = Y0; y < Y1; y++) { const int my = y - oy;
    for (int x = X0; x < X1; x++) { const int mx = x - X0 + p.viewX;
      if (my < 0 || my >= kPgH || mx < 0 || mx >= kPgW) { suiPx(b, x, y, SUI_RGB(16, 32, 72)); continue; }
      const uint32_t c = p.img[my * kPgW + mx]; suiPx(b, x, y, p.lit[(my / 8) * 47 + mx / 8] ? c : suiMix(c, dimTo, 5, 8)); } }
  /* selected location outline, then the player marker */
  if (p.sel >= 0) { int x, y, w, h; pgSpecRect(p.sel, &x, &y, &w, &h); const int sx = X0 + x * 8 - p.viewX, sy = oy + y * 8;
    for (int k = 0; k < 2; k++) for (int xx = sx - k; xx < sx + w * 8 + k; xx++) { if (xx >= X0 && xx < X1) { suiPx(b, xx, sy - 1 - k, SUI_RGB(255, 224, 64)); suiPx(b, xx, sy + h * 8 + k, SUI_RGB(255, 224, 64)); } }
    for (int k = 0; k < 2; k++) for (int yy = sy - k; yy < sy + h * 8 + k; yy++) { if (sx - 1 - k >= X0) suiPx(b, sx - 1 - k, yy, SUI_RGB(255, 224, 64)); if (sx + w * 8 + k < X1) suiPx(b, sx + w * 8 + k, yy, SUI_RGB(255, 224, 64)); } }
  if (P3DS_PgLoc >= 0) { int x, y, w, h; pgSpecRect(P3DS_PgLoc, &x, &y, &w, &h); const int px = X0 + x * 8 + w * 4 - p.viewX, py = oy + y * 8 + h * 4;
    if (px >= X0 + 3 && px < X1 - 3) mmPlayer(b, px, py, blink); }
  /* info card for the tapped location */
  if (p.sel >= 0) { const int cy0 = Y1 - 58; char ln[4][64];
    for (int y = cy0; y < Y1; y++) for (int x = X0; x < X1; x++) suiPx(b, x, y, suiMix(SUI_RGB(236, 240, 248), SUI_RGB(196, 206, 226), y - cy0, 58));
    for (int x = X0; x < X1; x++) suiPx(b, x, cy0, SUI_RGB(30, 48, 96));
    T.draw(b, X0 + 4, cy0 + 3, P3DS_PgName[p.sel][0] ? P3DS_PgName[p.sel] : "?", SUI_RGB(30, 48, 96), SUI_RGB(236, 240, 248));
    const char *st = pgVis(p, p.sel) ? "VISITED" : "NOT YET"; T.draw(b, X1 - 4 - T.width(st), cy0 + 3, st, pgVis(p, p.sel) ? SUI_RGB(40, 120, 64) : SUI_RGB(140, 100, 60), SUI_RGB(236, 240, 248));
    const int n = pgWrap(T, P3DS_PgText[p.sel], aw - 8, ln, 4); for (int i = 0; i < n; i++) T.draw(b, X0 + 4, cy0 + 16 + i * 10, ln[i], SUI_RGB(56, 64, 88), SUI_RGB(236, 240, 248)); }
  else { char buf[32]; snprintf(buf, sizeof buf, "%d/%d VISITED", pgCount(p), kPgN); T.draw(b, X1 - 4 - T.width(buf), Y1 - 12, buf, SUI_RGB(200, 212, 236), SUI_RGB(16, 32, 72)); } }
