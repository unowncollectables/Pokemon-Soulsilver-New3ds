/* PC preview of the Bottom menu + mini map layout (no ROM, no 3DS): g++ -std=c++17 -I soulsilver3ds/platform tools/minimap_preview.cpp
   -o /tmp/mmprev && /tmp/mmprev out_prefix  -> out_prefix_info.ppm / _world.ppm / _local.ppm (256x192). The text is a crude 6x8
   block font, only good enough to check widths and clipping; on the console the game's own font is used. */
#include "ss3ds_minimap.h"
#include <cstdlib>
static int tdraw(uint32_t *img, int x, int y, const char *t, uint32_t fg, uint32_t sh) { int n = 0; for (; *t; t++, n++) { if (*t != ' ') for (int dy = 1; dy < 8; dy++) for (int dx = 0; dx < 5; dx++) { suiPx(img, x + n * 6 + dx + 1, y + dy + 1, sh); suiPx(img, x + n * 6 + dx, y + dy, fg); } } return n * 6; }
static int twidth(const char *t) { return (int)strlen(t) * 6; }
int main(int argc, char **argv) {
  const SUiText T{tdraw, twidth}; SMm m; const char *pre = argc > 1 ? argv[1] : "mm";
  /* walk a fake route: New Bark -> Route 29 -> Cherrygrove -> Route 30 ... through the matrix, then enter a house */
  srand(3); int x = 20 * 32 + 10, z = 28 * 32 + 5; unsigned tick = 1;
  for (int i = 0; i < 700; i++) { x += (i % 40 < 28) ? 1 : 0; z += (i % 40 >= 28) ? -1 : 0; if (i % 90 == 80) x -= 6;
    const int cx = x / 32, cy = z / 32; mmUpdate(m, 100 + cx, x, z, cx, cy, (cx % 5 == 0) ? 1 : (cy % 7 == 0 ? 3 : 2), 1, (cx % 5 == 0) ? "Cherrygrove City" : "Route 30", tick++); }
  const char *nm[3] = { "info", "world", "local" };
  for (int mode = 0; mode < 3; mode++) {
    static uint32_t b[256 * 192]; suiBackground(b); mmDrawTop(b, T, m, mode, mode & 1);
    const char *L[8] = { "POKEDEX", "POKEMON", "BAG", "POKEGEAR", "TRAINER CARD", "SAVE", "OPTIONS", "MAP: WORLD" }, *A[8] = { "DEX", "PKMN", "BAG", "GEAR", "CARD", "SAVE", "OPTS", "MAP" };
    for (int i = 0; i < 8; i++) bmDrawTile(b, T, i, L[i], A[i], i != 3, i == 5);
    char fn[256]; snprintf(fn, sizeof fn, "%s_%s.ppm", pre, nm[mode]); FILE *f = fopen(fn, "wb"); fprintf(f, "P6\n256 192\n255\n");
    for (int i = 0; i < 256 * 192; i++) { fputc(b[i] & 255, f); fputc((b[i] >> 8) & 255, f); fputc((b[i] >> 16) & 255, f); } fclose(f); }
  /* hit-test sanity: every tile centre hits itself, panel centre hits 8, nothing overlaps or leaves the screen */
  int bad = 0; for (int i = 0; i < 8; i++) { int a, c, d, e; bmRect(i, &a, &c, &d, &e); if (bmHit((a + d) / 2, (c + e) / 2) != i || a < 0 || d > 256 || e > 192) bad++; }
  if (bmHit(128, 60) != 8) bad++; printf("hit-test problems: %d, world cells %d\n", bad, mmCellCount(m)); return bad;
}
