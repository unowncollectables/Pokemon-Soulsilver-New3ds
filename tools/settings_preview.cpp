/* Render the SoulSilver settings app (bottom screen) on a PC, with the font and frame read from your own ROM.
   echo 'extern const char *romPath;' > /tmp/rp.h
   gcc -c -O1 -include /tmp/rp.h -DP3DS_ROMPATH=romPath soulsilver3ds/platform/p3ds_gamefont.c -o /tmp/gf.o
   g++ -O1 -I soulsilver3ds/platform tools/settings_preview.cpp /tmp/gf.o -o /tmp/sp && /tmp/sp path/to/soulsilver.nds out.ppm [tab]
   Writes a 256x384 PPM: top screen box (on grey) over the bottom screen. Nothing from the ROM is stored. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ss3ds_settings_ui.h"
extern "C" { extern const char *romPath; int P3DS_GameFontLoad(void); int P3DS_GameBoxReady(void); int P3DS_GameFontReady(void);
  void P3DS_GameBoxDraw(uint32_t *, int, int, int, int, int, int, int); int P3DS_GameFontWidth(int, const char *);
  int P3DS_GameFontDraw(uint32_t *, int, int, int, int, int, const char *, uint32_t, uint32_t, int, int);
  void PSPNativeMemLog(const char *f, ...) { (void)f; } int P3DS_CfgInt(const char *, int d) { return d; } }
extern "C" { const char *romPath; }
static int dr(uint32_t *b, int x, int y, const char *t, uint32_t fg, uint32_t sh) { return P3DS_GameFontDraw(b, 256, 256, 192, x, y, t, fg, sh, 1, 1); }
static int wd(const char *t) { return P3DS_GameFontWidth(1, t); }
int main(int argc, char **argv) {
  if (argc < 3) return 1;
  romPath = argv[1]; const int tab = argc > 3 ? atoi(argv[3]) : 0;
  printf("font %d frame %d\n", P3DS_GameFontLoad(), P3DS_GameBoxReady()); if (!P3DS_GameFontReady()) return 2;
  static uint32_t top[256 * 192], bot[256 * 192]; const SUiText T = { dr, wd };
  for (int i = 0; i < 256 * 192; i++) top[i] = SUI_RGB(56, 72, 64);
  P3DS_GameBoxDraw(top, 256, 256, 192, 16, 16, 216, 32); dr(top, 18, 16, "Settings  (game paused)", SUI_RGB(88, 88, 80), SUI_RGB(168, 184, 184));
  dr(top, 18, 32, "SoulSilver 3DS ss3ds (preview)", 0x00787878u, 0x00D0D0D0u);
  static const char *const tabs[4] = { "Display", "3D", "Controls", "System" };
  static const SUiCell c0[9] = { { "Screen scale", "1.25X", true, false }, { "Wide view", "On", false, true }, { "Wide view FX", "On", false, false },
    { "Wide menus", "On", false, false }, { "Wide haze", "Off", false, false }, { "Weather in strips", "On", false, false },
    { "Edge fill", "On", false, false }, { "Camera zoom", "0", false, false }, { "FPS counter", "Off", false, false } };
  static const SUiCell c3[4] = { { "Menu music", "On", false, false }, { "Save backups (3)", "On", true, false }, { "Log size limit", "1 MB", false, false }, { "Log detail", "Short", false, false } };
  suiBackground(bot); suiHeader(bot, T, tabs, 4, tab, nullptr);
  if (tab == 3) { suiCells(bot, T, c3, 4); suiNote(bot, T, 4, "Logs: sdmc:/3ds/soulsilver3ds/"); } else { suiCells(bot, T, c0, 9); suiNote(bot, T, 9, "x"); }
  FILE *f = fopen(argv[2], "wb"); fprintf(f, "P6 256 384 255\n");
  for (int s = 0; s < 2; s++) for (int i = 0; i < 256 * 192; i++) { uint32_t c = (s ? bot : top)[i]; unsigned char p[3] = { (unsigned char)c, (unsigned char)(c >> 8), (unsigned char)(c >> 16) }; fwrite(p, 1, 3, f); }
  fclose(f); return 0; }
