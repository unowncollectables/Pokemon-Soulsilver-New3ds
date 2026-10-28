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
#include "p3ds_speciesnames.h"
#include "tracker.h"

extern "C" {
struct SS3DS_PgSpec { uint16_t mapId; uint8_t x, y; uint16_t bits; uint8_t flavorText, block, u8_, u9, uA, uB, srcX, srcY, dw, dh; };
extern const SS3DS_PgSpec sLocationSpecs[100];
volatile int P3DS_PgReady, P3DS_PgLoc = -1, P3DS_PgShoes = -1; volatile uint32_t P3DS_PgTid;
char P3DS_PgName[100][20]; char P3DS_PgText[100][100];
/* buffers the game thread fills (hook 0081). Keys: pgmap_gra member (0..63), 100 + a/0/1/4 member, 200/201 = registered
   item icon chars/palette (fixed 16 KB slots, refilled in place when the registered item changes). */
static uint8_t *pgRaw[256]; static uint32_t pgRawSz[256];
volatile int P3DS_PgGender = -1, P3DS_PgItem = -1;
/* run 13: progress per Pokegear location from pspoke hook 0083 (read-only flag checks on the game thread). Row i < 100:
   [0] species available, [1] caught, [2] seen, [3] trainers, [4] beaten, [5] visible items, [6] taken, [7] hidden items,
   [8] found. Row 100 = whole map (species counted once). */
/* run 16: hook 0083's ROM tables, kept here because the field overlay's .bss is cleared on every reload (each battle) */
uint16_t P3DS_PsKId[2400]; uint8_t P3DS_PsKLoc[2400], P3DS_PsKKind[2400], P3DS_PsKSp[100][64]; int P3DS_PsKN, P3DS_PsKBuilt;
volatile int P3DS_PsReady; volatile uint32_t P3DS_PsTick; uint16_t P3DS_PsStat[101][9];
/* run 16: hook 0084 publishes the wild species set per location (bit = species 1..493) and which of them are caught */
uint8_t P3DS_PsSp[100][64], P3DS_PsCg[100][64];
void *P3DS_PgAlloc(int key, uint32_t size) {
  if (key < 0 || key > 255 || size < 16 || size > 64 * 1024) return 0;
  if (key >= 200 && key <= 201) { if (size > 0x4000) return 0; if (!pgRaw[key]) pgRaw[key] = (uint8_t *)malloc(0x4000); pgRawSz[key] = pgRaw[key] ? size : 0; return pgRaw[key]; }
  free(pgRaw[key]); pgRaw[key] = (uint8_t *)malloc(size); pgRawSz[key] = pgRaw[key] ? size : 0; return pgRaw[key]; }
}
/* compatibility for the run 10 decode (members 10/11/20) */
#define pgRawM(m) pgRaw[m]
#define pgRawSzM(m) pgRawSz[m]

enum { kPgW = 47 * 8, kPgH = 20 * 8, kPgN = 100 };
struct SPg {
  uint32_t img[kPgW * kPgH]; uint8_t lit[47 * 20]; bool decoded, failed;
  uint8_t visited[16]; uint32_t tid; bool tidKnown, dirty;
  int sel = -1; int viewX = -1; int lastLoc = -2;
  int markX, markY; bool markOk, gameMarker; /* run 11: marker position (DS px) for the native overlay, and whether the game's head is used */
  bool ov; int ovScroll; int ovRows[kPgN]; int ovN, ovListY0, ovListY1; /* run 13: progress overview (menu-dots button) */
  struct { short x, y; uint8_t k; } ico[48]; int nIco; bool gameIco; /* small stat icons drawn by the native overlay */
  int dotsX0, dotsY0, dotsX1, dotsY1;
  int page; /* run 16: info card page, 0 = info, 1 = wild Pokemon list (tap the card to flip) */
  bool glide, moving; /* run 16: view slides to the player's new location instead of jumping; moving = still sliding (redraw) */
  int kantoX = -2, kantoAlways = -1; bool kantoOpen; uint8_t hide[47 * 20]; /* run 16: Kanto hidden until a Kanto place is visited */
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
  const uint8_t *chr = pgSect(pgRaw[10], pgRawSz[10], "RAHC"), *scr = pgSect(pgRaw[11], pgRawSz[11], "NRCS"), *pal = pgSect(pgRaw[20], pgRawSz[20], "TTLP");
  if (!chr || !scr || !pal) { p.failed = true; return false; }
  const uint8_t *C = chr + 8 + pgU32(chr + 28); const uint32_t cn = pgU32(chr + 24);
  const int sw = pgU16(scr + 8) / 8; const uint8_t *S = scr + 20;
  const uint8_t *P = pal + 8 + pgU32(pal + 20); const uint32_t pn = pgU32(pal + 16) / 2;
  if (sw < 47 || cn < 64 || pn < 16 || C + cn > pgRaw[10] + pgRawSz[10]) { p.failed = true; return false; }
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

/* run 14: labelled stats ("Caught 2/9", "Trainers 1/3", "Items 1/2", "Hidden 0/2", optional "Seen"), two per line in
   columns at x and x + col; only stats the place has. Returns the number of lines drawn. */
static int pgStats(uint32_t *b, const SUiText &T, SPg &p, int x, int y, int loc, uint32_t fg, uint32_t sh, int col, bool seen = false) {
  const uint16_t *s = P3DS_PsStat[loc]; struct { const char *l; int a, n; } it[5]; int k = 0;
  if (s[0]) it[k++] = { "Caught", s[1], s[0] }; if (seen && s[0]) it[k++] = { "Seen", s[2], s[0] };
  if (s[3]) it[k++] = { "Trainers", s[4], s[3] }; if (s[5]) it[k++] = { "Items", s[6], s[5] }; if (s[7]) it[k++] = { "Hidden", s[8], s[7] };
  if (!k) { T.draw(b, x, y, "Nothing to collect here", SUI_RGB(120, 128, 150), sh); return 1; }
  for (int i = 0; i < k; i++) { char t[32]; snprintf(t, sizeof t, "%s %d/%d", it[i].l, it[i].a, it[i].n);
    T.draw(b, x + (i & 1) * col, y + (i / 2) * 10, t, it[i].a >= it[i].n ? SUI_RGB(40, 120, 64) : fg, sh); }
  return (k + 1) / 2; }
static void pgDots(uint32_t *b, SPg &p, int X1, int Y0) {
  p.dotsX0 = X1 - 20; p.dotsY0 = Y0 + 2; p.dotsX1 = X1 - 2; p.dotsY1 = Y0 + 12;
  for (int y = p.dotsY0; y < p.dotsY1; y++) for (int x = p.dotsX0; x < p.dotsX1; x++) {
    const bool edge = y == p.dotsY0 || y == p.dotsY1 - 1 || x == p.dotsX0 || x == p.dotsX1 - 1;
    suiPx(b, x, y, edge ? kBlNavy : suiMix(SUI_RGB(248, 248, 252), SUI_RGB(200, 210, 230), y - p.dotsY0, 10)); }
  for (int d = 0; d < 3; d++) for (int yy = 0; yy < 2; yy++) for (int xx = 0; xx < 2; xx++) suiPx(b, p.dotsX0 + 4 + d * 4 + xx, p.dotsY0 + 4 + yy, SUI_RGB(30, 48, 96)); }
/* run 45: tracker block (roaming Pokemon + Day-Care) at the top of the overview; returns the height used, 0 when off */
static const uint32_t kTrkCol[4] = { SUI_RGB(232, 184, 16), SUI_RGB(224, 72, 40), SUI_RGB(240, 96, 144), SUI_RGB(64, 112, 232) };
static int pgTracker(uint32_t *b, const SUiText &T, int X0, int Y, int X1, uint32_t ink, uint32_t fg, uint32_t sh) {
  if (!P3DS_TrkOn()) return 0; int y = Y;
  T.draw(b, X0 + 4, y, "ROAMING NOW", ink, sh); y += 10;
  if (!P3DS_TrkReady) { T.draw(b, X0 + 4, y, "Take a step to look.", fg, sh); return y + 12 - Y; }
  int shown = 0;
  for (int i = 0; i < 4; i++) { const P3DS_TrkRoamer &r = P3DS_TrkRoam[i]; if (!r.active || !r.species) continue; char t[64];
    const char *l = r.loc >= 0 && r.loc < kPgN && P3DS_PgName[r.loc][0] ? P3DS_PgName[r.loc] : "somewhere";
    for (int yy = 0; yy < 7; yy++) for (int xx = 0; xx < 7; xx++) suiPx(b, X0 + 5 + xx, y + 1 + yy, xx == 0 || yy == 0 || xx == 6 || yy == 6 ? SUI_RGB(30, 48, 96) : kTrkCol[i]);
    snprintf(t, sizeof t, "%s Lv%d: %s", kP3dsSpecies[r.species], r.level, l); T.draw(b, X0 + 15, y, t, fg, sh); y += 10; shown++; }
  if (!shown) { T.draw(b, X0 + 4, y, "None out yet.", fg, sh); y += 10; }
  y += 2; char t[64];
  T.draw(b, X0 + 4, y, "DAY-CARE", ink, sh); y += 10; const P3DS_TrkDaycare &d = P3DS_TrkDc;
  if (d.n == 0) { T.draw(b, X0 + 4, y, "Empty.", fg, sh); y += 10; }
  else if (d.n == 1) { T.draw(b, X0 + 4, y, "One Pokemon: needs a partner.", fg, sh); y += 10; }
  else {
    snprintf(t, sizeof t, "%s + %s", kP3dsSpecies[d.sp[0]], kP3dsSpecies[d.sp[1]]); T.draw(b, X0 + 4, y, t, fg, sh); y += 10;
    if (d.compat > 0) snprintf(t, sizeof t, "Egg chance %d%% per 256 steps", d.compat); else snprintf(t, sizeof t, "These two cannot breed.");
    T.draw(b, X0 + 4, y, t, d.compat > 0 ? fg : SUI_RGB(140, 100, 60), sh); y += 10;
    if (d.compat > 0) {
      if (d.egg) T.draw(b, X0 + 4, y, "An egg is waiting!", SUI_RGB(40, 120, 64), sh);
      else { snprintf(t, sizeof t, "Next egg roll in %d steps", d.toCheck < 0 ? 0 : d.toCheck); T.draw(b, X0 + 4, y, t, fg, sh); }
      y += 10;
      T.draw(b, X0 + 4, y, d.masuda ? "Masuda: YES, shiny 5/8192 (1/1638)" : "Masuda: no, shiny 1/8192", d.masuda ? SUI_RGB(40, 120, 64) : fg, sh); y += 10; } }
  return y + 2 - Y; }
/* the overview: whole-map totals, then every visited location with its stats (drag scrolls, tap opens its card) */
static void pgOverview(uint32_t *b, const SUiText &T, SPg &p, int X0, int Y0, int X1, int Y1) {
  const uint32_t ink = SUI_RGB(30, 48, 96), sh = SUI_RGB(236, 240, 248);
  for (int y = Y0; y < Y1; y++) for (int x = X0; x < X1; x++) suiPx(b, x, y, suiMix(SUI_RGB(244, 246, 252), SUI_RGB(206, 214, 232), y - Y0, Y1 - Y0));
  char t[64]; int n = 0; for (int i = 0; i < kPgN; i++) n += pgVis(p, i);
  T.draw(b, X0 + 4, Y0 + 2, "PROGRESS", ink, sh); snprintf(t, sizeof t, "%d/%d VISITED", n, kPgN); T.draw(b, X1 - 24 - T.width(t), Y0 + 2, t, SUI_RGB(56, 64, 88), sh);
  if (!P3DS_PsReady) { T.draw(b, X0 + 4, Y0 + 16, "Take a step to count.", SUI_RGB(56, 64, 88), sh); p.ovN = 0; return; }
  const int dy = pgTracker(b, T, X0, Y0 + 13, X1, ink, SUI_RGB(56, 64, 88), sh);
  T.draw(b, X0 + 4, Y0 + 13 + dy, "Whole map:", SUI_RGB(56, 64, 88), sh);
  const int tl = pgStats(b, T, p, X0 + 4, Y0 + 24 + dy, 100, ink, sh, (X1 - X0) / 2, true); const int hy = Y0 + 27 + dy + tl * 10;
  for (int x = X0; x < X1; x++) suiPx(b, x, hy, SUI_RGB(104, 120, 168));
  p.ovN = 0; for (int i = 0; i < kPgN; i++) if (pgVis(p, i)) { bool dup = false; for (int j = 0; j < p.ovN; j++) if (!strcmp(P3DS_PgName[p.ovRows[j]], P3DS_PgName[i])) dup = true; if (!dup) p.ovRows[p.ovN++] = i; }
  p.ovListY0 = hy + 2; p.ovListY1 = Y1; const int rowH = 32, maxS = p.ovN * rowH - (Y1 - p.ovListY0); if (p.ovScroll > maxS) p.ovScroll = maxS; if (p.ovScroll < 0) p.ovScroll = 0;
  if (!p.ovN) T.draw(b, X0 + 4, p.ovListY0 + 4, "Visit places to list them here.", SUI_RGB(56, 64, 88), sh);
  for (int r = 0; r < p.ovN; r++) { const int y = p.ovListY0 + r * rowH - p.ovScroll; if (y < p.ovListY0 || y + rowH > Y1) continue; const int l = p.ovRows[r];
    if (r & 1) for (int yy = y; yy < y + rowH; yy++) for (int x = X0; x < X1; x++) suiPx(b, x, yy, suiMix(b[yy * 256 + x], SUI_RGB(255, 255, 255), 1, 2));
    T.draw(b, X0 + 4, y + 1, P3DS_PgName[l][0] ? P3DS_PgName[l] : "?", ink, sh); pgStats(b, T, p, X0 + 10, y + 11, l, SUI_RGB(56, 64, 88), sh, (X1 - X0 - 10) / 2); }
  if (maxS > 0) { const int h = Y1 - p.ovListY0, bh = h * h / (h + maxS), by = p.ovListY0 + (h - bh) * p.ovScroll / maxS; for (int y = by; y < by + bh; y++) { suiPx(b, X1 - 3, y, SUI_RGB(104, 120, 168)); suiPx(b, X1 - 2, y, SUI_RGB(104, 120, 168)); } } }
/* overview row under DS y, -1 none */
static int pgOvHit(const SPg &p, int y) { if (!p.ov || y < p.ovListY0 || y >= p.ovListY1) return -1; const int r = (y - p.ovListY0 + p.ovScroll) / 32; return r >= 0 && r < p.ovN ? p.ovRows[r] : -1; }

/* ---- run 16: Kanto on the map only after you have been there (the real Pokegear shows it after the S.S. Aqua).
   Kanto places are recognised by their names (from the ROM via hook 0081); the hidden area starts at the westmost Kanto
   town's column, but places that are not Kanto (Route 26/27, Victory Road, Indigo Plateau, Mt. Silver...) stay visible even
   when they lie further east. The gate opens for good when a Kanto place is in visited.bin or you are standing in one. */
static bool pgIsKanto(int i) {
  if (i < 0 || i >= kPgN) return false; const char *n = P3DS_PgName[i]; char u[24]; int k = 0;
  for (; n[k] && k < 23; k++) u[k] = (char)(n[k] >= 'a' && n[k] <= 'z' ? n[k] - 32 : n[k]); u[k] = 0;
  static const char *const town[] = { "PALLET", "VIRIDIAN", "PEWTER", "CERULEAN", "VERMILION", "LAVENDER", "CELADON", "FUCHSIA", "SAFFRON", "CINNABAR",
    "SEAFOAM", "DIGLETT", "MT. MOON", "MT.MOON", "ROCK TUNNEL", "POWER PLANT" };
  for (const char *t : town) if (!strncmp(u, t, strlen(t))) return true;
  if (!strncmp(u, "ROUTE ", 6)) { const int r = atoi(u + 6); return r >= 1 && r <= 25; }
  return false; }
static bool pgIsKantoTown(int i) { return pgIsKanto(i) && strncmp(P3DS_PgName[i], "Route", 5) && strncmp(P3DS_PgName[i], "ROUTE", 5); }
/* recompute the gate; returns true when it changed (the caller logs and redraws) */
static bool pgKantoUpdate(SPg &p, int always) {
  if (p.kantoX == -2) { p.kantoX = -1; for (int i = 0; i < kPgN; i++) if (pgIsKantoTown(i)) { int x, y, w, h; pgSpecRect(i, &x, &y, &w, &h); if (p.kantoX < 0 || x < p.kantoX) p.kantoX = x; } }
  bool open = always || p.kantoX < 0 || pgIsKanto(P3DS_PgLoc); for (int i = 0; i < kPgN && !open; i++) if (pgIsKanto(i) && pgVis(p, i)) open = true;
  const bool changed = open != p.kantoOpen || always != p.kantoAlways || p.kantoX == -1; p.kantoOpen = open; p.kantoAlways = always;
  if (!changed) return false;
  memset(p.hide, 0, sizeof p.hide);
  if (!open) { for (int ty = 0; ty < 20; ty++) for (int tx = p.kantoX; tx < 47; tx++) p.hide[ty * 47 + tx] = 1;
    for (int pass = 0; pass < 2; pass++) for (int i = 0; i < kPgN; i++) { if (pgIsKanto(i) != (pass == 0)) continue; int x, y, w, h; pgSpecRect(i, &x, &y, &w, &h); /* Kanto rects hidden first, then non-Kanto rects shown */
      for (int yy = y; yy < y + h && yy < 20; yy++) for (int xx = x; xx < x + w && xx < 47; xx++) if (xx >= 0 && yy >= 0) p.hide[yy * 47 + xx] = pass == 0; } }
  if (p.kantoX == -1) p.kantoX = -3; /* "no Kanto town found" reported once */
  return true; }
static inline bool pgHiddenAt(const SPg &p, int mx, int my) { return mx >= 0 && my >= 0 && mx < kPgW && my < kPgH && p.hide[(my / 8) * 47 + mx / 8]; }
static inline int pgCardH(const SPg &p) { return p.page ? 110 : 66; }
/* species in a location's wild set; fills `out` (up to max), returns the count, `*caught` = how many are caught */
static int pgWild(int loc, int *out, int max, int *caught) {
  int n = 0; *caught = 0; if (loc < 0 || loc >= kPgN) return 0;
  for (int sp = 1; sp <= 493; sp++) if (P3DS_PsSp[loc][sp >> 3] >> (sp & 7) & 1) { if (n < max) out[n] = sp; n++; if (P3DS_PsCg[loc][sp >> 3] >> (sp & 7) & 1) (*caught)++; }
  return n; }

/* draw the map into the panel interior (X0,Y0)-(X1,Y1); the view follows the player's location unless panned */
static void pxMapPins(uint32_t *b, const SPg &p, int X0, int X1, int oy, int viewX); /* ss3ds_pgextra.h: phone contact pins */
static bool pxCardText(const SUiText &T, char *out, int sz, int loc, int maxW);
static void pgDraw(uint32_t *b, const SUiText &T, SPg &p, int X0, int Y0, int X1, int Y1, bool blink) {
  const int aw = X1 - X0, ah = Y1 - Y0; p.nIco = 0;
  if (p.ov) { p.markOk = false; pgOverview(b, T, p, X0, Y0, X1, Y1); pgDots(b, p, X1, Y0); return; }
  int cx = kPgW / 2; if (P3DS_PgLoc >= 0) { int x, y, w, h; pgSpecRect(P3DS_PgLoc, &x, &y, &w, &h); cx = x * 8 + w * 4; }
  const int vw = !p.kantoOpen && p.kantoX > 0 && p.kantoX * 8 > aw ? p.kantoX * 8 : kPgW; /* Kanto hidden: do not scroll into the empty east */
  int goal = cx - aw / 2; if (goal > vw - aw) goal = vw - aw; if (goal < 0) goal = 0;
  if (p.viewX < 0) { p.viewX = goal; p.glide = false; }
  else if (p.glide) { const int d = goal - p.viewX; if (d >= -1 && d <= 1) { p.viewX = goal; p.glide = false; } else { const int q = P3DS_FpsTweaks() >= 5 ? 2 : 4; /* level 5: half the repaints, double the step */ p.viewX += d / q ? d / q : (d > 0 ? 1 : -1); } }
  p.moving = p.glide;
  if (p.viewX > vw - aw) p.viewX = vw - aw; if (p.viewX < 0) p.viewX = 0;
  const int oy = Y0 + (ah - kPgH) / 2;
  const uint32_t dimTo = SUI_RGB(14, 22, 48);
  for (int y = Y0; y < Y1; y++) { const int my = y - oy;
    for (int x = X0; x < X1; x++) { const int mx = x - X0 + p.viewX;
      if (my < 0 || my >= kPgH || mx < 0 || mx >= kPgW) { suiPx(b, x, y, SUI_RGB(16, 32, 72)); continue; }
      if (p.hide[(my / 8) * 47 + mx / 8]) { suiPx(b, x, y, SUI_RGB(16, 32, 72)); continue; }
      const uint32_t c = p.img[my * kPgW + mx]; suiPx(b, x, y, p.lit[(my / 8) * 47 + mx / 8] ? c : suiMix(c, dimTo, 5, 8)); } }
  pxMapPins(b, p, X0, X1, oy, p.viewX);
  /* selected location outline, then the player marker */
  if (p.sel >= 0) { int x, y, w, h; pgSpecRect(p.sel, &x, &y, &w, &h); const int sx = X0 + x * 8 - p.viewX, sy = oy + y * 8;
    for (int k = 0; k < 2; k++) for (int xx = sx - k; xx < sx + w * 8 + k; xx++) { if (xx >= X0 && xx < X1) { suiPx(b, xx, sy - 1 - k, SUI_RGB(255, 224, 64)); suiPx(b, xx, sy + h * 8 + k, SUI_RGB(255, 224, 64)); } }
    for (int k = 0; k < 2; k++) for (int yy = sy - k; yy < sy + h * 8 + k; yy++) { if (sx - 1 - k >= X0) suiPx(b, sx - 1 - k, yy, SUI_RGB(255, 224, 64)); if (sx + w * 8 + k < X1) suiPx(b, sx + w * 8 + k, yy, SUI_RGB(255, 224, 64)); } }
  if (P3DS_PgLoc >= 0) { int x, y, w, h; pgSpecRect(P3DS_PgLoc, &x, &y, &w, &h); const int px = X0 + x * 8 + w * 4 - p.viewX, py = oy + y * 8 + h * 4;
    p.markX = px; p.markY = py; p.markOk = px >= X0 + 3 && px < X1 - 3 && (p.sel < 0 || py < Y1 - pgCardH(p) - 6);
    if (px >= X0 + 3 && px < X1 - 3 && !p.gameMarker) mmPlayer(b, px, py, blink); } else p.markOk = false;
  /* run 45: roamer marks (tracker): a coloured square on the roamer's location, R / E / L / L */
  if (P3DS_TrkOn() && P3DS_TrkReady) for (int i = 0; i < 4; i++) { const P3DS_TrkRoamer &r = P3DS_TrkRoam[i]; if (!r.active || r.loc < 0 || r.loc >= kPgN) continue;
    int x, y, w, h; pgSpecRect(r.loc, &x, &y, &w, &h); const int rx = X0 + x * 8 + w * 4 - p.viewX, ry = oy + y * 8 + h * 4;
    if (pgHiddenAt(p, x * 8 + w * 4, y * 8 + h * 4) || rx < X0 + 5 || rx >= X1 - 5 || ry < Y0 + 5 || ry >= Y1 - 5) continue;
    for (int yy = -4; yy <= 4; yy++) for (int xx = -4; xx <= 4; xx++) suiPx(b, rx + xx, ry + yy, xx == -4 || xx == 4 || yy == -4 || yy == 4 ? SUI_RGB(30, 48, 96) : kTrkCol[i]);
    static const char *const kIni[4] = { "R", "E", "L", "L" }; const char *ini = kIni[i]; T.draw(b, rx - T.width(ini) / 2, ry - 4, ini, SUI_RGB(30, 30, 40), kTrkCol[i]); }
  /* info card for the tapped location */
  if (p.sel >= 0) { const int ch = pgCardH(p), cy0 = Y1 - ch; char ln[4][64];
    for (int y = cy0; y < Y1; y++) for (int x = X0; x < X1; x++) suiPx(b, x, y, suiMix(SUI_RGB(236, 240, 248), SUI_RGB(196, 206, 226), y - cy0, ch));
    for (int x = X0; x < X1; x++) suiPx(b, x, cy0, SUI_RGB(30, 48, 96));
    T.draw(b, X0 + 4, cy0 + 3, P3DS_PgName[p.sel][0] ? P3DS_PgName[p.sel] : "?", SUI_RGB(30, 48, 96), SUI_RGB(236, 240, 248));
    const uint32_t sh = SUI_RGB(236, 240, 248), fg = SUI_RGB(56, 64, 88);
    int wl[64], cg = 0; const int wn = pgWild(p.sel, wl, 64, &cg); const bool hasWild = P3DS_PsReady && P3DS_PsStat[p.sel][0];
    if (p.page) { /* wild Pokemon list: three columns, caught ones in green */
      char hd[24]; if (wn) snprintf(hd, sizeof hd, "CAUGHT %d/%d", cg, wn); else snprintf(hd, sizeof hd, "WILD");
      T.draw(b, X1 - 4 - T.width(hd), cy0 + 3, hd, cg >= wn && wn ? SUI_RGB(40, 120, 64) : fg, sh);
      if (!P3DS_PsReady) T.draw(b, X0 + 4, cy0 + 18, "Take a step to count.", fg, sh);
      else if (!hasWild) T.draw(b, X0 + 4, cy0 + 18, "No wild Pokemon here.", fg, sh);
      else if (!wn) { T.draw(b, X0 + 4, cy0 + 18, "Wild list needs hook 0084,", fg, sh); T.draw(b, X0 + 4, cy0 + 28, "not in this build.", fg, sh); }
      else { const int cw = (aw - 8) / 3, rows = (ch - 18) / 10, slots = rows * 3, shown = wn > slots ? slots - 1 : wn;
        for (int i = 0; i < shown; i++) { const int sp = wl[i], cx0 = X0 + 4 + (i / rows) * cw, cy = cy0 + 16 + (i % rows) * 10; const bool c = P3DS_PsCg[p.sel][sp >> 3] >> (sp & 7) & 1;
          char nm[16]; snprintf(nm, sizeof nm, "%s", kP3dsSpecies[sp]); int k = (int)strlen(nm); while (k > 1 && T.width(nm) > cw - 3) nm[--k] = 0;
          T.draw(b, cx0, cy, nm, c ? SUI_RGB(40, 120, 64) : fg, sh); }
        if (shown < wn) { char m[16]; snprintf(m, sizeof m, "+%d more", wn - shown); T.draw(b, X0 + 4 + 2 * cw, cy0 + 16 + (rows - 1) * 10, m, SUI_RGB(120, 128, 150), sh); } } }
    else {
    const char *st = pgVis(p, p.sel) ? "VISITED" : "NOT YET"; T.draw(b, X1 - 4 - T.width(st), cy0 + 3, st, pgVis(p, p.sel) ? SUI_RGB(40, 120, 64) : SUI_RGB(140, 100, 60), sh);
    char pcl[64]; const int pcAvail = aw - 8 - (hasWild ? T.width("Tap card: wild Pokemon") + 6 : 0); const bool hasPc = pcAvail > 30 && pxCardText(T, pcl, sizeof pcl, p.sel, pcAvail);
    int ty = cy0 + 16; int fl = (hasWild || hasPc) ? 3 : 4; if (P3DS_PsReady) { const int k = pgStats(b, T, p, X0 + 4, ty, p.sel, fg, sh, aw / 2); ty += k * 10 + 1; fl -= k; }
    const int n = fl > 0 ? pgWrap(T, P3DS_PgText[p.sel], aw - 8, ln, fl) : 0; for (int i = 0; i < n; i++) T.draw(b, X0 + 4, ty + i * 10, ln[i], fg, sh);
    if (hasWild) { const char *h = "Tap card: wild Pokemon"; T.draw(b, X1 - 4 - T.width(h), Y1 - 11, h, SUI_RGB(40, 88, 208), sh); }
    if (hasPc) T.draw(b, X0 + 4, Y1 - 11, pcl, SUI_RGB(30, 48, 96), sh); } }
  pgDots(b, p, X1, Y0); }

/* ---- run 11: the game's own icons, decoded from the members above (nothing is written anywhere) ---- */
/* LZ10 (BIOS LZ77, first byte 0x10) into a malloc'd buffer; returns 0 if the data is not LZ10 or broken */
static uint8_t *pgLz10(const uint8_t *s, uint32_t n, uint32_t *outN) {
  if (n < 4 || s[0] != 0x10) return 0; const uint32_t len = s[1] | s[2] << 8 | s[3] << 16; if (!len || len > 256 * 1024) return 0;
  uint8_t *d = (uint8_t *)malloc(len); if (!d) return 0; uint32_t i = 4, o = 0;
  while (o < len && i < n) { const uint8_t f = s[i++];
    for (int b = 0; b < 8 && o < len; b++) {
      if (f & (0x80 >> b)) { if (i + 1 >= n) { free(d); return 0; } const int l = (s[i] >> 4) + 3, dist = ((s[i] & 15) << 8 | s[i + 1]) + 1; i += 2;
        if ((uint32_t)dist > o) { free(d); return 0; } for (int k = 0; k < l && o < len; k++, o++) d[o] = d[o - dist]; }
      else { if (i >= n) { free(d); return 0; } d[o++] = s[i++]; } } }
  *outN = len; return d; }
/* member as plain bytes (LZ10 unpacked); caller frees when *own */
static const uint8_t *pgPlain(int key, uint32_t *n, bool *own) {
  *own = false; if (!pgRaw[key]) return 0; if (pgRaw[key][0] == 0x10) { uint8_t *u = pgLz10(pgRaw[key], pgRawSz[key], n); *own = u != 0; return u; }
  *n = pgRawSz[key]; return pgRaw[key]; }
/* palette bank -> 16 colours as 0x00BBGGRR (index 0 = clear) */
static bool pgPal16(int key, int bank, uint32_t *out) {
  const uint8_t *t = pgSect(pgRaw[key], pgRawSz[key], "TTLP"); if (!t) return false; const uint8_t *P = t + 8 + pgU32(t + 20); const uint32_t pn = pgU32(t + 16) / 2;
  if ((uint32_t)(bank * 16 + 16) > pn) return false;
  for (int i = 0; i < 16; i++) { const uint16_t k = pgU16(P + 2 * (bank * 16 + i)); out[i] = SUI_RGB((k & 31) * 255 / 31, (k >> 5 & 31) * 255 / 31, (k >> 10 & 31) * 255 / 31); } return true; }
/* RGBA icon (alpha 0/255), 0x00BBGGRR | A<<24 */
struct SPgIcon { int w, h; uint32_t px[32 * 32]; bool ok; };
/* a 32x32 OBJ in 1D mapping (tiles 0..15 row-major 4x4) */
static bool pgIcon32(SPgIcon &ic, int chrKey, int palKey, int bank) {
  uint32_t n; bool own; const uint8_t *d = pgPlain(chrKey, &n, &own); uint32_t pal[16]; ic.ok = false;
  if (!d) return false; const uint8_t *c = pgSect(d, n, "RAHC");
  if (c && pgPal16(palKey, bank, pal)) { const uint8_t *C = c + 8 + pgU32(c + 28); const uint32_t cn = pgU32(c + 24);
    if (cn >= 16 * 32 && C + 16 * 32 <= d + n) { ic.w = ic.h = 32;
      for (int t = 0; t < 16; t++) for (int y = 0; y < 8; y++) for (int x = 0; x < 8; x++) { const int v = C[t * 32 + y * 4 + x / 2] >> (4 * (x & 1)) & 15;
        ic.px[((t / 4) * 8 + y) * 32 + (t % 4) * 8 + x] = v ? pal[v] | 0xFF000000u : 0; }
      ic.ok = true; } }
  if (own) free((void *)d); return ic.ok; }
/* run 20: one NCER cell with each OAM's own 16-colour bank of a 256-colour palette, drawn into a w x h RGBA box with the
   cell origin at (ox, oy); only opaque pixels are written. Used for the touch menu's Sprint Button (a/0/1/4 70/68/7). */
static bool pgCellBanks(uint32_t *out, int w, int h, int ox, int oy, int chrKey, int cellKey, int palKey, int cell) {
  uint32_t n, en; bool own, eown; const uint8_t *d = pgPlain(chrKey, &n, &own), *e = pgPlain(cellKey, &en, &eown); bool ok = false;
  const uint8_t *c = d ? pgSect(d, n, "RAHC") : 0, *k = e ? pgSect(e, en, "KBEC") : 0;
  if (c && k) {
    const uint8_t *C = c + 8 + pgU32(c + 28); const uint32_t cn = pgU32(c + 24); const int nc = pgU16(k + 8), ext = pgU16(k + 10) == 1, csz = ext ? 16 : 8;
    const uint32_t mapping = pgU32(k + 16); const uint8_t *base = k + 8 + pgU32(k + 12), *oam = base + nc * csz;
    static const uint8_t SW[3][4] = { { 8, 16, 32, 64 }, { 16, 32, 32, 64 }, { 8, 8, 16, 32 } }, SH[3][4] = { { 8, 16, 32, 64 }, { 8, 8, 16, 32 }, { 16, 32, 32, 64 } };
    if (cell < nc && oam < e + en) { ok = true;
      const int no = pgU16(base + cell * csz); const uint32_t oo = pgU32(base + cell * csz + 4);
      for (int q = 0; q < no && ok; q++) { const uint8_t *a = oam + oo + q * 6; if (a + 6 > e + en) break;
        const uint16_t a0 = pgU16(a), a1 = pgU16(a + 2), a2 = pgU16(a + 4); int y = a0 & 255; if (y > 127) y -= 256; int x = a1 & 511; if (x > 255) x -= 512;
        const int sh = a0 >> 14, sz = a1 >> 14; if (sh > 2) continue; const int ow = SW[sh][sz], oh = SH[sh][sz], hf = a1 >> 12 & 1, vf = a1 >> 13 & 1;
        const int t0 = (a2 & 1023) << (mapping < 16 ? (mapping & 3) : 0); uint32_t pal[16]; if (!pgPal16(palKey, a2 >> 12, pal)) { ok = false; break; }
        for (int ty = 0; ty < oh / 8; ty++) for (int tx = 0; tx < ow / 8; tx++) { const int t = t0 + ty * (ow / 8) + tx; if ((uint32_t)(t * 32 + 32) > cn) continue;
          for (int yy = 0; yy < 8; yy++) for (int xx = 0; xx < 8; xx++) { const int v = C[t * 32 + yy * 4 + xx / 2] >> (4 * (xx & 1)) & 15; if (!v) continue;
            int lx = tx * 8 + xx, ly = ty * 8 + yy; if (hf) lx = ow - 1 - lx; if (vf) ly = oh - 1 - ly; const int px = ox + x + lx, py = oy + y + ly;
            if (px >= 0 && px < w && py >= 0 && py < h) out[py * w + px] = pal[v] | 0xFF000000u; } } } } }
  if (own) free((void *)d); if (eown) free((void *)e); return ok; }
/* run 20: the Sprint Button cropped to our square RUN slot (31x32: cell x 0..30, y 3..34), off = cells 4+12, on = 5+8.
   Recolours (user-approved mockup): the tab's dark-grey edge, any dark pixel, and the uncovered corner -> the tab orange. */
static bool pgSprint(uint32_t *out, bool on) {
  enum { W = 31, H = 32 }; memset(out, 0, W * H * 4);
  if (!pgCellBanks(out, W, H, 0, -3, 170, 168, 107, on ? 5 : 4) || !pgCellBanks(out, W, H, 0, -3, 170, 168, 107, on ? 8 : 12)) return false;
  const uint32_t OR = SUI_RGB(184, 104, 16) | 0xFF000000u;
  for (int i = 0; i < W * H; i++) { const uint32_t c = out[i]; /* DS 5-bit -> 8-bit as c << 3 (184,104,16 is the tab orange) */
    const int r = ((c & 255) * 31 + 127) / 255 << 3, g = ((c >> 8 & 255) * 31 + 127) / 255 << 3, b = ((c >> 16 & 255) * 31 + 127) / 255 << 3; out[i] = SUI_RGB(r, g, b) | (c & 0xFF000000u);
    if (!(c >> 24) || r + g + b < 140 || (r == 72 && g == 72 && b == 72)) out[i] = OR; }
  return true; }
/* one NCER cell (pgmap player head) into a 32x32 box centred on the cell origin */
static bool pgCell(SPgIcon &ic, int chrKey, int cellKey, int palKey, int cell) {
  uint32_t n, en; bool own, eown; const uint8_t *d = pgPlain(chrKey, &n, &own), *e = pgPlain(cellKey, &en, &eown); uint32_t pal[16]; ic.ok = false;
  const uint8_t *c = d ? pgSect(d, n, "RAHC") : 0, *k = e ? pgSect(e, en, "KBEC") : 0;
  if (c && k && pgPal16(palKey, 0, pal)) {
    const uint8_t *C = c + 8 + pgU32(c + 28); const uint32_t cn = pgU32(c + 24); const int nc = pgU16(k + 8), ext = pgU16(k + 10) == 1, csz = ext ? 16 : 8;
    const uint32_t mapping = pgU32(k + 16); const uint8_t *base = k + 8 + pgU32(k + 12), *oam = base + nc * csz;
    static const uint8_t SW[3][4] = { { 8, 16, 32, 64 }, { 16, 32, 32, 64 }, { 8, 8, 16, 32 } }, SH[3][4] = { { 8, 16, 32, 64 }, { 8, 8, 16, 32 }, { 16, 32, 32, 64 } };
    if (cell < nc && oam < e + en) { ic.w = ic.h = 32; memset(ic.px, 0, sizeof ic.px);
      const int no = pgU16(base + cell * csz); const uint32_t oo = pgU32(base + cell * csz + 4);
      for (int q = 0; q < no; q++) { const uint8_t *a = oam + oo + q * 6; if (a + 6 > e + en) break;
        const uint16_t a0 = pgU16(a), a1 = pgU16(a + 2), a2 = pgU16(a + 4); int y = a0 & 255; if (y > 127) y -= 256; int x = a1 & 511; if (x > 255) x -= 512;
        const int sh = a0 >> 14, sz = a1 >> 14; if (sh > 2) continue; const int w = SW[sh][sz], h = SH[sh][sz]; const int t0 = (a2 & 1023) << (mapping < 16 ? (mapping & 3) : 0);
        for (int ty = 0; ty < h / 8; ty++) for (int tx = 0; tx < w / 8; tx++) { const int t = t0 + ty * (w / 8) + tx; if ((uint32_t)(t * 32 + 32) > cn) continue;
          for (int yy = 0; yy < 8; yy++) for (int xx = 0; xx < 8; xx++) { const int v = C[t * 32 + yy * 4 + xx / 2] >> (4 * (xx & 1)) & 15; const int px = 16 + x + tx * 8 + xx, py = 16 + y + ty * 8 + yy;
            if (v && px >= 0 && px < 32 && py >= 0 && py < 32) ic.px[py * 32 + px] = pal[v] | 0xFF000000u; } } }
      ic.ok = true; } }
  if (own) free((void *)d); if (eown) free((void *)e); return ic.ok; }
