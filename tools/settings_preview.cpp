/* Render the SoulSilver settings app (bottom screen) on a PC, with the font and frame read from your own ROM.
   echo 'extern const char *romPath;' > /tmp/rp.h
   gcc -c -O1 -include /tmp/rp.h -DP3DS_ROMPATH=romPath soulsilver3ds/platform/p3ds_gamefont.c -o /tmp/gf.o
   g++ -O1 -I soulsilver3ds/platform tools/settings_preview.cpp /tmp/gf.o -o /tmp/sp && /tmp/sp path/to/soulsilver.nds out.ppm [tab [row]]   (rom '-' = block font, no ROM needed, layout check only)
   Writes a 256x384 PPM: top screen box (on grey) over the bottom screen. Nothing from the ROM is stored. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ss3ds_settings_ui.h"
extern "C" { extern const char *romPath; int P3DS_GameFontLoad(void); int P3DS_GameBoxReady(void); int P3DS_GameFontReady(void);
  void P3DS_GameBoxDraw(uint32_t *, int, int, int, int, int, int, int); int P3DS_GameFontWidth(int, const char *);
  int P3DS_GameFontDraw(uint32_t *, int, int, int, int, int, const char *, uint32_t, uint32_t, int, int);
  void PSPNativeMemLog(const char *f, ...) { (void)f; } int P3DS_CfgInt(const char *, int d) { return d; } }
extern "C" { const char *romPath;  /* weak stand-ins so the block-font mode links without p3ds_gamefont.c */
  __attribute__((weak)) int P3DS_GameFontLoad(void) { return 0; } __attribute__((weak)) int P3DS_GameBoxReady(void) { return 0; } __attribute__((weak)) int P3DS_GameFontReady(void) { return 0; }
  __attribute__((weak)) void P3DS_GameBoxDraw(uint32_t *, int, int, int, int, int, int, int) {} __attribute__((weak)) int P3DS_GameFontWidth(int, const char *) { return 0; }
  __attribute__((weak)) int P3DS_GameFontDraw(uint32_t *, int, int, int, int, int, const char *, uint32_t, uint32_t, int, int) { return 0; } }
static bool blockFont;
static int dr(uint32_t *b, int x, int y, const char *t, uint32_t fg, uint32_t sh) {
  if (!blockFont) return P3DS_GameFontDraw(b, 256, 256, 192, x, y, t, fg, sh, 1, 1);
  int n = 0; for (; t[n]; n++) if (t[n] != ' ') for (int yy = 3; yy < 12; yy++) for (int xx = 0; xx < 5; xx++) suiPx(b, x + n * 6 + xx, y + yy, fg); return n * 6; }
static int wd(const char *t) { return blockFont ? (int)strlen(t) * 6 : P3DS_GameFontWidth(1, t); }
int main(int argc, char **argv) {
  if (argc < 3) return 1;
  romPath = argv[1]; const int tab = argc > 3 ? atoi(argv[3]) : 0, selRow = argc > 4 ? atoi(argv[4]) : 0; blockFont = !strcmp(argv[1], "-");
  if (!blockFont) { printf("font %d frame %d\n", P3DS_GameFontLoad(), P3DS_GameBoxReady()); if (!P3DS_GameFontReady()) return 2; }
  static uint32_t top[256 * 192], bot[256 * 192]; const SUiText T = { dr, wd };
  for (int i = 0; i < 256 * 192; i++) top[i] = SUI_RGB(56, 72, 64);
  if (!blockFont) { P3DS_GameBoxDraw(top, 256, 256, 192, 16, 16, 216, 32); dr(top, 18, 16, "Settings  (game paused)", SUI_RGB(88, 88, 80), SUI_RGB(168, 184, 184));
    dr(top, 18, 32, "SoulSilver 3DS ss3ds (preview)", 0x00787878u, 0x00D0D0D0u); }
  static const char *const tabs[8] = { "Display", "Wide", "3D", "Input", "Battle", "Play", "Setup", "Debug" };
  /* sample tabs: a short one and a long one that scrolls (names / tips copied from render3ds.cpp kTabs) */
  static const SUiCell c0[5] = { { "Screen scale", "1.25X", false, false }, { "Wide view", "On", false, true }, { "Camera zoom", "0", false, false },
    { "Bottom menu HD", "On", false, false }, { "FPS counter", "Off", false, false } };
  static const SUiCell c5[10] = { { "Reusable TMs", "Off", false, false }, { "Field moves w/o HM", "On", false, false }, { "Repel prompt", "On", false, false },
    { "No wild battles", "Off", false, false }, { "IV/EV on summary", "On", false, false }, { "Ribbon checklist (X)", "On", false, false },
    { "Special date effects", "On", false, false }, { "Fast text", "2X", false, false }, { "Hold B skips text", "Off", false, false }, { "Auto-advance text", "Off", false, false } };
  static const char *const tips0[5] = { "Size of the picture: 1:1 DS pixels, 1.25X, or fill the whole 3DS screen.", "Shows extra scenery at the sides of the overworld. L+R+X also toggles it.",
    "Overworld camera distance. 0 is the original; minus is closer, plus is farther.", "Draws the Johto touch-menu buttons at full 3DS sharpness.", "Shows the frame rate on screen." };
  static const char *const tips5[10] = { "TMs are not used up when you teach a move.", "Cut, Surf, Strength and the other field moves work without a Pokemon knowing them.", "When a Repel runs out, asks if you want to use another.",
    "Cheat: random wild encounters never happen.", "Press Y on a Pokemon's summary screen to show its IVs and EVs.", "Press X on a summary screen to see which ribbons it has and is missing.",
    "Birthday and holiday sparkles in the overworld.", "Speeds up text boxes: 2X, 4X, or instant. Never skips a prompt.", "Holding B prints text instantly and moves on. Never picks a choice.", "Text boxes move on by themselves after a moment." };
  static SUiCell c[10]; const int n = tab == 5 ? 10 : 5; for (int i = 0; i < n; i++) { c[i] = tab == 5 ? c5[i] : c0[i]; c[i].sel = i == selRow; }
  int topRow = selRow >= kSuiRows ? selRow - kSuiRows + 1 : 0;
  suiBackground(bot); suiHeader(bot, T, tabs, 8, tab, nullptr); suiList(bot, T, c, n, topRow); suiTip(bot, T, (tab == 5 ? tips5 : tips0)[selRow < n ? selRow : 0]);
  FILE *f = fopen(argv[2], "wb"); fprintf(f, "P6 256 384 255\n");
  for (int s = 0; s < 2; s++) for (int i = 0; i < 256 * 192; i++) { uint32_t c = (s ? bot : top)[i]; unsigned char p[3] = { (unsigned char)c, (unsigned char)(c >> 8), (unsigned char)(c >> 16) }; fwrite(p, 1, 3, f); }
  fclose(f); return 0; }
