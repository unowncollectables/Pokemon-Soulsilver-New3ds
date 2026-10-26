/* ss3ds: Bottom menu layout + mini map (bottom screen, 256x192, 0x00BBGGRR). Pure pixel/state code with no 3DS calls, so
   tools/minimap_preview.cpp can render it on a PC. render3ds.cpp owns input, config and logging.

   Layout (everything inside 256x192, 4 px margins):
     y   0..16   header bar: current location name, right side X/Z
     y  18..100  map panel (INFO / WORLD / LOCAL), tap = next mode
     y 104..188  two rows of four tiles: POKEDEX POKEMON BAG POKEGEAR / TRAINER SAVE OPTIONS MAP

   Map data comes from the game thread (hook 0080 in FieldSystem_ProcessStep): map id, mapsec name, player tile x/z, the map
   matrix cell (x/32, z/32 outdoors; the header's town-map cell indoors) and the map type. Nothing is read from the ROM and
   nothing is written to the save. WORLD = every matrix cell the player has stood in, coloured by place type, with the player
   marked; kept across runs in minimap.bin next to config.txt. LOCAL = the path walked on this map, centred on the player. */
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ss3ds_settings_ui.h"

enum { kMmGrid = 64, kMmTrail = 400, kMmModes = 3 };
enum { kMmInfo = 0, kMmWorld = 1, kMmLocal = 2 };
static const int kBmPanelX0 = 4, kBmPanelY0 = 18, kBmPanelX1 = 252, kBmPanelY1 = 100;

struct SMm {
  uint8_t cell[kMmGrid * kMmGrid]; /* 0 = never visited, else the map type (1 city, 2 route, 3 cave, 4/5 inside, 6 underground) */
  int16_t tx[kMmTrail], tz[kMmTrail]; int tn, th; /* trail ring of tiles walked on the current map */
  int map, px, pz, cx, cy, type, onMain; unsigned tick, seen; char name[24];
  bool have, dirty;
  SMm() { memset(this, 0, sizeof *this); map = -1; cx = cy = -1; }
};

/* Called with the game thread's published values when `tick` changed. Returns 1 when a never-visited cell was entered. */
static int mmUpdate(SMm &m, int map, int x, int z, int cx, int cy, int type, int onMain, const char *name, unsigned tick) {
  int fresh = 0;
  const bool mapChanged = map != m.map;
  if (mapChanged) { m.tn = m.th = 0; }
  if (mapChanged || x != m.px || z != m.pz) {
    m.tx[m.th] = (int16_t)x; m.tz[m.th] = (int16_t)z; m.th = (m.th + 1) % kMmTrail; if (m.tn < kMmTrail) m.tn++;
  }
  if (cx >= 0 && cy >= 0 && cx < kMmGrid && cy < kMmGrid && onMain) { /* only the shared outdoor matrix goes in the world map */
    const int t = type < 1 ? 1 : type > 6 ? 6 : type;
    if (!m.cell[cy * kMmGrid + cx]) { fresh = 1; m.dirty = true; }
    m.cell[cy * kMmGrid + cx] = (uint8_t)t; /* 1..6 = map type */
  }
  m.map = map; m.px = x; m.pz = z; m.cx = cx; m.cy = cy; m.type = type; m.onMain = onMain; m.tick = tick; m.have = true;
  snprintf(m.name, sizeof m.name, "%s", name && name[0] ? name : "?");
  return fresh;
}

/* minimap.bin: "MM01" + 64x64 cells. Failure to read/write is harmless (the map just starts empty / is not kept). */
static int mmLoad(SMm &m, const char *path) {
  FILE *f = fopen(path, "rb"); if (!f) return 0; char h[4]; uint8_t buf[kMmGrid * kMmGrid]; int ok = 0;
  if (fread(h, 1, 4, f) == 4 && !memcmp(h, "MM01", 4) && fread(buf, 1, sizeof buf, f) == sizeof buf) { memcpy(m.cell, buf, sizeof buf); ok = 1; }
  fclose(f); return ok; }
static int mmSave(SMm &m, const char *path) {
  FILE *f = fopen(path, "wb"); if (!f) return 0; int ok = fwrite("MM01", 1, 4, f) == 4 && fwrite(m.cell, 1, sizeof m.cell, f) == sizeof m.cell;
  fclose(f); if (ok) m.dirty = false; return ok; }
static int mmCellCount(const SMm &m) { int n = 0; for (int i = 0; i < kMmGrid * kMmGrid; i++) n += m.cell[i] != 0; return n; }

/* ---- tile layout (shared by drawing and hit tests) ---- */
static inline void bmRect(int i, int *x0, int *y0, int *x1, int *y1) {
  *x0 = 4 + (i & 3) * 63; *y0 = 104 + (i >> 2) * 44; *x1 = *x0 + 59; *y1 = *y0 + 40; }
static inline bool bmInPanel(int x, int y) { return x >= kBmPanelX0 && x < kBmPanelX1 && y >= kBmPanelY0 && y < kBmPanelY1; }
/* 0..7 = tile, 8 = map panel, -1 nothing */
static inline int bmHit(int x, int y) {
  for (int i = 0; i < 8; i++) { int a, b, c, d; bmRect(i, &a, &b, &c, &d); if (x >= a && x < c && y >= b && y < d) return i; }
  return bmInPanel(x, y) ? 8 : -1; }

static void mmFill(uint32_t *b, int x0, int y0, int x1, int y1, uint32_t c) { for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) suiPx(b, x, y, c); }
static void mmFrame(uint32_t *b, int x0, int y0, int x1, int y1, uint32_t c) {
  for (int x = x0; x < x1; x++) { suiPx(b, x, y0, c); suiPx(b, x, y1 - 1, c); } for (int y = y0; y < y1; y++) { suiPx(b, x0, y, c); suiPx(b, x1 - 1, y, c); } }
static const char *mmTypeName(int t) { static const char *const n[] = { "?", "TOWN", "ROUTE", "CAVE", "INSIDE", "INSIDE", "UNDERGROUND" }; return n[t < 0 || t > 6 ? 0 : t]; }
static uint32_t mmCellColor(int v) {
  switch (v) { case 1: return SUI_RGB(104, 176, 104); case 2: return SUI_RGB(224, 204, 136); case 3: return SUI_RGB(160, 120, 88);
    case 4: case 5: return SUI_RGB(176, 168, 200); default: return SUI_RGB(144, 152, 176); } }
static const uint32_t kMmSea = SUI_RGB(168, 204, 240), kMmSeaLo = SUI_RGB(148, 188, 232), kMmPlayer = SUI_RGB(232, 48, 40), kMmPlayerIn = SUI_RGB(255, 255, 255);

/* red player marker (blinks between red and white), 5x5 diamond-ish dot centred on (x,y) */
static void mmPlayer(uint32_t *b, int x, int y, bool blink) {
  const uint32_t c = blink ? kMmPlayerIn : kMmPlayer, e = blink ? kMmPlayer : kMmPlayerIn;
  for (int dy = -3; dy <= 3; dy++) for (int dx = -3; dx <= 3; dx++) { const int d = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy); if (d == 4) suiPx(b, x + dx, y + dy, e); else if (d < 4) suiPx(b, x + dx, y + dy, c); } }

/* the whole header + panel; `mode` kMmInfo/kMmWorld/kMmLocal, `blink` toggles the player marker */
static void mmDrawTop(uint32_t *b, const SUiText &T, const SMm &m, int mode, bool blink) {
  /* header bar */
  for (int y = 0; y < 17; y++) { const uint32_t c = suiMix(kSuiHead, SUI_RGB(32, 44, 80), y, 16); for (int x = 0; x < 256; x++) b[y * 256 + x] = c; }
  for (int x = 0; x < 256; x++) b[16 * 256 + x] = kSuiHeadLine;
  char buf[48];
  if (m.have) { T.draw(b, 6, 3, m.name, SUI_RGB(248, 248, 255), kSuiTabTxSh);
    snprintf(buf, sizeof buf, "X%d Z%d", m.px, m.pz); T.draw(b, 250 - T.width(buf), 3, buf, kSuiTabTx, kSuiTabTxSh); }
  else T.draw(b, 6, 3, "LOCATING...", kSuiTabTx, kSuiTabTxSh);
  /* panel */
  suiPanel(b, kBmPanelX0, kBmPanelY0, kBmPanelX1, kBmPanelY1, kSuiCell, kSuiCellLo, kSuiEdge, false);
  const int ix0 = kBmPanelX0 + 2, iy0 = kBmPanelY0 + 2, ix1 = kBmPanelX1 - 2, iy1 = kBmPanelY1 - 2; /* 244 x 78 */
  if (mode == kMmInfo || !m.have) {
    mmFill(b, ix0, iy0, ix1, iy1, kSuiCell);
    if (!m.have) { const char *t = "WALK AROUND TO LOCATE YOU"; T.draw(b, (ix0 + ix1) / 2 - T.width(t) / 2, iy0 + 30, t, kSuiNote, kSuiNoteSh); return; }
    const int w = T.width(m.name); T.draw(b, (ix0 + ix1) / 2 - w / 2, iy0 + 8, m.name, kSuiName, kSuiNameSh);
    snprintf(buf, sizeof buf, "X %d   Z %d", m.px, m.pz); T.draw(b, (ix0 + ix1) / 2 - T.width(buf) / 2, iy0 + 30, buf, kSuiVal, kSuiValSh);
    snprintf(buf, sizeof buf, "%s   MAP %d", mmTypeName(m.type), m.map); T.draw(b, (ix0 + ix1) / 2 - T.width(buf) / 2, iy0 + 50, buf, kSuiNote, kSuiNoteSh);
    return; }
  /* map area: sea-blue with faint stripes, clipped to the panel interior */
  for (int y = iy0; y < iy1; y++) for (int x = ix0; x < ix1; x++) suiPx(b, x, y, ((x + y) & 7) < 1 ? kMmSeaLo : kMmSea);
  const int aw = ix1 - ix0, ah = iy1 - iy0;
  if (mode == kMmWorld) {
    int x0 = 99, y0 = 99, x1 = -1, y1 = -1;
    for (int y = 0; y < kMmGrid; y++) for (int x = 0; x < kMmGrid; x++) if (m.cell[y * kMmGrid + x]) { if (x < x0) x0 = x; if (x > x1) x1 = x; if (y < y0) y0 = y; if (y > y1) y1 = y; }
    const bool pOk = m.cx >= 0 && m.cy >= 0 && m.cx < kMmGrid && m.cy < kMmGrid;
    if (pOk) { if (m.cx < x0) x0 = m.cx; if (m.cx > x1) x1 = m.cx; if (m.cy < y0) y0 = m.cy; if (m.cy > y1) y1 = m.cy; }
    if (x1 < 0) { const char *t = "NOTHING EXPLORED YET"; T.draw(b, (ix0 + ix1) / 2 - T.width(t) / 2, iy0 + 30, t, kSuiNote, kSuiNoteSh); return; }
    /* pad to at least 24 x 10 cells so the first steps do not blow up to one giant cell */
    int w = x1 - x0 + 1, h = y1 - y0 + 1; if (w < 24) { x0 -= (24 - w) / 2; w = 24; } if (h < 10) { y0 -= (10 - h) / 2; h = 10; }
    int s = aw / w; const int sy = ah / h; if (sy < s) s = sy; if (s < 1) s = 1; if (s > 8) s = 8;
    const int ox = ix0 + (aw - w * s) / 2, oy = iy0 + (ah - h * s) / 2;
    for (int y = 0; y < kMmGrid; y++) for (int x = 0; x < kMmGrid; x++) { const int v = m.cell[y * kMmGrid + x]; if (!v) continue;
      const int gx = ox + (x - x0) * s, gy = oy + (y - y0) * s; mmFill(b, gx < ix0 ? ix0 : gx, gy < iy0 ? iy0 : gy, gx + s > ix1 ? ix1 : gx + s, gy + s > iy1 ? iy1 : gy + s, mmCellColor(v));
      if (s >= 3) mmFrame(b, gx, gy, gx + s, gy + s, suiMix(mmCellColor(v), kSuiEdge, 1, 3)); }
    if (pOk) { int px = ox + (m.cx - x0) * s + s / 2, py = oy + (m.cy - y0) * s + s / 2;
      if (m.onMain && s >= 4) { px = ox + (m.cx - x0) * s + (m.px % 32) * s / 32; py = oy + (m.cy - y0) * s + (m.pz % 32) * s / 32; }
      if (px < ix0 + 3) px = ix0 + 3; if (px > ix1 - 4) px = ix1 - 4; if (py < iy0 + 3) py = iy0 + 3; if (py > iy1 - 4) py = iy1 - 4;
      mmPlayer(b, px, py, blink); }
    if (!m.onMain) { const char *t = "INDOORS"; T.draw(b, ix1 - 4 - T.width(t), iy1 - 12, t, kSuiNote, kSuiNoteSh); }
    return; }
  /* LOCAL: 3 px per tile, player fixed in the middle, trail fades from old (pale) to new (strong) */
  const int cxp = (ix0 + ix1) / 2, cyp = (iy0 + iy1) / 2, ts = 3;
  for (int gy = -(ah / 2 / ts / 8 + 1) * 8; gy <= (ah / 2 / ts / 8 + 1) * 8; gy += 8) for (int gx = -(aw / 2 / ts / 8 + 1) * 8; gx <= (aw / 2 / ts / 8 + 1) * 8; gx += 8) { /* 8-tile grid dots, scroll with the player */
    const int sx = cxp + ((gx - ((m.px % 8) + 8) % 8)) * ts, sy = cyp + ((gy - ((m.pz % 8) + 8) % 8)) * ts; if (sx >= ix0 && sx < ix1 && sy >= iy0 && sy < iy1) suiPx(b, sx, sy, SUI_RGB(120, 156, 208)); }
  for (int k = 0; k < m.tn; k++) { const int idx = (m.th - m.tn + k + kMmTrail * 2) % kMmTrail;
    const int sx = cxp + (m.tx[idx] - m.px) * ts - 1, sy = cyp + (m.tz[idx] - m.pz) * ts - 1; if (sx < ix0 || sy < iy0 || sx + ts > ix1 || sy + ts > iy1) continue;
    mmFill(b, sx, sy, sx + ts, sy + ts, suiMix(SUI_RGB(232, 240, 255), SUI_RGB(64, 112, 208), k, m.tn)); }
  mmPlayer(b, cxp, cyp, blink);
  const char *t = "PATH WALKED HERE"; T.draw(b, ix0 + 4, iy1 - 12, t, kSuiNote, kSuiNoteSh); }

/* one tile; label picked so it always fits (`alt` is the short form) */
static void bmDrawTile(uint32_t *b, const SUiText &T, int i, const char *label, const char *alt, bool on, bool pressed) {
  int x0, y0, x1, y1; bmRect(i, &x0, &y0, &x1, &y1);
  suiPanel(b, x0, y0, x1, y1, pressed ? kSuiSelCell : on ? kSuiCell : kSuiCellLo, pressed ? kSuiSelLo : kSuiCellLo, pressed ? kSuiSelEdge : kSuiEdge, pressed);
  const char *t = T.width(label) <= x1 - x0 - 6 ? label : alt;
  T.draw(b, (x0 + x1) / 2 - T.width(t) / 2, (y0 + y1) / 2 - 5, t, on ? kSuiName : kSuiNameSh, kSuiNameSh); }
