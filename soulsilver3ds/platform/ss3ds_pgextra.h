/* ss3ds 2026-10-28 (Nathan: "polish Pokegear map, phone list and radio"): the JOHTO bottom screen's left panel gets three
   tabs on its top bar, MAP / PHONE / RADIO, the three cards of the real Pokegear. Setting `pg_extras` (Settings > Input,
   default ON until tested); OFF = the panel is the plain map exactly as before.
   MAP    unchanged map plus pins on places where a phone contact lives (blue = number registered, amber = wants a rematch)
          and a "Phone:" line on the info card (pgDraw calls pxMapPins / pxCardLine).
   PHONE  the Pokegear's registered numbers (hook 0122 reads the save's phone book, the ROM's tel/pmtel_book.dat places and
          the game's own rematch/gift flags). Drag to scroll with a fling, drag the right edge to scrub, SORT chip cycles
          A-Z / PLACE / REMATCH, tap a row = a real outgoing call (hook 0128 opens the game's Pokegear on the phone app and
          starts the game's own call, scripts and all), tap the MAP chip at the right of a row = jump to it on the map.
   RADIO  tap a station = the Pokegear opens on its radio already tuned to it (hook 0129; needs the Radio Card, the game's gate).
          The stations you can tune where you stand right now (the game's Radio_GetAvailableChannels, published by the same
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
volatile int P3DS_PhReady, P3DS_RadSel, P3DS_RadNat, P3DS_RadGb, P3DS_RadCard, P3DS_PhRing, P3DS_PhCallReq, P3DS_RadTuneReq; /* RadTuneReq: station id + 1 to tune from the RADIO tab (hook 0129), 0 none; PhCallReq: contact id + 1 to call from the PHONE tab (hook 0128), 0 none; RadCard: Radio card registered; PhRing: 0 quiet, 1 ringing (caller unknown), 2+id = contact id ringing (hook 0125) */ volatile uint32_t P3DS_PhTick;
/* published by hook 0167 (field_control.c / field_system.c) while a PARTY or BAG tab is open: party slots {species, level, hp, max hp, held item, status bits, egg}, every bag slot of the eight pockets in the game's pocket order (ITEMS, MEDICINE, BALLS, TM/HM, BERRIES, MAIL, BATTLE, KEY), item names read from the game's own text */
volatile int P3DS_VwOn, P3DS_VwReady; volatile unsigned P3DS_VwTick; int P3DS_VwPartyN; uint16_t P3DS_VwParty[6][7]; uint16_t P3DS_VwBagId[8][165], P3DS_VwBagQty[8][165]; char P3DS_VwItem[540][20];
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
  int pane;                       /* 0 MAP, 1 PHONE, 2 RADIO, 3 PARTY, 4 BAG */
  int bagLX0, bagLX1, bagRX0, bagRX1; /* BAG pocket arrows, set while drawing */
  int bag, bagN, bagList[165]; unsigned vwSeen; int vwWait; bool vwLogged; /* BAG pocket (0..7), its non-empty slots, last data tick, frames waiting for data */
  int sort;                       /* 0 A-Z, 1 PLACE, 2 REMATCH first */
  int ord[75], n; uint32_t tick; bool built;
  float scroll, vel; int lastY; bool scrub, touching;
  int sortX0, sortX1;             /* SORT chip, set while drawing */
  int listY0, listY1, listX1;     /* list geometry, set while drawing */
  int locN[100]; uint8_t locRe[100]; /* contacts per Pokegear location, any wants a rematch */
  int minute;                     /* radio pane redraws when the minute changes */
  bool loggedPhone, loggedRadio;
  int radN, radSt[5], radY0[5], radY1[5], radX0, radX1; /* RADIO pane rows (game station ids and rects), set while drawing */
  int pressId; bool moved;        /* contact under the finger (-1 none) for the pressed highlight */
};
static SPx gPx;
/* pxOn() / pxOnV (the pg_extras setting) live in render3ds.cpp next to the settings rows */
static inline int pxPane() { return pxOn() ? gPx.pane : 0; }

/* game-style button: vertical gradient plus a navy frame; `hot` = pressed look */
static void pxBtn(uint32_t *b, int x0, int y0, int x1, int y1, uint32_t hi, uint32_t lo, uint32_t edge) {
  for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) suiPx(b, x, y, suiMix(hi, lo, y - y0, y1 - y0));
  mmFrame(b, x0, y0, x1, y1, edge); }
/* pane backdrop: the Pokegear's blue, with a navy title strip of height h that shows white text */
static void pxBack(uint32_t *b, int X0, int Y0, int X1, int Y1, int h) {
  for (int y = Y0; y < Y1; y++) for (int x = X0; x < X1; x++) suiPx(b, x, y, suiMix(SUI_RGB(168, 188, 226), SUI_RGB(120, 144, 196), y - Y0, Y1 - Y0));
  for (int y = Y0; y < Y0 + h; y++) for (int x = X0; x < X1; x++) suiPx(b, x, y, suiMix(kBlBlue, kBlNavy, y - Y0, h));
  for (int x = X0; x < X1; x++) suiPx(b, x, Y0 + h, kBlNavy); }
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
static inline bool pxScrolls() { return pxPane() == 1 || pxPane() == 4; } /* panes with a drag-to-scroll list */
static inline int pxMaxScroll() { const int h = gPx.listY1 - gPx.listY0; const int m = (gPx.pane == 4 ? gPx.bagN : gPx.n) * kPxRowH - h; return m > 0 ? m : 0; }

/* ---- tab bar ---- */
enum { kPxNTab = 5 };
static const char *const kPxTabs[kPxNTab] = { "MAP", "PHONE", "RADIO", "PARTY", "BAG" };
static void pxChips(const SUiText &T, int X1, int Y0, int *x0, int *x1, int *y0, int *y1) {
  int right = X1 - 2; for (int i = kPxNTab - 1; i >= 0; i--) { const int w = suiSWidth(T, kPxTabs[i]) + 6; x0[i] = right - w; x1[i] = right; right -= w + 1; }
  *y0 = Y0 + 2; *y1 = Y0 + 14; }
/* location bar: name on the left, the three tabs on the right */
static void pxBar(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, const char *name) {
  int cx0[kPxNTab], cx1[kPxNTab], cy0, cy1; pxChips(T, X1, Y0, cx0, cx1, &cy0, &cy1);
  if (gPx.pane == 0) { char nm[40]; snprintf(nm, sizeof nm, "%s", name); for (int k = (int)strlen(nm); k > 1 && suiSWidth(T, nm) > cx0[0] - 4 - (X0 + 5); ) nm[--k] = 0; suiSDraw(T, b, X0 + 5, Y0 + 2, nm, kBlNavy, kBlSilverHi); }
  for (int i = 0; i < kPxNTab; i++) { const bool on = gPx.pane == i;
    for (int y = cy0; y < cy1; y++) for (int x = cx0[i]; x < cx1[i]; x++) suiPx(b, x, y, on ? kBlNavy : suiMix(SUI_RGB(250, 252, 255), kBlSilverLo, y - cy0, cy1 - cy0));
    mmFrame(b, cx0[i], cy0, cx1[i], cy1, kBlNavy);
    suiSDraw(T, b, cx0[i] + 3, cy0, kPxTabs[i], on ? SUI_RGB(255, 255, 255) : kBlNavy, on ? kBlNavy : kBlSilverHi); } }
/* tab index under bar x, -1 none (the chips are measured with the same font as the drawing) */
static int pxChipHit(const SUiText &T, int X1, int Y0, int x) {
  int cx0[kPxNTab], cx1[kPxNTab], cy0, cy1; pxChips(T, X1, Y0, cx0, cx1, &cy0, &cy1);
  for (int i = 0; i < kPxNTab; i++) if (x >= cx0[i] - 1 && x < cx1[i] + 1) return i; return -1; }

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

static inline int pxMapChipW(const SUiText &T) { return T.width("MAP") + 7; }
/* ---- the PHONE pane ---- */
static void pxPhone(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, int Y1) {
  const uint32_t ink = SUI_RGB(30, 48, 96), sh = SUI_RGB(236, 240, 248), fg = SUI_RGB(56, 64, 88);
  if (!gPx.built || gPx.tick != P3DS_PhTick) pxBuild();
  const uint32_t wh = SUI_RGB(255, 255, 255);
  pxBack(b, X0, Y0, X1, Y1, kPxSubH - 1);
  char t[48]; snprintf(t, sizeof t, "TAP TO CALL (%d)", gPx.n); T.draw(b, X0 + 4, Y0 + 2, t, wh, kBlNavy);
  static const char *const kSort[3] = { "A-Z", "PLACE", "REMATCH" }; const int sw = T.width(kSort[gPx.sort]) + 8;
  gPx.sortX1 = X1 - 3; gPx.sortX0 = gPx.sortX1 - sw;
  pxBtn(b, gPx.sortX0, Y0 + 1, gPx.sortX1, Y0 + kPxSubH - 2, SUI_RGB(250, 252, 255), kBlSilverLo, kBlNavy); T.draw(b, gPx.sortX0 + 4, Y0 + 1, kSort[gPx.sort], ink, sh);
  gPx.listY0 = Y0 + kPxSubH; gPx.listY1 = Y1; gPx.listX1 = X1;
  if (!P3DS_PhReady) { T.draw(b, X0 + 4, gPx.listY0 + 6, "Contacts load after your", wh, SUI_RGB(40, 64, 120)); T.draw(b, X0 + 4, gPx.listY0 + 16, "first step in the field.", wh, SUI_RGB(40, 64, 120)); return; }
  if (!gPx.n) { T.draw(b, X0 + 4, gPx.listY0 + 6, "No phone numbers yet.", wh, SUI_RGB(40, 64, 120)); T.draw(b, X0 + 4, gPx.listY0 + 16, "Trainers and friends you", wh, SUI_RGB(40, 64, 120)); T.draw(b, X0 + 4, gPx.listY0 + 26, "register show up here.", wh, SUI_RGB(40, 64, 120)); return; }
  const int ms = pxMaxScroll(); if (gPx.scroll > ms) { gPx.scroll = (float)ms; gPx.vel = 0; } if (gPx.scroll < 0) { gPx.scroll = 0; gPx.vel = 0; }
  const int sc = (int)gPx.scroll, nr = (gPx.listY1 - gPx.listY0) / kPxRowH + 2, r0 = sc / kPxRowH, listR = X1 - (ms > 0 ? 13 : 3);
  for (int r = r0; r < r0 + nr && r < gPx.n; r++) { const int id = gPx.ord[r]; const int y0 = gPx.listY0 + r * kPxRowH - sc, y1 = y0 + kPxRowH; if (y1 <= gPx.listY0 || y0 >= gPx.listY1) continue;
    const bool hot = gPx.touching && !gPx.moved && gPx.pressId == id, inside = y0 + 1 >= gPx.listY0 && y1 - 1 <= gPx.listY1;
    if (inside) pxBtn(b, X0 + 2, y0 + 1, listR, y1 - 1, hot ? kBlPress : SUI_RGB(252, 253, 255), hot ? kBlPress : (r & 1) ? SUI_RGB(214, 222, 240) : SUI_RGB(226, 232, 246), kBlNavy);
    else for (int y = y0; y < y1; y++) { if (y < gPx.listY0 || y >= gPx.listY1) continue; for (int x = X0 + 2; x < listR; x++) suiPx(b, x, y, SUI_RGB(226, 232, 246)); }
    const int ty = y0 + 4; if (!inside) continue; /* text rows only when fully inside the list */
    const uint8_t f = P3DS_PhFlag[id]; int bx = X0 + 5;
    if (f & 2) { for (int yy = 0; yy < 10; yy++) for (int xx = 0; xx < 10; xx++) { const int d = (xx - 5) * (xx - 5) + (yy - 5) * (yy - 5); if (d <= 25) suiPx(b, bx + xx, y0 + 3 + yy, d > 16 ? kBlNavy : SUI_RGB(255, 200, 40)); } T.draw(b, bx + 3, y0 + 2, "!", kBlNavy, SUI_RGB(255, 200, 40)); }
    bx += 12;
    if (f & 4) { for (int yy = 0; yy < 10; yy++) for (int xx = 0; xx < 10; xx++) suiPx(b, bx + xx, y0 + 3 + yy, (xx == 0 || yy == 0 || xx == 9 || yy == 9) ? kBlNavy : SUI_RGB(120, 190, 120)); T.draw(b, bx + 2, y0 + 2, "G", kBlNavy, SUI_RGB(120, 190, 120)); }
    const int nx = X0 + 32; T.draw(b, nx, ty - 1, kPxName[id], ink, sh);
    const int loc = P3DS_PhLoc[id]; if (loc >= 0 && loc < 100 && P3DS_PgName[loc][0]) { char pl[24]; const int cw = pxMapChipW(T), room = listR - 4 - cw - 4 - (nx + T.width(kPxName[id]) + 6);
      pxBtn(b, listR - 2 - cw, y0 + 3, listR - 3, y1 - 3, SUI_RGB(250, 252, 255), kBlSilverLo, kBlNavy); T.draw(b, listR - 2 - cw + 3, ty - 1, "MAP", ink, sh); /* MAP chip = jump to the place; the rest of the row calls */
      if (room > 18) { pxFit(T, pl, sizeof pl, P3DS_PgName[loc], room); T.draw(b, listR - 2 - cw - 4 - T.width(pl), ty - 1, pl, fg, sh); } } }
  if (ms > 0) { const int h = gPx.listY1 - gPx.listY0, bh = h * h / (h + ms) < 8 ? 8 : h * h / (h + ms), by = gPx.listY0 + (h - bh) * sc / ms;
    for (int y = gPx.listY0; y < gPx.listY1; y++) for (int x = X1 - 10; x < X1 - 3; x++) suiPx(b, x, y, SUI_RGB(96, 120, 176));
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
  const uint32_t wh = SUI_RGB(255, 255, 255), dk = SUI_RGB(40, 64, 120);
  pxBack(b, X0, Y0, X1, Y1, kPxSubH - 1); gPx.radN = 0; gPx.radX0 = X0 + 2; gPx.radX1 = X1 - 2;
  const time_t now = time(0); struct tm tmv; const struct tm *lt = localtime_r(&now, &tmv); const int hh = lt ? lt->tm_hour : 0, mi = lt ? lt->tm_min : 0, wd = lt ? lt->tm_wday : 0;
  static const char *const kDay[7] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" }; char clk[24]; snprintf(clk, sizeof clk, "%s %02d:%02d", kDay[wd % 7], hh, mi);
  const int sel = P3DS_RadSel;
  static const char *const kArea[7] = { "JOHTO RADIO", "KANTO RADIO", "KANTO + EXPN CARD", "NO SIGNAL HERE", "RUINS OF ALPH", "ROCKET TAKEOVER", "MAHOGANY SIGNAL" };
  T.draw(b, X0 + 4, Y0 + 2, kArea[sel < 0 || sel > 6 ? 0 : sel], wh, kBlNavy); T.draw(b, X1 - 4 - T.width(clk), Y0 + 2, clk, SUI_RGB(200, 214, 244), kBlNavy);
  if (!P3DS_PhReady) { T.draw(b, X0 + 4, Y0 + 22, "Radio info loads after your", wh, dk); T.draw(b, X0 + 4, Y0 + 32, "first step in the field.", wh, dk); return; }
  if (!P3DS_RadCard) { T.draw(b, X0 + 4, Y0 + 22, "No Radio Card yet.", wh, dk); T.draw(b, X0 + 4, Y0 + 32, "The Pokegear radio unlocks", wh, dk); T.draw(b, X0 + 4, Y0 + 42, "when you get it.", wh, dk); return; }
  int st[5], n = 0;
  switch (sel) { case 0: st[n++] = 0; st[n++] = 1; st[n++] = 2; st[n++] = 3; break; case 1: st[n++] = 0; st[n++] = 1; st[n++] = 2; break;
  case 2: st[n++] = 0; st[n++] = 1; st[n++] = 2; st[n++] = 3; st[n++] = 4; break; case 4: st[n++] = 5; break; case 5: st[n++] = 6; break; case 6: st[n++] = 7; break; default: break; }
  if (!n) { T.draw(b, X0 + 4, Y0 + 22, "The Pokegear gets no radio", wh, dk); T.draw(b, X0 + 4, Y0 + 32, "signal in this place.", wh, dk); return; }
  for (int i = 0; i < n; i++) { const int y0 = Y0 + kPxSubH + 2 + i * 28, y1 = y0 + 26; if (y1 > Y1) break; const char *what, *sub; int until; pxOnAir(st[i], hh, wd, &what, &sub, &until);
    gPx.radSt[i] = st[i]; gPx.radY0[i] = y0; gPx.radY1[i] = y1; gPx.radN = i + 1;
    pxBtn(b, X0 + 2, y0, X1 - 2, y1, SUI_RGB(252, 253, 255), SUI_RGB(214, 222, 240), kBlNavy);
    for (int y = y0 + 1; y < y1 - 1; y++) for (int x = X0 + 3; x < X0 + 5; x++) suiPx(b, x, y, kBlBlue); /* accent stripe */
    /* the "on air" lamp */
    for (int yy = 0; yy < 7; yy++) for (int xx = 0; xx < 7; xx++) { const int d = (xx - 3) * (xx - 3) + (yy - 3) * (yy - 3); if (d <= 10) suiPx(b, X0 + 7 + xx, y0 + 4 + yy, d > 6 ? SUI_RGB(30, 120, 60) : SUI_RGB(90, 220, 120)); }
    char nm[40]; pxFit(T, nm, sizeof nm, what, X1 - X0 - 24 - 6); T.draw(b, X0 + 18, y0 + 2, nm, ink, sh);
    char ln[64]; if (sub[0]) snprintf(ln, sizeof ln, "Today: %s", sub); else if (until >= 0) snprintf(ln, sizeof ln, "Changes at %02d:00", until); else snprintf(ln, sizeof ln, "On the air all day");
    T.draw(b, X0 + 18, y0 + 13, ln, fg, sh); } }

/* ---- the PARTY and BAG panes (read-only viewer, hook 0167): what the game's own Pokemon and Bag screens list, drawn here while you keep walking ---- */
/* the game's item name with its accent restored ("Pok Ball" -> "Poke Ball"; the text pipe drops the e-acute) */
static void pxItemName(int id, char *out, int sz) {
  const char *n = id > 0 && id < 540 && P3DS_VwItem[id][0] ? P3DS_VwItem[id] : "?"; int o = 0;
  for (int i = 0; n[i] && o < sz - 2; i++) { out[o++] = n[i]; if (i >= 2 && n[i - 2] == 'P' && n[i - 1] == 'o' && n[i] == 'k' && n[i + 1] == ' ' && (i == 2 || n[i - 3] == ' ')) out[o++] = 'e'; }
  out[o] = 0; }
static const char *const kVwPocket[8] = { "ITEMS", "MEDICINE", "POKE BALLS", "TMs & HMs", "BERRIES", "MAIL", "BATTLE ITEMS", "KEY ITEMS" };
static void pxVwWait(uint32_t *b, const SUiText &T, int X0, int Y) {
  const uint32_t wh = SUI_RGB(255, 255, 255), dk = SUI_RGB(40, 64, 120);
  if (gPx.vwWait < 90) { T.draw(b, X0 + 4, Y, "Reading your save...", wh, dk); return; }
  T.draw(b, X0 + 4, Y, "No data from the game yet.", wh, dk); T.draw(b, X0 + 4, Y + 10, "Hook 0167 is missing from", wh, dk); T.draw(b, X0 + 4, Y + 20, "this build, or the field", wh, dk); T.draw(b, X0 + 4, Y + 30, "is not running.", wh, dk); }
static void pxParty(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, int Y1) {
  const uint32_t ink = SUI_RGB(30, 48, 96), sh = SUI_RGB(236, 240, 248), fg = SUI_RGB(56, 64, 88), wh = SUI_RGB(255, 255, 255);
  pxBack(b, X0, Y0, X1, Y1, kPxSubH - 1);
  T.draw(b, X0 + 4, Y0 + 2, "PARTY", wh, kBlNavy); const char *vo = "VIEW ONLY"; T.draw(b, X1 - 4 - T.width(vo), Y0 + 2, vo, SUI_RGB(200, 214, 244), kBlNavy);
  if (!P3DS_VwReady) { pxVwWait(b, T, X0, Y0 + kPxSubH + 6); return; }
  if (!P3DS_VwPartyN) { T.draw(b, X0 + 4, Y0 + kPxSubH + 6, "No Pokemon in the party.", wh, SUI_RGB(40, 64, 120)); return; }
  for (int i = 0; i < P3DS_VwPartyN && i < 6; i++) { const uint16_t *m = P3DS_VwParty[i]; const int y0 = Y0 + kPxSubH + 1 + i * 25, y1 = y0 + 24; if (y1 > Y1) break;
    pxBtn(b, X0 + 2, y0, X1 - 2, y1, SUI_RGB(252, 253, 255), (i & 1) ? SUI_RGB(214, 222, 240) : SUI_RGB(226, 232, 246), kBlNavy);
    char nm[32]; if (m[6]) snprintf(nm, sizeof nm, "EGG"); else snprintf(nm, sizeof nm, "%s", m[0] > 0 && m[0] < 494 ? kP3dsSpecies[m[0]] : "?");
    T.draw(b, X0 + 5, y0 + 2, nm, ink, sh);
    if (m[6]) { T.draw(b, X0 + 5, y0 + 13, "It has not hatched yet.", fg, sh); continue; }
    char lv[12]; snprintf(lv, sizeof lv, "Lv%d", (int)m[1]); T.draw(b, X1 - 5 - T.width(lv), y0 + 2, lv, ink, sh);
    const int hp = m[2] > m[3] ? m[3] : m[2], mx = m[3] ? m[3] : 1; const int bw = 64, bx = X0 + 5, by = y0 + 15;
    for (int y = by; y < by + 5; y++) for (int x = bx; x < bx + bw; x++) suiPx(b, x, y, SUI_RGB(70, 76, 100));
    const int fw = hp * (bw - 2) / mx; const uint32_t col = hp == 0 ? SUI_RGB(120, 120, 130) : hp * 2 > mx ? SUI_RGB(64, 200, 96) : hp * 5 > mx ? SUI_RGB(240, 200, 48) : SUI_RGB(224, 64, 56);
    for (int y = by + 1; y < by + 4; y++) for (int x = bx + 1; x < bx + 1 + fw; x++) suiPx(b, x, y, col);
    char hs[20]; snprintf(hs, sizeof hs, "%d/%d", (int)m[2], (int)m[3]); T.draw(b, bx + bw + 4, y0 + 12, hs, fg, sh);
    const uint16_t st = m[5]; const char *tag = hp == 0 ? "FNT" : (st & 7) ? "SLP" : (st & 0x88) ? "PSN" : (st & 0x10) ? "BRN" : (st & 0x20) ? "FRZ" : (st & 0x40) ? "PAR" : nullptr;
    int rx = X1 - 5;
    if (tag) { const int w = suiSWidth(T, tag) + 6; pxBtn(b, rx - w, y0 + 13, rx, y0 + 22, hp == 0 ? SUI_RGB(200, 200, 208) : SUI_RGB(250, 224, 150), hp == 0 ? SUI_RGB(150, 150, 160) : SUI_RGB(226, 170, 60), kBlNavy); suiSDraw(T, b, rx - w + 3, y0 + 13, tag, kBlNavy, SUI_RGB(240, 230, 200)); rx -= w + 3; }
    if (m[4]) { char it[24], fit[24]; pxItemName(m[4], it, sizeof it); const int room = rx - (bx + bw + 4 + T.width(hs) + 6); pxFit(T, fit, sizeof fit, it, room); if (room > 20) T.draw(b, rx - T.width(fit), y0 + 12, fit, fg, sh); } } }
static void pxBagBuild() {
  gPx.bagN = 0; for (int i = 0; i < 165; i++) if (P3DS_VwBagId[gPx.bag][i]) gPx.bagList[gPx.bagN++] = i;
  const int ms = pxMaxScroll(); if (gPx.scroll > ms) gPx.scroll = (float)ms; }
static void pxBag(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, int Y1) {
  const uint32_t ink = SUI_RGB(30, 48, 96), sh = SUI_RGB(236, 240, 248), fg = SUI_RGB(56, 64, 88), wh = SUI_RGB(255, 255, 255), dk = SUI_RGB(40, 64, 120);
  pxBack(b, X0, Y0, X1, Y1, kPxSubH - 1);
  gPx.listY0 = Y0 + kPxSubH; gPx.listY1 = Y1; gPx.listX1 = X1;
  pxBagBuild();
  /* < pocket name (count) >: tap the arrows to change pocket */
  gPx.bagLX0 = X0 + 2; gPx.bagLX1 = X0 + 16; gPx.bagRX1 = X1 - 3; gPx.bagRX0 = X1 - 17;
  pxBtn(b, gPx.bagLX0, Y0 + 1, gPx.bagLX1, Y0 + kPxSubH - 2, SUI_RGB(250, 252, 255), kBlSilverLo, kBlNavy); T.draw(b, gPx.bagLX0 + 4, Y0 + 1, "<", ink, sh);
  pxBtn(b, gPx.bagRX0, Y0 + 1, gPx.bagRX1, Y0 + kPxSubH - 2, SUI_RGB(250, 252, 255), kBlSilverLo, kBlNavy); T.draw(b, gPx.bagRX0 + 5, Y0 + 1, ">", ink, sh);
  { char t[40]; snprintf(t, sizeof t, "%s (%d)", kVwPocket[gPx.bag], gPx.bagN); T.draw(b, X0 + (X1 - X0 - T.width(t)) / 2, Y0 + 2, t, wh, kBlNavy); }
  if (!P3DS_VwReady) { pxVwWait(b, T, X0, gPx.listY0 + 6); return; }
  if (!gPx.bagN) { T.draw(b, X0 + 4, gPx.listY0 + 6, "Nothing in this pocket.", wh, dk); return; }
  const int ms = pxMaxScroll(); if (gPx.scroll > ms) { gPx.scroll = (float)ms; gPx.vel = 0; } if (gPx.scroll < 0) { gPx.scroll = 0; gPx.vel = 0; }
  const int sc = (int)gPx.scroll, nr = (gPx.listY1 - gPx.listY0) / kPxRowH + 2, r0 = sc / kPxRowH, listR = X1 - (ms > 0 ? 13 : 3);
  for (int r = r0; r < r0 + nr && r < gPx.bagN; r++) { const int slot = gPx.bagList[r]; const int y0 = gPx.listY0 + r * kPxRowH - sc, y1 = y0 + kPxRowH; if (y1 <= gPx.listY0 || y0 >= gPx.listY1) continue;
    const bool inside = y0 + 1 >= gPx.listY0 && y1 - 1 <= gPx.listY1;
    if (inside) pxBtn(b, X0 + 2, y0 + 1, listR, y1 - 1, SUI_RGB(252, 253, 255), (r & 1) ? SUI_RGB(214, 222, 240) : SUI_RGB(226, 232, 246), kBlNavy);
    else for (int y = y0; y < y1; y++) { if (y < gPx.listY0 || y >= gPx.listY1) continue; for (int x = X0 + 2; x < listR; x++) suiPx(b, x, y, SUI_RGB(226, 232, 246)); }
    if (!inside) continue;
    const int id = P3DS_VwBagId[gPx.bag][slot], q = P3DS_VwBagQty[gPx.bag][slot]; char it[24], fit[24], qs[12] = ""; pxItemName(id, it, sizeof it);
    if (gPx.bag != 7 && !(gPx.bag == 3 && q == 1)) snprintf(qs, sizeof qs, "x%d", q);
    const int qw = qs[0] ? T.width(qs) + 4 : 0; pxFit(T, fit, sizeof fit, it, listR - 4 - (X0 + 6) - qw);
    T.draw(b, X0 + 6, y0 + 3, fit, ink, sh); if (qw) T.draw(b, listR - 4 - T.width(qs), y0 + 3, qs, fg, sh); }
  if (ms > 0) { const int h = gPx.listY1 - gPx.listY0, bh = h * h / (h + ms) < 8 ? 8 : h * h / (h + ms), by = gPx.listY0 + (h - bh) * sc / ms;
    for (int y = gPx.listY0; y < gPx.listY1; y++) for (int x = X1 - 10; x < X1 - 3; x++) suiPx(b, x, y, SUI_RGB(96, 120, 176));
    for (int y = by; y < by + bh; y++) for (int x = X1 - 10; x < X1 - 3; x++) suiPx(b, x, y, (gPx.scrub || gPx.touching) ? SUI_RGB(64, 112, 200) : SUI_RGB(104, 120, 168)); } }

/* the pane under the tab bar; false = draw the map */
static bool pxPaneDraw(uint32_t *b, const SUiText &T, int X0, int Y0, int X1, int Y1) {
  const int p = pxPane(); if (!p) return false;
  if (p == 1) { pxPhone(b, T, X0, Y0, X1, Y1); if (!gPx.loggedPhone) { gPx.loggedPhone = true; PSPNativeMemLog("[PHONE] phone pane open: %d contacts%s", gPx.n, P3DS_PhReady ? "" : " (no data from the game yet: hook 0122 not in this build, or no field step yet)"); } }
  else if (p == 3 || p == 4) { if (p == 3) pxParty(b, T, X0, Y0, X1, Y1); else pxBag(b, T, X0, Y0, X1, Y1);
    if (!gPx.vwLogged) { gPx.vwLogged = true; PSPNativeMemLog("[VIEW] %s pane open: %s", p == 3 ? "party" : "bag", P3DS_VwReady ? "data already here" : "waiting for hook 0167 data"); } }
  else { pxRadio(b, T, X0, Y0, X1, Y1); if (!gPx.loggedRadio) { gPx.loggedRadio = true; PSPNativeMemLog("[RADIO] radio pane open: area %d, national dex %d, GB Sounds %d%s", (int)P3DS_RadSel, (int)P3DS_RadNat, (int)P3DS_RadGb, P3DS_PhReady ? "" : " (no data from the game yet: hook 0122 not in this build, or no field step yet)"); } }
  return true; }

/* ---- touch ---- */
static void pxSetPane(int p, const char *why) { if (p < 0 || p >= kPxNTab || p == gPx.pane) return; gPx.pane = p; gPx.vel = 0; gPx.scroll = 0; gPx.touching = gPx.scrub = false; gPx.vwWait = 0; BLINV; PSPNativeMemLog("[PHONE] panel -> %s (%s)", kPxTabs[p], why); }
/* every frame: fling after the finger lifts, and the radio's clock */
static void pxFrame(bool touching) {
  if (!pxOn()) return; gPx.touching = touching;
  if (pxScrolls() && !touching && (gPx.vel > 0.5f || gPx.vel < -0.5f)) { gPx.scroll += gPx.vel; gPx.vel *= 0.9f; const int ms = pxMaxScroll(); if (gPx.scroll < 0 || gPx.scroll > ms) { gPx.scroll = gPx.scroll < 0 ? 0.f : (float)ms; gPx.vel = 0; } BLINV; }
  else if (!touching) gPx.vel = 0;
  { const bool vw = gPx.pane == 3 || gPx.pane == 4; P3DS_VwOn = vw ? 1 : 0;
    if (vw) { if (!P3DS_VwReady) gPx.vwWait++; if (P3DS_VwTick != gPx.vwSeen) { gPx.vwSeen = P3DS_VwTick; BLINV; if (gPx.vwWait) { PSPNativeMemLog("[VIEW] data from the game after %d frames: %d party Pokemon", gPx.vwWait, P3DS_VwPartyN); gPx.vwWait = 0; } } } }
  if (gPx.pane == 2) { const time_t now = time(0); const int m = (int)(now / 60); if (m != gPx.minute) { gPx.minute = m; BLINV; } } }
/* finger down / held on the panel (x,y in DS pixels); first = this frame the touch started */
static int pxRowAt(int x, int y);
static void pxDrag(int x, int y, bool first) {
  if (!(gPx.pane == 1 || gPx.pane == 4)) return;
  if (first) { gPx.lastY = y; gPx.vel = 0; gPx.moved = false; gPx.pressId = pxRowAt(x, y); BLINV; gPx.scrub = pxMaxScroll() > 0 && x >= gPx.listX1 - 14 && y >= gPx.listY0; }
  if (gPx.scrub) { const int h = gPx.listY1 - gPx.listY0, ms = pxMaxScroll(); float s = (float)(y - gPx.listY0) * ms / (h > 0 ? h : 1); if (s < 0) s = 0; if (s > ms) s = (float)ms; if (s != gPx.scroll) { gPx.scroll = s; BLINV; } return; }
  const int dy = y - gPx.lastY; if (dy) { gPx.moved = true; gPx.scroll -= dy; gPx.vel = -(float)dy; const int ms = pxMaxScroll(); if (gPx.scroll < 0) gPx.scroll = 0; if (gPx.scroll > ms) gPx.scroll = (float)ms; gPx.lastY = y; BLINV; } }
/* tap on the tab bar */
static void pxChipTap(const SUiText &T, int X1, int Y0, int x) { const int i = pxChipHit(T, X1, Y0, x); if (i >= 0) pxSetPane(i, "tab tapped"); }
/* game station id (0..7) of the RADIO row under (x,y), -1 none */
static int pxRadioAt(int x, int y) { if (gPx.pane != 2) return -1; for (int i = 0; i < gPx.radN; i++) if (x >= gPx.radX0 && x < gPx.radX1 && y >= gPx.radY0[i] && y < gPx.radY1[i]) return gPx.radSt[i]; return -1; }
/* true if x is on the MAP chip at the right of contact id's row (that part jumps to the map, the rest of the row calls) */
static bool pxTapIsPlace(const SUiText &T, int id, int x) {
  const int loc = P3DS_PhLoc[id]; if (loc < 0 || loc >= 100 || !P3DS_PgName[loc][0]) return false;
  const int listR = gPx.listX1 - (pxMaxScroll() > 0 ? 13 : 3); return x >= listR - 2 - pxMapChipW(T) - 3; }
/* contact id under list y (-1 none), and the SORT chip */
static int pxRowAt(int x, int y) { if (gPx.pane != 1 || y < gPx.listY0 || y >= gPx.listY1) return -1; if (pxMaxScroll() > 0 && x >= gPx.listX1 - 14) return -1;
  const int r = (y - gPx.listY0 + (int)gPx.scroll) / kPxRowH; return r >= 0 && r < gPx.n ? gPx.ord[r] : -1; }
/* BAG pane: the < and > chips in the title strip change the pocket */
static bool pxBagTap(int x, int y) {
  if (gPx.pane != 4 || y < gPx.listY0 - kPxSubH || y >= gPx.listY0) return false;
  int d = 0; if (x >= gPx.bagLX0 - 2 && x < gPx.bagLX1 + 2) d = -1; else if (x >= gPx.bagRX0 - 2 && x < gPx.bagRX1 + 2) d = 1; if (!d) return false;
  gPx.bag = (gPx.bag + d + 8) % 8; gPx.scroll = 0; gPx.vel = 0; BLINV; PSPNativeMemLog("[VIEW] bag pocket -> %s", kVwPocket[gPx.bag]); return true; }
static bool pxSortTap(int x, int y) {
  if (gPx.pane != 1 || y < gPx.listY0 - kPxSubH || y >= gPx.listY0 || x < gPx.sortX0 - 2 || x >= gPx.sortX1 + 2) return false;
  gPx.sort = (gPx.sort + 1) % 3; gPx.built = false; gPx.scroll = 0; gPx.vel = 0; BLINV; static const char *const k[3] = { "A-Z", "PLACE", "REMATCH first" };
  PSPNativeMemLog("[PHONE] sort -> %s", k[gPx.sort]); return true; }

/* ---- incoming call banner (hook 0125 publishes whether the Pokegear is ringing). Tapping it = the POKEGEAR button: the game's
   Pokegear opens on the phone and answers (FieldSystem_InitPokegearArgs_Phone). Drawn over any pane, bottom of the panel. ---- */
enum { kPxRingX0 = kBlPanX0 + 2, kPxRingX1 = kBlPanX1 - 2, kPxRingY0 = kBlPanY1 - 21, kPxRingY1 = kBlPanY1 - 2 };
static inline bool pxRinging() { return pxOn() && P3DS_PhRing > 0; }
static bool pxRingHit(int x, int y) { return pxRinging() && x >= kPxRingX0 && x < kPxRingX1 && y >= kPxRingY0 && y < kPxRingY1; }
static void pxRingDraw(uint32_t *b, const SUiText &T, bool blink) {
  if (!pxRinging()) return;
  const uint32_t bg = blink ? SUI_RGB(255, 214, 64) : SUI_RGB(255, 170, 40);
  for (int y = kPxRingY0; y < kPxRingY1; y++) for (int x = kPxRingX0; x < kPxRingX1; x++) suiPx(b, x, y, bg);
  mmFrame(b, kPxRingX0, kPxRingY0, kPxRingX1, kPxRingY1, kBlNavy);
  const int id = P3DS_PhRing - 2; char t[48]; snprintf(t, sizeof t, "CALL: %s", id >= 0 && id < 75 ? kPxName[id] : "?"); char fit[48]; pxFit(T, fit, sizeof fit, t, kPxRingX1 - kPxRingX0 - 60);
  T.draw(b, kPxRingX0 + 4, kPxRingY0 + 4, fit, kBlNavy, bg);
  const int w = T.width("ANSWER") + 8, x1 = kPxRingX1 - 3, x0 = x1 - w;
  pxBtn(b, x0, kPxRingY0 + 3, x1, kPxRingY1 - 3, SUI_RGB(90, 150, 230), kBlNavy, kBlNavy);
  T.draw(b, x0 + 4, kPxRingY0 + 4, "ANSWER", SUI_RGB(255, 255, 255), kBlNavy); }
/* every frame: redraw and log when the ringing state changes */
static void pxRingFrame() {
  static int last; if (!pxOn()) return; const int r = P3DS_PhRing;
  if (r != last) { last = r; BLINV; if (r > 0) PSPNativeMemLog("[PHONE] Pokegear ringing: caller %d '%s' (tap ANSWER or POKEGEAR)", r - 2, r >= 2 && r - 2 < 75 ? kPxName[r - 2] : "?"); else PSPNativeMemLog("[PHONE] ringing stopped"); } }
