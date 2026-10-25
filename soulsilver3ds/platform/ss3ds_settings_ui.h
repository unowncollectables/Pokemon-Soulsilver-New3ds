/* ss3ds: drawing for the SoulSilver settings app (bottom screen, 256x192, 0x00BBGGRR). Pure pixel code with no
   3DS calls, so tools/settings_preview.cpp can render it on a PC. render3ds.cpp's runApp owns input and state.
   Look: SoulSilver's own menus (summary / Pokegear): light steel-blue striped background, navy header bar,
   white panels with a blue-grey rim, the selected panel in a bright blue rim, values in the game's blue. */
#pragma once
#include <stdint.h>
#define SUI_RGB(r, g, b) ((uint32_t)(r) | (uint32_t)(g) << 8 | (uint32_t)(b) << 16)
struct SUiText { int (*draw)(uint32_t *img, int x, int y, const char *t, uint32_t fg, uint32_t sh); int (*width)(const char *t); };
struct SUiCell { const char *name, *value; bool sel, changed; };
static const uint32_t kSuiBgTop = SUI_RGB(208, 224, 248), kSuiBgBot = SUI_RGB(160, 188, 232), kSuiStripe = SUI_RGB(232, 240, 255),
  kSuiHead = SUI_RGB(48, 64, 112), kSuiHeadLine = SUI_RGB(200, 212, 236), kSuiTabTx = SUI_RGB(184, 200, 232), kSuiTabTxSh = SUI_RGB(32, 40, 80),
  kSuiTabOn = SUI_RGB(240, 244, 252), kSuiTabOnEdge = SUI_RGB(120, 144, 200), kSuiTabOnTx = SUI_RGB(40, 48, 88), kSuiTabOnSh = SUI_RGB(176, 188, 216),
  kSuiCell = SUI_RGB(248, 248, 252), kSuiCellLo = SUI_RGB(228, 236, 248), kSuiEdge = SUI_RGB(104, 120, 168),
  kSuiSelEdge = SUI_RGB(56, 128, 232), kSuiSelCell = SUI_RGB(232, 244, 255), kSuiSelLo = SUI_RGB(208, 228, 255),
  kSuiName = SUI_RGB(72, 72, 80), kSuiNameSh = SUI_RGB(208, 208, 216), kSuiVal = SUI_RGB(40, 88, 208), kSuiValSh = SUI_RGB(176, 196, 240),
  kSuiArrow = SUI_RGB(120, 136, 184), kSuiChanged = SUI_RGB(232, 72, 56), kSuiNote = SUI_RGB(112, 120, 144), kSuiNoteSh = SUI_RGB(216, 220, 232);
static inline void suiPx(uint32_t *b, int x, int y, uint32_t c) { if (x >= 0 && x < 256 && y >= 0 && y < 192) b[y * 256 + x] = c; }
static inline uint32_t suiMix(uint32_t a, uint32_t c, int t, int n) { /* a..c, t of n */
  uint32_t o = 0; for (int s = 0; s < 24; s += 8) { int x = (a >> s) & 255, y = (c >> s) & 255; o |= (uint32_t)(x + (y - x) * t / n) << s; } return o; }
/* rounded panel: 1 px rim (2 px when thick), top half `fill`, bottom half shaded towards `lo` like the game's panels */
static void suiPanel(uint32_t *b, int x0, int y0, int x1, int y1, uint32_t fill, uint32_t lo, uint32_t edge, bool thick) {
  const int h = y1 - y0;
  for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) {
    const int dx = x - x0 < x1 - 1 - x ? x - x0 : x1 - 1 - x, dy = y - y0 < y1 - 1 - y ? y - y0 : y1 - 1 - y;
    if (dx + dy < 2) continue; /* cut corners */
    const bool rim = dx == 0 || dy == 0 || (dx + dy == 2) || (thick && (dx == 1 || dy == 1 || dx + dy == 3));
    suiPx(b, x, y, rim ? edge : y - y0 < h / 2 ? fill : suiMix(fill, lo, y - y0 - h / 2, h - h / 2)); } }
static void suiArrow(uint32_t *b, int x, int y, int dir, uint32_t c) {
  for (int k = 0; k < 4; k++) for (int yy = k; yy < 7 - k; yy++) suiPx(b, dir > 0 ? x + k : x + 3 - k, y + yy, c); }
static void suiBackground(uint32_t *b) {
  for (int y = 0; y < 192; y++) { const uint32_t c = suiMix(kSuiBgTop, kSuiBgBot, y, 191);
    for (int x = 0; x < 256; x++) b[y * 256 + x] = ((x + y) & 15) < 2 ? suiMix(c, kSuiStripe, 1, 2) : c; } }
/* header bar with the tabs; returns nothing, tab hit-test is x * nt / 256 for y < 22 */
static void suiHeader(uint32_t *b, const SUiText &T, const char *const *tabs, int nt, int tab, const char *tabLabel) {
  for (int y = 0; y < 21; y++) for (int x = 0; x < 256; x++) b[y * 256 + x] = suiMix(kSuiHead, SUI_RGB(32, 44, 80), y, 20);
  for (int x = 0; x < 256; x++) b[21 * 256 + x] = kSuiHeadLine;
  for (int t = 0; t < nt; t++) { const int x0 = t * 256 / nt + 2, x1 = (t + 1) * 256 / nt - 2; const bool on = t == tab;
    const char *nm = on && tabLabel ? tabLabel : tabs[t];
    if (on) suiPanel(b, x0, 2, x1, 20, kSuiTabOn, kSuiCellLo, kSuiTabOnEdge, false);
    T.draw(b, (x0 + x1) / 2 - T.width(nm) / 2, 2, nm, on ? kSuiTabOnTx : kSuiTabTx, on ? kSuiTabOnSh : kSuiTabTxSh); } }
/* 2 x 5 grid of setting panels (DS y 24..190): name on top, value under it between < > arrows */
static void suiCells(uint32_t *b, const SUiText &T, const SUiCell *c, int n) {
  for (int i = 0; i < n && i < 10; i++) { const int r = i / 2, col = i % 2, x0 = col ? 130 : 4, y0 = 24 + r * 34, x1 = x0 + 122, y1 = y0 + 32;
    const SUiCell &e = c[i];
    suiPanel(b, x0, y0, x1, y1, e.sel ? kSuiSelCell : kSuiCell, e.sel ? kSuiSelLo : kSuiCellLo, e.sel ? kSuiSelEdge : kSuiEdge, e.sel);
    T.draw(b, (x0 + x1) / 2 - T.width(e.name) / 2, y0, e.name, kSuiName, kSuiNameSh);
    T.draw(b, (x0 + x1) / 2 - T.width(e.value) / 2, y0 + 15, e.value, kSuiVal, kSuiValSh);
    suiArrow(b, x0 + 6, y0 + 20, -1, e.sel ? kSuiSelEdge : kSuiArrow); suiArrow(b, x1 - 10, y0 + 20, 1, e.sel ? kSuiSelEdge : kSuiArrow);
    if (e.changed) for (int k = 0; k < 4; k++) for (int q = 0; q < 4; q++) if (k + q > 0 && k + q < 6) suiPx(b, x1 - 8 + q, y0 + 4 + k, kSuiChanged); } }
/* footer hint under a short tab (fewer than 9 cells leaves room) */
static void suiNote(uint32_t *b, const SUiText &T, int n, const char *t) {
  const int rows = (n + 1) / 2, y = 24 + rows * 34 + 4; if (y > 172) return;
  T.draw(b, 128 - T.width(t) / 2, y, t, kSuiNote, kSuiNoteSh); }
