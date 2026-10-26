/* ss3ds run 18: the JOHTO layout's button column drawn at the 3DS's own 320x240 resolution into the native overlay atlas
   (gpuscale.c), so its frames and names are not blurred by the 256x192 -> 320x240 stretch. Style after GabeSud0's
   pokeplatinum-3ds lower menu (3ds_port/nitro/src/port_lowermenu.c, GPL-3.0): 1 px outline with cut corners, a highlight
   row under the top, two shading rows over the bottom, an icon cell + divider + name per button, a coloured tab at the
   right edge, and its small 9-row capital font (glyphs A..X from that file; F J L U W Y - and the space added here).
   Colours are ours (SoulSilver silver and Johto blue). Hit areas are unchanged: every native rect is the DS rect x 1.25. */
#pragma once
#include <stdint.h>
#include <string.h>
struct SBnGlyph { char ch; const char *rows[9]; };
static const SBnGlyph kBnGlyphs[] = {
  { 'A', { ".##.", "#..#", "#..#", "#..#", "####", "#..#", "#..#", "#..#", "#..#" } },
  { 'B', { "###.", "#..#", "#..#", "#..#", "###.", "#..#", "#..#", "#..#", "###." } },
  { 'C', { ".##.", "#..#", "#...", "#...", "#...", "#...", "#...", "#..#", ".##." } },
  { 'D', { "###.", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", "###." } },
  { 'E', { "####", "#...", "#...", "#...", "###.", "#...", "#...", "#...", "####" } },
  { 'F', { "####", "#...", "#...", "#...", "###.", "#...", "#...", "#...", "#..." } },
  { 'G', { ".##.", "#..#", "#...", "#...", "#.##", "#..#", "#..#", "#..#", ".###" } },
  { 'H', { "#..#", "#..#", "#..#", "#..#", "####", "#..#", "#..#", "#..#", "#..#" } },
  { 'I', { "###", ".#.", ".#.", ".#.", ".#.", ".#.", ".#.", ".#.", "###" } },
  { 'J', { "..##", "...#", "...#", "...#", "...#", "...#", "#..#", "#..#", ".##." } },
  { 'K', { "#..#", "#..#", "#.#.", "#.#.", "##..", "#.#.", "#.#.", "#..#", "#..#" } },
  { 'L', { "#...", "#...", "#...", "#...", "#...", "#...", "#...", "#...", "####" } },
  { 'M', { "#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#", "#...#", "#...#" } },
  { 'N', { "#..#", "#..#", "##.#", "##.#", "#.##", "#.##", "#..#", "#..#", "#..#" } },
  { 'O', { ".##.", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", ".##." } },
  { 'P', { "###.", "#..#", "#..#", "#..#", "###.", "#...", "#...", "#...", "#..." } },
  { 'R', { "###.", "#..#", "#..#", "#..#", "###.", "#.#.", "#..#", "#..#", "#..#" } },
  { 'S', { ".##.", "#..#", "#...", "#...", ".##.", "...#", "...#", "#..#", ".##." } },
  { 'T', { "#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.." } },
  { 'U', { "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#", ".##." } },
  { 'V', { "#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", ".#.#.", "..#..", "..#.." } },
  { 'W', { "#...#", "#...#", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", "##.##", "#...#" } },
  { 'X', { "#..#", "#..#", "#..#", ".##.", ".##.", ".##.", "#..#", "#..#", "#..#" } },
  { 'Y', { "#...#", "#...#", ".#.#.", ".#.#.", "..#..", "..#..", "..#..", "..#..", "..#.." } },
  { '-', { "...", "...", "...", "...", "###", "...", "...", "...", "..." } },
  { ' ', { "..", "..", "..", "..", "..", "..", "..", "..", ".." } },
};
static const SBnGlyph *bnGlyph(char c) { if (c >= 'a' && c <= 'z') c -= 32; for (const SBnGlyph &g : kBnGlyphs) if (g.ch == c) return &g; return 0; }
static int bnTextW(const char *s) { int w = 0, n = 0; for (; *s; s++, n++) { const SBnGlyph *g = bnGlyph(*s); w += g ? (int)strlen(g->rows[0]) : 4; } return n ? w + n - 1 : 0; }
/* canvas: the atlas (stride 512, words R<<24|G<<16|B<<8|A), origin at atlas (ox,oy) = native (nx,ny) */
struct SBnCanvas { uint32_t *a; int ox, oy, nx, ny, w, h; };
static inline uint32_t bnRGB(int r, int g, int b) { return (uint32_t)r << 24 | (uint32_t)g << 16 | (uint32_t)b << 8 | 0xFFu; }
static inline void bnPut(const SBnCanvas &c, int x, int y, uint32_t v) { x -= c.nx; y -= c.ny; if (x >= 0 && y >= 0 && x < c.w && y < c.h) c.a[(c.oy + y) * 512 + c.ox + x] = v; }
static void bnRect(const SBnCanvas &c, int x0, int y0, int x1, int y1, uint32_t v) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) bnPut(c, x, y, v); }
static void bnText(const SBnCanvas &c, int x, int y, const char *s, uint32_t v) {
  for (; *s; s++) { const SBnGlyph *g = bnGlyph(*s); if (!g) { x += 5; continue; } const int gw = (int)strlen(g->rows[0]);
    for (int r = 0; r < 9; r++) for (int k = 0; k < gw; k++) if (g->rows[r][k] == '#') bnPut(c, x + k, y + r, v); x += gw + 1; } }
/* the box: outline with its corners left out, fill, highlight row, `shade` rows */
static void bnBox(const SBnCanvas &c, int x0, int y0, int x1, int y1, uint32_t ol, uint32_t fill, uint32_t top, uint32_t shade, int shadeRows) {
  for (int x = x0 + 1; x < x1; x++) { bnPut(c, x, y0, ol); bnPut(c, x, y1, ol); } for (int y = y0 + 1; y < y1; y++) { bnPut(c, x0, y, ol); bnPut(c, x1, y, ol); }
  bnRect(c, x0 + 1, y0 + 1, x1 - 1, y1 - 1, fill); for (int x = x0 + 1; x < x1; x++) bnPut(c, x, y0 + 1, top);
  for (int r = 0; r < shadeRows; r++) for (int x = x0 + 1; x < x1; x++) bnPut(c, x, y1 - 1 - r, shade); }
/* one of our 12x12 icons (ss3ds_minimap.h kBlIcon) at 2x, rows [r0, r1) */
static void bnIcon2x(const SBnCanvas &c, int x, int y, int i, uint32_t ink, uint32_t acc, int r0 = 0, int r1 = 12) {
  for (int r = r0; r < r1; r++) for (int k = 0; k < 12 && kBlIcon[i][r][k]; k++) { const char ch = kBlIcon[i][r][k]; uint32_t v;
    if (ch == '#') v = ink; else if (ch == '+') v = acc; else if (ch == 'o') v = bnRGB(255, 255, 255); else continue;
    for (int d = 0; d < 4; d++) bnPut(c, x + k * 2 + (d & 1), y + (r - r0) * 2 + (d >> 1), v); } }
/* native rect of button/square i (DS rect x 1.25) */
static void bnRectOf(int i, int *x0, int *y0, int *x1, int *y1) { int a, b, cc, d; blRect(i, &a, &b, &cc, &d); *x0 = (a * 5 + 2) / 4; *y0 = (b * 5 + 2) / 4; *x1 = (cc * 5 + 2) / 4 - 1; *y1 = (d * 5 + 2) / 4 - 1; }
/* the whole column into atlas (256,0)..(339,239) = native (236,0)..(319,239); gaps stay transparent. gameIco: bit i =
   the game's icon is drawn on top by the overlay (cell left empty). Returns the native icon cell origin per button. */
static void bnColumn(uint32_t *atlas, const char *const *lb, const char *const *al, const bool *on, int press, unsigned gameIco, int iconX[10], int iconY[10]) {
  const SBnCanvas c = { atlas, 256, 0, 236, 0, 84, 240 };
  for (int y = 0; y < 240; y++) memset(atlas + y * 512 + 256, 0, 84 * 4);
  const uint32_t ol = bnRGB(30, 48, 96), fill = bnRGB(244, 247, 254), top = bnRGB(255, 255, 255), shade = bnRGB(196, 206, 226), cell = bnRGB(228, 234, 246),
    div = bnRGB(164, 176, 196), ink = bnRGB(18, 30, 64), off = bnRGB(150, 160, 176), tab = bnRGB(64, 112, 200), pFill = bnRGB(84, 132, 220), pShade = bnRGB(56, 96, 184),
    pTop = bnRGB(140, 180, 240), pCell = bnRGB(72, 118, 208), onTab = bnRGB(40, 160, 96);
  for (int i = 0; i < 10; i++) { int x0, y0, x1, y1; bnRectOf(i, &x0, &y0, &x1, &y1); const bool p = press == i;
    bnBox(c, x0, y0, x1, y1, ol, p ? pFill : fill, p ? pTop : top, p ? pShade : shade, 2);
    const uint32_t txt = p ? bnRGB(255, 255, 255) : on[i] ? ink : off;
    if (i < 8) {
      bnRect(c, x0 + 1, y0 + 2, x0 + 24, y1 - 3, p ? pCell : cell); for (int y = y0 + 2; y <= y1 - 3; y++) bnPut(c, x0 + 25, y, p ? pShade : div);
      for (int y = y0 + 2; y <= y1 - 3; y++) for (int x = x1 - 3; x < x1; x++) bnPut(c, x, y, p ? bnRGB(255, 255, 255) : tab);
      iconX[i] = x0 + 3; iconY[i] = (y0 + y1 + 1) / 2 - 11;
      if (!((gameIco >> i) & 1)) bnIcon2x(c, x0 + 1, (y0 + y1 + 1) / 2 - 10, i, on[i] ? ink : off, on[i] ? tab : div, 0, 10);
      const char *t = bnTextW(lb[i]) <= x1 - 4 - (x0 + 28) ? lb[i] : al[i]; bnText(c, x0 + 28 + (x1 - 4 - (x0 + 28) - bnTextW(t)) / 2, (y0 + y1 + 1) / 2 - 4, t, txt); }
    else { const int cx = (x0 + x1 + 1) / 2; iconX[i] = cx - 16; iconY[i] = y0 + 3;
      if (i == 9 && on[i]) for (int x = x0 + 2; x <= x1 - 2; x++) { bnPut(c, x, y1 - 3, onTab); bnPut(c, x, y1 - 4, onTab); }
      if (!((gameIco >> i) & 1)) bnIcon2x(c, cx - 12, y0 + 6, i, on[i] || i == 8 ? ink : off, on[i] ? tab : div);
      const char *t = bnTextW(lb[i]) <= x1 - x0 - 4 ? lb[i] : al[i]; bnText(c, cx - bnTextW(t) / 2, y1 - 11, t, txt); } } }
