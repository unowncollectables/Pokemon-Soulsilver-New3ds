/* ss3ds 2026-10-28 (Nathan: "polish Pokegear map, phone list and radio"): the JOHTO bottom screen's left panel gets three
   tabs on its top bar, MAP / PHONE / RADIO, the three cards of the real Pokegear. Setting `pg_extras` (Settings > Input,
   default ON until tested); OFF = the panel is the plain map exactly as before.
   MAP    unchanged map plus pins on places where a phone contact lives (blue = number registered, amber = wants a rematch)
          and a "Phone:" line on the info card (pgDraw calls pxMapPins / pxCardLine).
   PHONE  the Pokegear's registered numbers (hook 0122 reads the save's phone book, the ROM's tel/pmtel_book.dat places and
          the game's own rematch/gift flags). Drag to scroll with a fling, drag the right edge to scrub, SORT chip cycles
          A-Z / PLACE / REMATCH, tap a name = jump to that person's place on the map. Calling is left to the game's own
          Pokegear (POKEGEAR button): a call runs the game's phone scripts, which this screen must not fake.
   RADIO  the stations you can tune where you stand right now (the game's Radio_GetAvailableChannels, published by the same
          hook) with what is on the air this hour: the real rotations from pokegear_radio.c / pokemon_music.c, from the
          3DS clock.
   Read-only: nothing here writes the save. Logs: [PHONE] and [RADIO]. */
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "ss3ds_settings_ui.h"

extern "C" {
/* published by hook 0122 on the game thread (field_control.c) */
volatile int P3DS_PhReady, P3DS_RadSel, P3DS_RadNat, P3DS_RadGb; volatile uint32_t P3DS_PhTick;
uint8_t P3DS_PhFlag[75], P3DS_PhLoc[75]; /* flag bit0 registered, bit1 rematch, bit2 gift; loc = Pokegear location 0..99 or 255 */
}

static const char *const kPxName[75] = {
  "Mom", "Prof. Elm", "Prof. Oak", "Ethan", "Lyra", "Kurt", "Day-Care Man", "Day-Care Lady", "Buena", "Bill",
  "Joey", "Ralph", "Liz", "Wade", "Anthony", "Bike Shop", "Kenji", "Whitney", "Falkner", "Jack",
  "Chad", "Brent", "Todd", "Arnie", "Baoba", "Irwin", "Janine", "Clair", "Erika", "Misty",
  "Blaine", "Blue", "Chuck", "Brock", "Bugsy", "Sabrina", "Lt. Surge", "Morty", "Jasmine", "Pryce",
  "Huey", "Gaven", "Jamie", "Reena", "Vance", "Parry", "Erin", "Beverly", "Jose", "Gina",
  "Alan", "Dana", "Derek", "Tully", "Tiffany", "Wilton", "Krise", "Ian", "Walt", "Alfred",
  "Doug", "Rob", "Kyle", "Kyler", "Tim & Sue", "Kenny", "Tanner", "Josh", "Torin", "Hillary",
  "Billy", "Kay & Tia", "Reese", "Aiden", "Ernest" };

enum { kPxRowH = 17, kPxSubH = 14 };
struct SPx {
  int pane;                       /* 0 MAP, 1 PHONE, 2 RADIO */
  int sort;                       /* 0 A-Z, 1 PLACE, 2 REMATCH first */
  int ord[75], n; uint32_t tick; bool built;
  float scroll, vel; int lastY; bool scrub, touching;
  int sortX0, sortX1;             /* SORT chip, set while drawing */
  int listY0, listY1, listX1;     /* list geometry, set while drawing */
  int locN[100]; uint8_t locRe[100]; /* contacts per Pokegear location, any wants a rematch */
  int minute;                     /* radio pane redraws when the minute changes */
  bool loggedPhone, loggedRadio;
};
static SPx gPx;
/* pxOn() / pxOnV (the pg_extras setting) live in render3ds.cpp next to the settings rows */
static inline int pxPane() { return pxOn() ? gPx.pane : 0; }

static void pxFit(const SUiText &T, char *out, int sz, const char *s, int maxW) {
  snprintf(out, sz, "%s", s); int k = (int)strlen(out);
  while (k > 1 && T.width(out) > maxW) out[--k] = 0; }

/* ---- phone list ---- */
static int pxCmpMode;
static int pxCmp(const void *pa, const void *pb) {
  const int a = *(const int *)pa, b = *(const int *)pb;
  if (pxCmpMode == 2) { const int ra = (P3DS_PhFlag[a] >> 1) & 1, rb = (P3DS_PhFlag[b] >> 1) & 1; if (ra != rb) return rb - ra; }
  if (pxCmpMode == 1) { const int la = P3DS_PhLoc[a], lb = P3DS_PhLoc[b]; if ((la == 255) != (lb == 255)) return la == 255 ? 1 : -1;
    if (la != 255) { const int c = strcmp(P3DS_PgName[la], P3DS_PgName[lb]); if (c) return c; } }
  return strcmp(kPxName[a], kPxName[b]); }
static void pxBuild() {
  gPx.tick = P3DS_PhTick; gPx.n = 0; memset(gPx.locN, 0, sizeof gPx.locN); memset(gPx.locRe, 0, sizeof gPx.locRe);
  for (int i = 0; i < 75; i++) if (P3DS_PhFlag[i] & 1) { gPx.ord[gPx.n++] = i; const int l = P3DS_PhLoc[i];
    if (l >= 0 && l < 100) { gPx.locN[l]++; if (P3DS_PhFlag[i] & 2) gPx.locRe[l] = 1; } }
  pxCmpMode = gPx.sort; qsort(gPx.ord, gPx.n, sizeof(int), pxCmp); gPx.built = true; }
static inline int pxMaxScroll() { const int h = gPx.listY1 - gPx.listY0; const int m = gPx.n * kPxRowH - h; return m > 0 ? m : 0; }

/* ---- tab bar ---- */
static const char *const kPxTabs[3] = { "MAP", "PHONE", "RADIO" };
static void pxChips(const SUiText &T, int X1, int Y0, int *x0, int *x1, int *y0, int *y1) {
  int right = X1 - 2; for (int i = 2; i >= 0; i--) { const int w = T.width(kPxTabs[i]) + 8; x0[i] = right - w; x1[i] = right; right -= w + 2; }
  *y0 = Y0 + 2; *y1 = Y0 + 14; }
/* location bar: name on the left, the three tabs on the right */
static void pxBar(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, const char *name) {
  int cx0[3], cx1[3], cy0, cy1; pxChips(T, X1, Y0, cx0, cx1, &cy0, &cy1);
  if (gPx.pane == 0) { char nm[40]; pxFit(T, nm, sizeof nm, name, cx0[0] - 4 - (X0 + 5)); T.draw(b, X0 + 5, Y0 + 3, nm, kBlNavy, kBlSilverHi); }
  for (int i = 0; i < 3; i++) { const bool on = gPx.pane == i;
    for (int y = cy0; y < cy1; y++) for (int x = cx0[i]; x < cx1[i]; x++) suiPx(b, x, y, on ? kBlNavy : suiMix(SUI_RGB(250, 252, 255), kBlSilverLo, y - cy0, cy1 - cy0));
    mmFrame(b, cx0[i], cy0, cx1[i], cy1, kBlNavy);
    T.draw(b, cx0[i] + 4, cy0 + 1, kPxTabs[i], on ? SUI_RGB(255, 255, 255) : kBlNavy, on ? kBlNavy : kBlSilverHi); } }
/* tab index under bar x, -1 none (the chips are measured with the same font as the drawing) */
static int pxChipHit(const SUiText &T, int X1, int Y0, int x) {
  int cx0[3], cx1[3], cy0, cy1; pxChips(T, X1, Y0, cx0, cx1, &cy0, &cy1);
  for (int i = 0; i < 3; i++) if (x >= cx0[i] - 1 && x < cx1[i] + 1) return i; return -1; }

/* ---- map extras (called from pgDraw) ---- */
static void pxMapPins(uint32_t *b, const SPg &p, int X0, int X1, int oy, int viewX) {
  if (!pxOn() || !P3DS_PhReady) return;
  if (!gPx.built || gPx.tick != P3DS_PhTick) pxBuild();
  for (int l = 0; l < 100; l++) if (gPx.locN[l]) { int x, y, w, h; pgSpecRect(l, &x, &y, &w, &h);
    if (pgHiddenAt(p, x * 8 + w * 4, y * 8 + h * 4)) continue;
    const int px = X0 + x * 8 + w * 8 - viewX - 2, py = oy + y * 8 + 2; if (px < X0 + 3 || px >= X1 - 3) continue;
    const bool re = gPx.locRe[l]; const uint32_t fill = re ? SUI_RGB(255, 200, 40) : SUI_RGB(80, 150, 255);
    for (int yy = -3; yy <= 3; yy++) for (int xx = -3; xx <= 3; xx++) { const int d = abs(xx) + abs(yy); if (d <= 3) suiPx(b, px + xx, py + yy, d == 3 ? kBlNavy : fill); } } }
/* "Phone: Joey, Wade" for the info card of location `loc` into out (empty if none); true if there is something to show */
static bool pxCardText(const SUiText &T, char *out, int sz, int loc, int maxW) {
  out[0] = 0; if (!pxOn() || !P3DS_PhReady || loc < 0 || loc >= 100) return false;
  char t[100] = "Phone: "; int n = 0;
  for (int i = 0; i < 75; i++) if ((P3DS_PhFlag[i] & 1) && P3DS_PhLoc[i] == loc) { const size_t L = strlen(t); if (L + 20 >= sizeof t) { strcat(t, ".."); break; } snprintf(t + L, sizeof t - L, "%s%s%s", n ? ", " : "", kPxName[i], (P3DS_PhFlag[i] & 2) ? "!" : ""); n++; }
  if (!n) return false; pxFit(T, out, sz, t, maxW); return true; }

/* ---- the PHONE pane ---- */
static void pxPhone(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, int Y1) {
  const uint32_t ink = SUI_RGB(30, 48, 96), sh = SUI_RGB(236, 240, 248), fg = SUI_RGB(56, 64, 88);
  if (!gPx.built || gPx.tick != P3DS_PhTick) pxBuild();
  for (int y = Y0; y < Y1; y++) for (int x = X0; x < X1; x++) suiPx(b, x, y, suiMix(SUI_RGB(244, 246, 252), SUI_RGB(206, 214, 232), y - Y0, Y1 - Y0));
  char t[48]; snprintf(t, sizeof t, "CONTACTS %d", gPx.n); T.draw(b, X0 + 4, Y0 + 2, t, ink, sh);
  static const char *const kSort[3] = { "A-Z", "PLACE", "REMATCH" }; const int sw = T.width(kSort[gPx.sort]) + 8;
  gPx.sortX1 = X1 - 3; gPx.sortX0 = gPx.sortX1 - sw;
  for (int y = Y0 + 1; y < Y0 + kPxSubH - 1; y++) for (int x = gPx.sortX0; x < gPx.sortX1; x++) suiPx(b, x, y, suiMix(SUI_RGB(250, 252, 255), kBlSilverLo, y - Y0, kPxSubH));
  mmFrame(b, gPx.sortX0, Y0 + 1, gPx.sortX1, Y0 + kPxSubH - 1, kBlNavy); T.draw(b, gPx.sortX0 + 4, Y0 + 1, kSort[gPx.sort], ink, sh);
  for (int x = X0; x < X1; x++) suiPx(b, x, Y0 + kPxSubH - 1, SUI_RGB(104, 120, 168));
  gPx.listY0 = Y0 + kPxSubH; gPx.listY1 = Y1; gPx.listX1 = X1;
  if (!P3DS_PhReady) { T.draw(b, X0 + 4, gPx.listY0 + 6, "Contacts load after your", fg, sh); T.draw(b, X0 + 4, gPx.listY0 + 16, "first step in the field.", fg, sh); return; }
  if (!gPx.n) { T.draw(b, X0 + 4, gPx.listY0 + 6, "No phone numbers yet.", fg, sh); T.draw(b, X0 + 4, gPx.listY0 + 16, "Trainers and friends you", fg, sh); T.draw(b, X0 + 4, gPx.listY0 + 26, "register show up here.", fg, sh); return; }
  const int ms = pxMaxScroll(); if (gPx.scroll > ms) { gPx.scroll = (float)ms; gPx.vel = 0; } if (gPx.scroll < 0) { gPx.scroll = 0; gPx.vel = 0; }
  const int sc = (int)gPx.scroll, nr = (gPx.listY1 - gPx.listY0) / kPxRowH + 2, r0 = sc / kPxRowH, listR = X1 - (ms > 0 ? 13 : 3);
  for (int r = r0; r < r0 + nr && r < gPx.n; r++) { const int id = gPx.ord[r]; const int y0 = gPx.listY0 + r * kPxRowH - sc, y1 = y0 + kPxRowH; if (y1 <= gPx.listY0 || y0 >= gPx.listY1) continue;
    for (int y = y0; y < y1; y++) { if (y < gPx.listY0 || y >= gPx.listY1) continue; for (int x = X0; x < listR; x++) suiPx(b, x, y, (r & 1) ? suiMix(b[y * 256 + x], SUI_RGB(255, 255, 255), 1, 2) : b[y * 256 + x]); }
    const int ty = y0 + 4; if (ty - 2 < gPx.listY0 || ty + 12 > gPx.listY1) continue; /* text rows only when fully inside the list */
    const uint8_t f = P3DS_PhFlag[id]; int bx = X0 + 3;
    if (f & 2) { for (int yy = 0; yy < 10; yy++) for (int xx = 0; xx < 10; xx++) { const int d = (xx - 5) * (xx - 5) + (yy - 5) * (yy - 5); if (d <= 25) suiPx(b, bx + xx, y0 + 3 + yy, d > 16 ? kBlNavy : SUI_RGB(255, 200, 40)); } T.draw(b, bx + 3, y0 + 2, "!", kBlNavy, SUI_RGB(255, 200, 40)); }
    bx += 12;
    if (f & 4) { for (int yy = 0; yy < 10; yy++) for (int xx = 0; xx < 10; xx++) suiPx(b, bx + xx, y0 + 3 + yy, (xx == 0 || yy == 0 || xx == 9 || yy == 9) ? kBlNavy : SUI_RGB(120, 190, 120)); T.draw(b, bx + 2, y0 + 2, "G", kBlNavy, SUI_RGB(120, 190, 120)); }
    const int nx = X0 + 30; T.draw(b, nx, ty - 1, kPxName[id], ink, sh);
    const int loc = P3DS_PhLoc[id]; if (loc >= 0 && loc < 100 && P3DS_PgName[loc][0]) { char pl[24]; const int room = listR - 4 - (nx + T.width(kPxName[id]) + 6); if (room > 18) { pxFit(T, pl, sizeof pl, P3DS_PgName[loc], room); T.draw(b, listR - 3 - T.width(pl), ty - 1, pl, fg, sh); } } }
  if (ms > 0) { const int h = gPx.listY1 - gPx.listY0, bh = h * h / (h + ms) < 8 ? 8 : h * h / (h + ms), by = gPx.listY0 + (h - bh) * sc / ms;
    for (int y = gPx.listY0; y < gPx.listY1; y++) for (int x = X1 - 10; x < X1 - 3; x++) suiPx(b, x, y, SUI_RGB(214, 220, 236));
    for (int y = by; y < by + bh; y++) for (int x = X1 - 10; x < X1 - 3; x++) suiPx(b, x, y, (gPx.scrub || gPx.touching) ? SUI_RGB(64, 112, 200) : SUI_RGB(104, 120, 168)); } }

/* ---- the RADIO pane: the game's rotations (pokegear_radio.c RadioShow_TranslateStationID, pokemon_music.c) ---- */
static const char *pxMusicToday(int wday) {
  switch (wday) {
  case 0: return P3DS_RadGb ? "GB Sounds" : "Pokemon March";
  case 1: case 5: return "Pokemon March";
  case 2: case 6: return "Pokemon Lullaby";
  case 3: return P3DS_RadNat ? "Hoenn Sound" : "Pokemon March";
  case 4: return P3DS_RadNat ? "Sinnoh Sound" : "Pokemon Lullaby";
  } return "Pokemon March"; }
/* station index s of the game (0..7) -> name on the air at hour h, weekday w; `until` = hour it changes (-1 = all day) */
static void pxOnAir(int s, int h, int w, const char **what, const char **sub, int *until) {
  *until = -1; *sub = "";
  switch (s) {
  case 0: *what = "POKEMON MUSIC"; *sub = pxMusicToday(w); break;
  case 1: *what = "POKEMON TALK"; break;
  case 2: *what = (h % 2) ? "THAT TOWN, THESE PEOPLE" : "TRAINER PROFILES"; *until = (h + 1) % 24; break;
  case 3: { static const char *const k[3] = { "POKEMON SEARCH PARTY", "SERIAL RADIO DRAMA", "BUENA'S PASSWORD" }; *what = k[h % 3]; *until = (h + 1) % 24; break; }
  case 4: *what = "POKE FLUTE"; break;
  case 5: *what = "UNOWN RADIO"; break;
  case 6: *what = "ROCKET RADIO"; break;
  default: *what = "MAHOGANY SIGNAL"; break; } }
static void pxRadio(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, int Y1) {
  const uint32_t ink = SUI_RGB(30, 48, 96), sh = SUI_RGB(236, 240, 248), fg = SUI_RGB(56, 64, 88);
  for (int y = Y0; y < Y1; y++) for (int x = X0; x < X1; x++) suiPx(b, x, y, suiMix(SUI_RGB(244, 246, 252), SUI_RGB(206, 214, 232), y - Y0, Y1 - Y0));
  const time_t now = time(0); struct tm tmv; const struct tm *lt = localtime_r(&now, &tmv); const int hh = lt ? lt->tm_hour : 0, mi = lt ? lt->tm_min : 0, wd = lt ? lt->tm_wday : 0;
  static const char *const kDay[7] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" }; char clk[24]; snprintf(clk, sizeof clk, "%s %02d:%02d", kDay[wd % 7], hh, mi);
  const int sel = P3DS_RadSel;
  static const char *const kArea[7] = { "JOHTO RADIO", "KANTO RADIO", "KANTO + EXPN CARD", "NO SIGNAL HERE", "RUINS OF ALPH", "ROCKET TAKEOVER", "MAHOGANY SIGNAL" };
  T.draw(b, X0 + 4, Y0 + 2, kArea[sel < 0 || sel > 6 ? 0 : sel], ink, sh); T.draw(b, X1 - 4 - T.width(clk), Y0 + 2, clk, fg, sh);
  for (int x = X0; x < X1; x++) suiPx(b, x, Y0 + kPxSubH - 1, SUI_RGB(104, 120, 168));
  if (!P3DS_PhReady) { T.draw(b, X0 + 4, Y0 + 22, "Radio info loads after your", fg, sh); T.draw(b, X0 + 4, Y0 + 32, "first step in the field.", fg, sh); return; }
  int st[5], n = 0;
  switch (sel) { case 0: st[n++] = 0; st[n++] = 1; st[n++] = 2; st[n++] = 3; break; case 1: st[n++] = 0; st[n++] = 1; st[n++] = 2; break;
  case 2: st[n++] = 0; st[n++] = 1; st[n++] = 2; st[n++] = 3; st[n++] = 4; break; case 4: st[n++] = 5; break; case 5: st[n++] = 6; break; case 6: st[n++] = 7; break; default: break; }
  if (!n) { T.draw(b, X0 + 4, Y0 + 22, "The Pokegear gets no radio", fg, sh); T.draw(b, X0 + 4, Y0 + 32, "signal in this place.", fg, sh); return; }
  for (int i = 0; i < n; i++) { const int y0 = Y0 + kPxSubH + 2 + i * 28, y1 = y0 + 26; if (y1 > Y1) break; const char *what, *sub; int until; pxOnAir(st[i], hh, wd, &what, &sub, &until);
    for (int y = y0; y < y1; y++) for (int x = X0 + 2; x < X1 - 2; x++) suiPx(b, x, y, suiMix(SUI_RGB(250, 252, 255), SUI_RGB(222, 230, 246), y - y0, 26));
    mmFrame(b, X0 + 2, y0, X1 - 2, y1, SUI_RGB(104, 120, 168));
    /* the "on air" lamp */
    for (int yy = 0; yy < 7; yy++) for (int xx = 0; xx < 7; xx++) { const int d = (xx - 3) * (xx - 3) + (yy - 3) * (yy - 3); if (d <= 10) suiPx(b, X0 + 7 + xx, y0 + 4 + yy, d > 6 ? SUI_RGB(30, 120, 60) : SUI_RGB(90, 220, 120)); }
    char nm[40]; pxFit(T, nm, sizeof nm, what, X1 - X0 - 24 - 6); T.draw(b, X0 + 18, y0 + 2, nm, ink, sh);
    char ln[64]; if (sub[0]) snprintf(ln, sizeof ln, "Today: %s", sub); else if (until >= 0) snprintf(ln, sizeof ln, "Changes at %02d:00", until); else snprintf(ln, sizeof ln, "On the air all day");
    T.draw(b, X0 + 18, y0 + 13, ln, fg, sh); } }

/* the pane under the tab bar; false = draw the map */
static bool pxPaneDraw(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, int Y1) {
  const int p = pxPane(); if (!p) return false;
  if (p == 1) { pxPhone(b, T, X0, Y0, X1, Y1); if (!gPx.loggedPhone) { gPx.loggedPhone = true; PSPNativeMemLog("[PHONE] phone pane open: %d contacts%s", gPx.n, P3DS_PhReady ? "" : " (no data from the game yet: hook 0122 not in this build, or no field step yet)"); } }
  else { pxRadio(b, T, X0, Y0, X1, Y1); if (!gPx.loggedRadio) { gPx.loggedRadio = true; PSPNativeMemLog("[RADIO] radio pane open: area %d, national dex %d, GB Sounds %d%s", (int)P3DS_RadSel, (int)P3DS_RadNat, (int)P3DS_RadGb, P3DS_PhReady ? "" : " (no data from the game yet: hook 0122 not in this build, or no field step yet)"); } }
  return true; }

/* ---- touch ---- */
static void pxSetPane(int p, const char *why) { if (p < 0 || p > 2 || p == gPx.pane) return; gPx.pane = p; gPx.vel = 0; gPx.touching = gPx.scrub = false; BLINV; PSPNativeMemLog("[PHONE] panel -> %s (%s)", kPxTabs[p], why); }
/* every frame: fling after the finger lifts, and the radio's clock */
static void pxFrame(bool touching) {
  if (!pxOn()) return; gPx.touching = touching;
  if (gPx.pane == 1 && !touching && (gPx.vel > 0.5f || gPx.vel < -0.5f)) { gPx.scroll += gPx.vel; gPx.vel *= 0.9f; const int ms = pxMaxScroll(); if (gPx.scroll < 0 || gPx.scroll > ms) { gPx.scroll = gPx.scroll < 0 ? 0.f : (float)ms; gPx.vel = 0; } BLINV; }
  else if (!touching) gPx.vel = 0;
  if (gPx.pane == 2) { const time_t now = time(0); const int m = (int)(now / 60); if (m != gPx.minute) { gPx.minute = m; BLINV; } } }
/* finger down / held on the panel (x,y in DS pixels); first = this frame the touch started */
static void pxDrag(int x, int y, bool first) {
  if (gPx.pane != 1) return;
  if (first) { gPx.lastY = y; gPx.vel = 0; gPx.scrub = pxMaxScroll() > 0 && x >= gPx.listX1 - 14 && y >= gPx.listY0; }
  if (gPx.scrub) { const int h = gPx.listY1 - gPx.listY0, ms = pxMaxScroll(); float s = (float)(y - gPx.listY0) * ms / (h > 0 ? h : 1); if (s < 0) s = 0; if (s > ms) s = (float)ms; if (s != gPx.scroll) { gPx.scroll = s; BLINV; } return; }
  const int dy = y - gPx.lastY; if (dy) { gPx.scroll -= dy; gPx.vel = -(float)dy; const int ms = pxMaxScroll(); if (gPx.scroll < 0) gPx.scroll = 0; if (gPx.scroll > ms) gPx.scroll = (float)ms; gPx.lastY = y; BLINV; } }
/* tap on the tab bar */
static void pxChipTap(const SUiText &T, int X1, int Y0, int x) { const int i = pxChipHit(T, X1, Y0, x); if (i >= 0) pxSetPane(i, "tab tapped"); }
/* contact id under list y (-1 none), and the SORT chip */
static int pxRowAt(int x, int y) { if (gPx.pane != 1 || y < gPx.listY0 || y >= gPx.listY1) return -1; if (pxMaxScroll() > 0 && x >= gPx.listX1 - 14) return -1;
  const int r = (y - gPx.listY0 + (int)gPx.scroll) / kPxRowH; return r >= 0 && r < gPx.n ? gPx.ord[r] : -1; }
static bool pxSortTap(int x, int y) {
  if (gPx.pane != 1 || y < gPx.listY0 - kPxSubH || y >= gPx.listY0 || x < gPx.sortX0 - 2 || x >= gPx.sortX1 + 2) return false;
  gPx.sort = (gPx.sort + 1) % 3; gPx.built = false; gPx.scroll = 0; gPx.vel = 0; BLINV; static const char *const k[3] = { "A-Z", "PLACE", "REMATCH first" };
  PSPNativeMemLog("[PHONE] sort -> %s", k[gPx.sort]); return true; }
