SoulSilver 3DS port - settings (sdmc:/3ds/soulsilver3ds/config.txt)

NOTE: this file started as the Platinum 3DS port's settings list. Keys for Platinum-only features (Distortion World dw_*,
Pal/Poketch extras, touch_menu, bottom_strip, cheat_pp, cheat_speed, cheat_walk_walls, cheat_time, item_ping, tod_tint) are NOT implemented in the SoulSilver port
and are ignored; the [CFG] section at the top of log.txt flags them. Keys used by this port are listed in the log header, and the
Gameplay section at the end of this file is SoulSilver-specific. g3_wdepth is dead (replaced by g3_wmode).

================================================================
Put one setting per line, like:   wideview=1
Anything not in the file uses its default. Delete a line to go back to the default.
If something looks or sounds wrong after a change, delete that line (or the whole config.txt).

HOTKEYS (hold L and R, then press the button; the game never sees these presses)
  L+R+SELECT  cycle screen scale: 1:1 / 1.25x / fill (remembered) - happens when you let go of SELECT
  L+R+X       wide view on/off (see more of the overworld at the sides)
  L+R+A       (does nothing: sharp 3D is disabled)
  L+R+START   FPS counter on/off - happens when you let go of START
  L+R+START+SELECT  soft reset, like the original game (the scale/FPS toggles do not fire with it)
  L+R+X+Y     save debug screenshots (BMP files) to the SD card
  L+R+Y       SETTINGS MENU (the game pauses). Up/down = choose, left/right or A = change, B or START = close.
              Changes: FPS counter, wide view, scale, C-stick mode, ZL run, ZR item, circle pad as D-pad,
              wide view effects, log detail, camera zoom, 3D strength, and the speed options below
              (fast upload, fast 3D convert, split top). The list scrolls (10 rows shown). They apply when you
              close the menu and are saved to config.txt.
  A small box at the top of the screen confirms each toggle.

FOLLOWING POKEMON (run 58)
  follower=0        1 = the first Pokemon in your party walks behind you in the overworld, HGSS style (HGSS sprites,
                    all 493 species, shiny colours, forms, female variants). Default 0 (new; turn it on to try it).
                    (Menu: FOLLOWING POKEMON)
                    Needs followers.bin next to the game in sdmc:/3ds/soulsilver3ds/ (made on the box from YOUR
                    SoulSilver ROM by followers/extract.py). Without that file the follower is simply off.
                    It is drawn by the port only: it is not a game object, is never written to your save, has no
                    collision and is invisible to the game's scripts. It hides on the bike, while surfing/using field
                    moves, in the Distortion World, while a story partner walks with you, and after warps / scripted
                    movement until you take your next step. Eggs don't follow.
  follower_behind=1 1 = the follower is always drawn behind you (no more covering your legs when it walks behind
                    you going up). 0 = old run-58 order. Default 1.
  follower_debug=0  1 = draw it two tiles right of the player (alignment test only). Default 0.
  follower_slot=0   which party Pokemon follows: 0 = LEAD (first in the party), 1-6 = that party slot. An empty slot
                    or an egg falls back to the lead. Stored in config.txt only, never in the save. Default 0.
                    (Menu: FOLLOWER SLOT)  [run 61]
  follower_talk=0   1 = press A while standing still and facing your follower: a short text box ("BOOTS IS HAPPY TO
                    BE WITH YOU!", 8 different lines) appears at the bottom of the top screen for ~2.6 s. Drawn by the
                    port over the picture: no game message, no script, nothing saved; it never shows if the game
                    starts something itself (hidden item, sign) and disappears when you move or a script starts.
                    Default 0. (Menu: FOLLOWER TALK)  [run 61]
  follower_sidefix=1  walking left/right the follower now faces the way it walks (run 58-60 showed the side views
                    mirrored the wrong way round). 0 = old look. Default 1.  [run 61]

PICTURE
  cam_zoom=0        overworld camera distance: -2 / -1 closer, 0 original, +1 / +2 farther (-14%..+16%). Zooming out
                    also makes fog areas look foggier. Default 0. (Menu: CAMERA ZOOM, applies at once)
  stereo_strength=0 3D slider depth: 0 = normal, 1 = 1.5x, 2 = 2x deeper (whole 3D world incl. people/Pokemon in the
                    overworld; battle backgrounds follow). Default 0. (Menu: 3D STRENGTH)
  gpuscale=1        screen scale at start: 0 = 1:1, 1 = 1.25x, 2 = fill. Default 1.
  wideview=1        1 = wide view: the top screen uses its full 400 px width (more world at the sides in the
                    overworld, continued backgrounds on the title, menus and battles). 0 = DS 4:3 picture. Default 1.
  wide_cull=1       with wide view: also draw buildings/objects in the side areas. 0 = off. Default 1.
  land_fastload=1   with wide view: load new map blocks in one step (less pop-in at the corners). 0 = off. Default 1.
  wide_fadefix=1    with wide view: the side areas fade to black/white together with the screen on area changes
                    (no early peek at the new area). 0 = off. Default 1. (Only used when wide_fx=0.)
  wide_fx=0         with wide view: the side areas copy the middle picture's brightness, fades, wipes, flashes and
                    weather. SLOW (about 14 ms per frame on hardware) and can copy menu edges into the sides.
                    0 = off (wide_fadefix is used instead). Default 0.
  wide_fill=1       with wide view: screens without real wide 3D (battles, title, bag/party/Pokedex menus) fill the
                    sides with a soft mirrored copy of the picture's edge instead of black bars. Looks poor and can copy menu
                    edges into the sides. 0 = black bars (the run 54 look). Default 0.
  wide_battle=1     (Settings > Display > Wide battles) with wide view and 1.25x: only while a battle runs, the sides continue
                    the battle background (same cheap strip renderer as wide_menus, <1 ms/frame, no Wide FX). Default 0.
  wide_boxes=1      (Settings > Wide > Wide HP boxes, 2026-10-27) the Platinum port's battle layout: with wide view and 1.25x the
                    player's HP box (name, level, HP bar) moves 32 px out to the right edge and the enemy's 32 px left, and
                    the parts that reach into the side strips are drawn there (strips continue the battle background as
                    wide_battle does; works on its own). Applies from the next battle. Needs pspoke patch 0100 (rebuild the
                    game). Log: [WIDE] wide_boxes / HP box offset / HP box overlay. Default 0 until tested.
  wide_menu_screens=1 (Settings > Display > Wide party/bag) with wide view and 1.25x: only the party menu, bag and
                    Pokemon summary (also when opened from a battle) continue their backgrounds into the sides, same cheap
                    strip renderer as wide_battle. Title, intro and other 2D screens stay as they were. Default 0.
  wide_menus=0      (run 65) with wide view and 1.25x: menus, battles, the title and other 2D screens fill the full
                    400 px width by continuing the DS's own background tiles past the screen edges (same 1.25x size as
                    the overworld, not mirrored, not blurred; fades/brightness follow). Sprites are not continued.
                    Costs about 0.3-1 ms per frame (unchanged frames reuse the last strips). 0 = off (wide_fill / black
                    bars as before). Default 0 in SoulSilver: the intro movie, title and summary screens showed
                    cut-off panels in the sides, so everything outside the overworld keeps the DS picture.
  encounter_test=1  (run 65, DEBUG ONLY) every step on any tile starts a wild grass battle. Default 0. Leave off.
  fog3d=1           DS 3D fog (Route 210 etc.; also in the wide view sides). 1 = done by the 3DS GPU's fog unit,
                    practically free. 2 = the run 55 exact method (much slower, about 6-10 ms per frame in fog areas
                    and on the title). 0 = no fog. Default 1.
  fog_hole=1        Dark Cave / Flash light circle (no menu row any more; config.txt only). On a DS, see-through polygons drawn
                    without fog clear the fog under them; the game uses that for the small lit circle around you in
                    dark caves. 1 = fog frames with such polygons use the exact fog=2 method (about 6-10 ms per frame,
                    only there). 0 = old behaviour (dark caves fully black on a 3DS). Default 1.
  stereo_2d=2       3D slider, battles/menus: how far the background layers sit behind the screen (text, menus,
                    HP boxes stay at the screen). 2 = GENTLE (new default: back layer 3 px, middle 1-2 px per eye
                    at full slider). 1 = STRONG (run 55-60: 12 / 9 px - far more depth than the battle Pokemon
                    themselves have, which made the Pokemon look doubled/ghosted against the background).
                    0 = FLAT. 3 = GENTLE+LOCK (default): like 2, and in battles every 3D polygon gets one fixed
                    parallax (0.25 of the shift per eye = the depth stereo_2d gives the ground BG), so the Pokemon quads no longer sit at a different depth/spot in each eye while their
                    z changes during send-outs and attacks. Since run 80 the lock uses the BG's exact integer offset (left eye 0, right eye +(shift+1)/2 px), so the Pokemon no longer drifts half a pixel off its platform at odd slider shifts. Log: [BATTLE] stereo 3D lines. (Menu: BATTLE 3D DEPTH)  [run 61]
  wide_haze=1       with wide view: a 2D haze/fog layer that the game blends over the 3D world (foggy areas) is
                    continued into the side areas (tint measured from the picture's own edges, applied by the GPU;
                    ~0.1 ms). 0 = off. Default 1. (Menu: WIDE HAZE)  [run 61]
  battle_monobj_hide=1  fixes the Pokemon showing twice while a Poke Ball is thrown in battle. 0 = off. Default 1.
  (res3d: ignored since run 52. Use g3_sharp instead.)
  g3_sharp=0        all 3D drawn at 2x and box-downscaled (smooth edges); fog areas stay 1x. Experimental, may cost
                    fps. Default 0. (Menu: 3D > SHARP 3D, applies at once) Run 49: needs more free VRAM, usually stays 1x
                    with the toast SHARP 3D: NO VRAM.
  tex_postvblank=1  3D sprites pick up their new picture in the same frame (fixes the one-frame coloured box
                    when getting on the bike, and late sprite frames). 0 = off. Default 1.
  vram_frame_gen=1  SoulSilver: the 3D texture cache re-checks VRAM every frame, so battle Pokemon show their real
                    picture and colours (fixes the dim / wrong / doubled battle Pokemon). 0 = old (stale). Default 1.
  stereo_depth=1    3D-slider depth for the 3D scene. 0 = flat 3D layer. Default 1.

SPEED (all safe defaults; only change if something looks wrong)
  gpu_swizzle=1     the finished screens go straight to the 3DS GPU (no CPU colour-swap pass per screen). Picture is
                    identical. 0 = old way. Default 1. (Menu: FAST UPLOAD)
  fast_conv=1       faster conversion of the 3D picture for the 2D compositor (same result, bit for bit). 0 = old.
                    Default 1. (Menu: FAST 3D CONVERT)
  split_top=1       3D slider OFF only: the top screen is composed by two CPU cores (top half / bottom half). Biggest
                    gain in battles and menus. 0 = one core (old). Default 1. (Menu: SPLIT TOP (MONO))
  bottom_skip=1     reuse the bottom screen picture when nothing changed (faster). 0 = always redraw. Default 1.
  present_skipbot=1 skip re-uploading an unchanged bottom screen (faster). 0 = off. Default 1.
  bottom_refresh=8  after scene changes, fully redraw the bottom screen for this many frames. 0 = off. Default 8.
  early_b=1         draw the bottom screen while the GPU draws 3D (faster). 0 = off. Default 1.
  split_top=0       1 = split the top-screen drawing over two CPU cores (2D/no-3D-slider only). Experimental,
                    untested on hardware. If the top screen shows a seam or wrong half, set 0. Default 0.
  rom_cache_mb=32   game data kept in RAM (MiB) to shorten loading hitches. 0 = off. Default 32 (uses less
                    automatically if memory is short: 24/16/8...).
  zone_prefetch=1   while you stand/walk around, quietly load the data of the areas you usually go to next, so
                    area changes hitch less. It learns as you play (first visit to an area is not faster) and
                    remembers in sdmc:/3ds/soulsilver3ds/prefetch.bin (safe to delete any time). It pauses while
                    the game loads, while saving and while the log is written. To turn it off: zone_prefetch=0.
                    Default 1.

SAVING
  save_anim=2       2 = our SAVING box replaces the game's save text; 1 = small corner box; 0 = off. Default 2.
  save_seq=2        2 = our box (with our YES/NO) covers the whole save prompt; 1 = without our YES/NO;
                    0 = show the game's own save text. Default 2. (The save itself is always the game's own.)
  save_minms=1500   how long (ms) "SAVING" shows at least. Default 1500.

BUTTONS (New 3DS)
  Circle pad  walks like the D-pad (4 directions, small dead zone).
  ZR          uses your registered key item (same as Y, e.g. the bike).
  ZL          running on/off (with it on you run without holding B; hold B to walk). A box shows RUN ON / RUN OFF.
  C-stick     works like the D-pad (walk, menus). With cstick_mode=1: up/down = Poketch side button instead.
  Soft reset (L+R+START+SELECT) closes the save file cleanly and restarts the game. If your homebrew loader can't
  restart it, the game just closes (nothing is lost; start it again).

CONTROLS / TOUCH
  cpad_dpad=1       circle pad moves like the D-pad. 0 = off. Default 1.
  zr_item=1         ZR = Y (registered item). 0 = off. Default 1.
  auto_run=1        The run toggle starts ON, so you run by default and B walks (ZL / the RUN tile flip it). Default 0.
  sprite_depth=1    Overworld sprites (following Pokemon, NPCs, you) get one 3D depth from their feet so they stand on the ground with the slider up. Default 0, experimental.
  pm_shortcuts=1    In the Pokemon menu: X = Move Reminder, Y = Move Tutor on the highlighted Pokemon. Default 0.
  zl_run=1          ZL toggles running (SoulSilver: swaps what B does). 0 = off. Default 0.
  cstick_mode=2     C-stick: 0 = off, 1 = up/down flick presses the Poketch button (overworld only),
                    2 = C-stick moves like the D-pad (walking and menus, same as the circle pad). Default 2.
                    (the old cstick_poketch line is ignored now.)
  menu_combo=1      L+R+Y opens the settings menu. 0 = off. Default 1.
  fps_counter=0     FPS counter shown at start (the menu saves this; L+R+START still toggles). Default 0 (run 64).
  soft_reset=1      L+R+START+SELECT soft reset. 0 = off (the combo does nothing). Default 1.
  select_reset=1    (ss3ds 0070; Settings > Battle > SELECT = reset) 1 = tap SELECT in a wild battle (once the FIGHT/BAG
                    menu is up) for the same soft reset, for shiny hunting. Never in the overworld, menus or trainer battles,
                    and never when the wild Pokemon is shiny. Default 0. Log tag [BOOT].
  reset_skip_intro=0 (ss3ds 0070; Settings > Battle > Skip intro on reset) 1 = after a soft reset the game skips the logos,
                    intro and title and opens Continue / New Game straight away (no credits splash either). Only right
                    after a soft reset; a normal start is unchanged. Default 0. Log tag [BOOT] ("hook NOT applied" = this
                    build lacks the game hook and the option does nothing).
  hotkey_swallow=1  hide hotkey presses from the game. 0 = off. Default 1.
  touch_latch=1     catch very quick taps on the touch screen. 0 = off. Default 1.

SOUND
  audio_latency=16  sound buffer (lower = less delay, higher = safer against crackles). Default 16.
  audio_lead=8      output buffer; if sound crackles, try 16. Default 8.
  audio_adapt=1     keep sound smooth when the game slows down. 0 = off. Default 1.
  audio_gain=7      volume (Settings > System > Volume: 55..145 = gain 4..10, 100 = 7). Live. Default 7.
  audio_limiter=1   pulls loud moments down instead of clipping them (Settings > Debug > Audio limiter). Live. Default 1.
  audio_stallfade=1 (2026-10-26) when the game stops making sound for over ~0.5 s (long load, crash screen, menu
                    with Menu music off) the held notes fade out instead of droning; they fade back in when the game
                    continues (Settings > Debug > Audio stall fade). Live. Default 1.

LOGGING
  stereo_blobdiag=1 with the 3D slider up, logs [STEREO] blob lines and saves stereo_blob_*.bmp once if one eye
                    shows a black patch the other doesn't (for the black-blob report). 0 = off. Default 1.
  gpu_compose=2     GPU 2D compositor for the TOP screen (engine A). Default 2 since run 61 (was 0; you confirmed on
                    hardware that it runs much better). 0 = off (old CPU path, unchanged).
                    1 = ANALYZE: only writes [GPUC] statistics lines to log.txt.
                    2 = GPU: frames that use only "simple" 2D features (text BGs, 3D layer, plain sprites, no
                        windows/blending/brightness/mosaic) are drawn by the 3DS GPU with a tile cache; every other
                        frame (battles with effects, fades, Cut, ...) uses the old CPU path automatically.
                        Azahar: overworld composite 14 ms -> 5.5 ms, pixel-identical (run 60).
                    3 = VERIFY (debugging): picture comes from the old path, the GPU result is made too and compared
                        pixel by pixel; log.txt gets "[GPUC] verify" lines, the first mismatches are saved as
                        gpuc_mmN_cpu.bmp / gpuc_mmN_gpu.bmp. Slower than 0. (Menu: GPU COMPOSE)
  gpu_blend=0      with gpu_compose=2: also GPU-compose frames that use alpha blend (BLDCNT effect 1) or brightness
                    up/down on a layer (title, intro, menus: ~3/4 of non-field frames use these, so they ran at ~30 fps
                    on the CPU path). Default 0 = off. Frames whose blend can't be reproduced exactly (a layer below a
                    blended layer is not a second target, 3D layer blending, windows, master-brightness fades, semi-
                    transparent sprites) still use the CPU path. To test safely: gpu_compose=3 gpu_blend=1 shows the CPU
                    picture and logs "[GPUC] gpu_blend: ..." plus verify lines ("px off by >4" should be 0; small +-1..2
                    rounding differences are expected).  [run 62]
  gpu_compose_stereo=1  with gpu_compose=2 and the 3D slider up: overworld frames are GPU-composed for BOTH eyes
                    (before run 61 the 3D slider always forced the slow CPU path: ~17 ms, 25 fps). 0 = old.
                    Default 1. (Menu: GPU COMPOSE 3D)  [run 61]
  gpu_compose_stereo_nf=1  same, outside the overworld (battles, menus) with the 3D slider up: both eyes GPU-composed, the right
                    eye's BG layers shifted by priority like stereo_2d. Log 2026-10-26: battle composite was 14-22 ms on the CPU
                    (fps 25-30 with 2D sprites), the title at slider 0 was 4.8 ms. 0 = old CPU path. Default 1.
                    (Debug tab: GPU COMPOSE 3D MENUS; remove once tested)  [run 17]
  gpu_direct3d=0    EXPERIMENTAL (default OFF; Debug tab: GPU 3D DIRECT). The 3D layer stays on the GPU: g3gpu queues the draw and does not wait,
                    no readback DMA, no CPU 6-bit convert, no CPU re-swizzle and upload; the GPU compositor samples the eye render target
                    directly (about 12 ms less per 3D frame expected). Only with gpu_compose=2, plain 256x256 3D, no CPU fog; frames the
                    GPU compositor rejects (or any CPU consumer) read the 3D back on demand. Colour is 8 bit (not 6 bit expanded), so
                    pixels differ slightly from the CPU path. Log `[GPU3D] direct 600 3D frames: ...` shows direct vs normal frames, how
                    many were resolved to the CPU, and the reasons it was not used. Compare PERF/[GPU3D] avg G3G_End with it ON and OFF.
                    [2026-10-26 fps thread] Since stage 1b Wide view frames also go direct (the GPU copies the centre, the CPU
                    converts only the side strips); Wide FX on still uses the old path. `[GPU3D] direct resolves by reason` counts
                    why the 3D was still pulled back to the CPU.
  gpu_direct3d_wide=1  with gpu_direct3d=1: 0 = Wide view frames use the old path (config.txt only, for diagnosis).
  dc_flush_skip=0   EXPERIMENTAL (default 0 = OFF; Debug tab: SKIP GAME CACHE FLUSHES). 1 = the game's own DC_FlushRange calls are ignored (each is a GSP
                    IPC call); audio and platform GPU/DSP flushes are untouched. Log: `[HEAP] dc_flush` lines show calls, KB and (OFF) time spent.
  fps_tweaks=0      EXPERIMENTAL (default 0 = OFF; Debug tab: SPEED TWEAKS = OFF / BOTTOM / +FADES / +3D BLEND / +TRIM / +OVERLAP, values 0 / 2 / 3 / 4 / 5 / 6;
                    6 = + OVERLAP (Platinum frame_pacing, overlap part only): with the 3D slider up the left eye's GPU compose is not waited for;
                    the CPU prepares the right eye meanwhile. Log: [GPUC] overlap. 1 is treated as 0).
                    5 = + CPU trims (no GPU change): the wide-view side-strip conversion takes a one-OR-per-pixel fast path (bit-identical),
                    and two diagnostics are skipped (the mid-composite OAM/palette hash check and the stereo blob scan).
                    Safe boot: a non-zero level is forced back to 0 if the previous boot with it never reached 600 frames
                    (fps_tweaks.pending file). fps_leanflush=1 (config.txt only, default 0) = citro3d frames end without the
                    whole-linear-heap flush; it HUNG the game on hardware, do not use. 2 = engine B not composed while the Bottom menu
                    covers the bottom screen, and the Bottom menu caches both blink states (no 10.7 ms repaint twice a second),
                    3 = + master-brightness fades composed on the GPU (no CPU pull-back of the 3D during fades). 4 = + 3D translucency
                    in battles composed on the GPU (Debug row: +3D BLEND; checked: BLDCNT no first target, all second targets, black clear colour). `[PERF4]` lines
                    time every stage with it on or off. [2026-10-27 fps thread]
  menu_audio=1      music and sounds keep playing while the L+R+Y settings menu is open (the game itself stays
                    paused). 0 = frozen like before. Default 1. (Menu: MENU MUSIC)  [run 61]
  log_level=1       1 = short log.txt; 2 = detailed diagnostics (writes much more to the SD card and can cost
                    some speed - only for bug hunting). Default 1.

Everything else in old config files (stereo_c1, steal_b, gpu3d, tearfix, rb_sync, ...) is for diagnosis only;
leave it out unless asked.

stereo_edgefix=1        (run 62, default 1) 3D slider + wide view: keep the 3D triangles that the per-eye stereo
                        shift brings on screen at the far left (right eye) / far right (left eye). Fixes the thin
                        black sliver at the left edge of the right eye. 0 = old behaviour.

fog_animfix=1           (run 64, default 1) Fixes the player's walk animation freezing (or facing the wrong way)
                        after a seamless map change such as Route 210 South -> North in the fog. The game freed the
                        player's walk-frame table while the sprite still used it; the port now keeps it. 0 = old.

credits_splash=1        (run 64, default 1) Boot credits "Patch by Unown Collectables & 3DS Homebrew Dev" on the top
                        screen while the game starts (drawn by the port in the game's own font, read from your
                        ROM). A skips it. 0 = off.
splash_ms=3000          (run 64) Splash length in ms including fades (300..5000).

ui_gamefont=1           (run 64, default 1) Port text boxes (follower talk, toasts, save box, settings app) use the
                        game's own font and message-box frame, read from your ROM at start. 0 = old port font.
ui_frame=0              (run 64) Which of the game's 20 message-box frames the port boxes use (0..19).
settings_app        (retired 2026-10-28; the settings app is the only settings screen, the key is ignored)
fps60_probe=0           (default 0 = off, log only) 1: log.txt gets '[FPS] field 10 s' (busy vs idle time per game tick in the
                    overworld) and '[FPS] probe' (how stable the 3D triangle order is between frames). Data for a future
                    60 fps overworld, see docs/FPS60.md. Changes nothing on screen.
fast_text=2             (default 2) 2, 3, 4 or 64 (instant): the game prints that many letters per frame. Stops at
                        every pause / button prompt, so no text is skipped. Settings app, System tab (Fast text).
reusable_tms=0          (run 64, default 0) 1 = TMs are not used up when you teach them. Also in the settings app.
follower_gap=1          (run 64, default 1) The following Pokemon walks a bit farther back (more for big Pokemon), so
                        it no longer covers your head. 0 = old spacing.
follower_area_talk=1    (run 64, default 1) With follower_talk=1: what your Pokemon says depends on where you are
                        (town, route, cave, lake, legendary place), the weather, the time of day and its type.
follower_cry=1          (run 64, default 1) Your Pokemon cries when you talk to it (the game's own cry sound).
follower_items=0        (run 64, default 0) 1 = now and then, after you talk to it, your Pokemon finds a common item
                        (Potion, Poke Ball, Repel, berries ...). It is put in your Bag the normal game way, so your
                        save stays a normal save. At most once every 120 steps.
follower_ballpop=1      (run 64, default 1) After a door, warp or cutscene your Pokemon comes out of its ball: the
                        game's ball sound and it grows in for a moment. 0 = it just appears.
settings_touch=1        (run 66, default 1) Overworld touch shortcuts in the bottom screen's corners where the Poketch has
                        no screen or button. Touch and HOLD about 0.3 s: top-left = this settings app (same as L+R+Y),
                        top-right = Pokemon (party), bottom-right = Bag. Party and Bag are the game's own screens, opened
                        through the game's own Start menu (same as the Menu tab), so saving works normally. A short tap
                        in a corner does nothing. 0 = off.
Cheats tab (run 67, every cheat OFF by default, stored in config.txt; values written to the save are ones the real
game accepts):
cheat_money=1           Infinite money: in the overworld your money is kept at 999,999 (the game's own maximum).
cheat_catch=1           Always catch: every Poke Ball thrown in a wild battle catches (the game's own "always catch" result).
cheat_shiny=1           Every wild Pokemon is shiny (uses the game's own Poke Radar shiny code; Cute Charm/Synchronize kept).
cheat_eggs=1            Fast eggs: eggs in your party hatch after a few steps (egg cycles set to 0, the normal hatch event).
cheat_exp=1 / 2         EXP boost: battle EXP x2 / x3.
cheat_candy=1           Rare Candy 99: in the overworld your Bag is topped up to 99 Rare Candies (the Bag allows 999).
cheat_no_encounters=1   (run 64) no random wild battles.
Debug tab (run 67): the old Speed tab (fast upload, fast 3D convert, GPU compose, GPU compose 3D, log detail) is hidden;
press SELECT inside the settings app to show / hide it. The config.txt keys still work as before.
Run 67: config.txt is read from the SD card once and then kept in memory (it used to be re-read after every map
change / battle, ~13 ms each on hardware).
lid_sound=1             (run 64, default 1; run 65 sounds) Closing the lid plays the game's Poke Ball recall
                        (return) sound, opening it the Poke Ball release (send-out) sound - the same sounds as in battle
                        (overworld only; the music keeps playing / resumes). NOTE: those two sounds are in the game's battle
                        sound bank, which the game doesn't keep loaded in the overworld; when they can't start (always so far
                        in testing) the game's overworld Poke Ball sound (the Poke Center ball sound) plays instead. 0 = silent.
lid_test=1              (run 65, DEBUG ONLY) fakes a lid close / open about 5 and 7 s into the overworld. Default 0.
summary_ivs=1           (default 1) On the Pokemon summary screen press Y to show / hide a box
                        with the Pokemon's IVs and EVs, in the order HP/Atk/Def/SpA/SpD/Spe. Read only.
                        Also in the settings app (Game page 2).
tracker=1              (default 0; Settings > Play) Roamer + breeding tracker on the JOHTO bottom map: coloured R/E/L/L marks on
                       the roaming Pokemon's location (Raikou, Entei, Latias, Latios), and the progress overview (menu-dots button)
                       lists them plus the Day-Care (egg chance per 256 steps, steps to the next roll, Masuda yes/no with the
                       egg's shiny odds). Read-only; data from pspoke hook 0120. Suicune does not roam in HeartGold/SoulSilver.
ribbon_check=1          (default 1; Settings > Battle) On the Pokemon summary screen press X for a read-only ribbon
                        checklist: Earned in this game (n/14), Still available, From earlier games. X turns the page,
                        the last page closes it. Never writes to the Pokemon or the save. Y (IV/EV) is unchanged.
celebrations=1          (default 1; Settings > Battle) Birthday and holiday particle effects in the overworld (visual only).
birthdays=10-27,03-14   Your birthday list, MM-DD, comma separated, up to 32. Lives only in this file. On those days a
                        star sparkle shows in Sprout Tower (New Bark Town too). Beats any holiday that day.
                        Holidays: Jan 1 fireworks Goldenrod, Feb 14 hearts Cherrygrove, Mar 17 green sparkles Ilex
                        Forest, Jul 1 red/white sparkles Olivine, Oct 31 purple wisps Lavender Town, Dec 25 snow Mahogany.
celeb_test=N            (test only, config.txt) forces effect N on every overworld map: 1 birthday, 2 fireworks, 3 hearts,
                        4 green rain, 5 red/white, 6 wisps, 7 snow. 0 = real dates.
repel_prompt=1          (run 64, default 1) When a Repel wears off and you still have one, after the game's "Repel's
                        effect wore off" message you are asked "Use another Repel?" (Up/Down, A = yes, B = no).
                        Yes uses it exactly like the Bag does (one taken from the Bag, normal Repel length).
                        Uses the kind you used last (else Repel, Super Repel, Max Repel). Game page 2 of the app.
cheat_no_encounters=0   (run 64 cheat, default 0) 1 = no random wild battles while walking / surfing; Sweet Scent and
                        Great Marsh mud encounters are blocked too. Fishing, Honey trees and scripted /
                        legendary Pokemon are unchanged.
                        Nothing is written to your save. Settings app, Cheats tab.

Run 73
  (Platinum's wild_vis_max / wild_vis_test / wild_vis_species are not used here; SoulSilver's keys are in the Gameplay section)
  (no new key) wide_weather copies only grouped weather particles (no cursor / lone sprites); wide_edgefill is off
  indoors and only extends repeating background; wide_menus continues only the screen background layer; wide-view
  strips hold still during attack screen shake. 3D vertex buffer raised (Spear Pillar cutscene abort).
cheat_pp=0  (1 = your moves use no PP in battle; runtime only)
cheat_speed=0        (1 = holding B runs at bike speed; runtime only)
cheat_walk_walls=0   (1 = hold R while walking to pass through walls/objects; warps and ledges still work)
cheat_time=0         (0 off, 1 morning, 2 day, 3 evening, 4 night: overrides time-of-day only; the save clock is untouched)
catch_overlay=1      (default 1, no menu row any more; 0 = off) Y at the command menu in battle opens the foe info screen (its own screen, not the settings app). Wild: species, nature, ability, IVs, Pokerus, HP/status, catch odds. Trainer (hook 0031): active foe HP/status/ability/moves with PP, page 2 its party; Left/Right flips, B closes
battle_anim_speed=1  (1 = skip plain move animations; Fly/Dig/Dive/Bounce/Shadow Force/Transform/Substitute/Minimize still play)
Run 76
  battle_anim_speed=2  (also makes the HP bar drain 3x faster; damage and end values unchanged)
  display_capture=1    (screen capture used by the Spear Pillar / Distortion World transition; 0 = old behaviour)
  dw_depth=2           (Distortion World 3D depth: 0 off, 1 subtle, 2 moderate, 3 strong; eases in over ~0.75 s)
  dw_debris=0          (1 = drifting debris and dust over the Distortion World, with depth in 3D)
  dw_unease=0          (1 = occasional uneasy line from your Pokemon in the Distortion World; text only)
  dw_cry=0             (Platinum only; ignored)
  hunt_counter=0       (old key, superseded by shiny_suite; 1 still counts encounters in hunt.txt)
  shiny_suite=1        (default 1, no menu row any more; 0 = off. Ported from Platinum) 1 = hunt.txt keeps per species: encounters, encounters since the last shiny,
                       soft resets (any soft reset while that species is the current hunt), shinies found, hunting time, start date. Written after every
                       encounter, soft reset, lid close, exit and every 2 minutes (async, via hunt.tmp). Old "species=count" lines still load. Never the save.
                       On a shiny foe: chime + "SHINY POKEMON!" toast + a top-screen flash. In a wild battle Y opens the foe info screen and Left/Right
                       (or a tap) flips to page 2: that species' hunt stats, the odds, chance of a shiny by now, and the 50%/90% encounter counts.
  shiny_rate=0         (Battle tab "Shiny odds", pspoke hook 0113) wild shiny odds: 0 = 1/8192 (original), 1 = 1/4096, 2 = 1/2048, 3 = 1/1024. Grass, caves,
                       Rock Smash, surfing, fishing, Headbutt; the visible sparkle (wild_vis_shiny) uses the same odds. Not gifts, eggs, roamers, Safari, Contest.
                       The game's own shiny code makes the Pokemon (like the Poke Radar); the extra roll is the port's own generator, never the game's RNG.
  battery_warn=0       (Battle tab "Low battery warning") at 1 bar or less and not charging: "BATTERY LOW: SAVE NOW" once a minute and hunt.txt is written at once.
type_hint=0           (Battle tab "Move type hint") while the FIGHT menu is open, each damaging move button on the TOUCH screen gets a small tag: SUPER EFFECTIVE (green), NOT VERY EFF. (red) or NO EFFECT (grey); nothing for neutral or status moves (doubles show the better of the two foes). Gen 4 chart; abilities/items not considered. Hook 0121, read-only. Log tag [HINT] (also prints a colour profile of the bottom image so the tag position can be checked).
  tod_tint=0           (1 = soft evening / night tint outdoors; follows cheat_time when set)
  bottom_vramgen=1     (Underground fix: bottom screen redraws when its VRAM is written; 0 = old reuse)
  Ball tab now also lists Dusk and Net Ball odds. Log adds [PERF3] (worst frame, bottom reuse) every ~10 s.
  item_ping=0          (1 = soft Poketch dowsing blip when a hidden item comes within 2 tiles; read-only)
Run 77
  touch_menu=0         (1 = touch and hold the bottom-left corner of the bottom screen for 0.3 s in the overworld: quick menu
                        with Pokemon / Bag / Pokedex / Trainer card / Save / Options, Map tab next to it; uses the real Start menu)
  dw_roll=0            (1 = Distortion World: ~400 ms damped tilt plus a small 3D depth pulse when you move to a new gravity plane)
Run 78
  bottom_strip=0       (1 = overworld only: Menu / Wide / Ball / Set buttons in the bottom border below the Poketch; taps there never reach the Poketch. Wide toggles wide view for this session only)
Run 79
  (no new keys) The HOME Menu .cia (make cia) uses the same sdmc:/3ds/soulsilver3ds/ config.txt, soulsilver.sav and log.txt as the .3dsx.
Run 80
  text_hold_b=0        (Settings > System. 1 = holding B prints text instantly and advances finished text boxes; never picks yes/no or menu choices)
  text_auto=0          (Settings > System. 1 = text boxes advance by themselves after ~1.5 s; choices still wait for you)

SoulSilver gameplay toggles (Settings > Gameplay, patch 0040; all default 0, apply at once, nothing extra is saved)
reusable_tms=0          1 = teaching a TM does not use it up ([ITEM] log line).
hm_free=1               1 = Cut, Surf, Strength, Rock Smash, Waterfall, Whirlpool and Rock Climb work in the field
                        with no Pokemon needing to know or learn the HM (patch 0115; the first non-egg Pokemon does it, and the
                        Pokemon menu lists each HM usable right now). Badges and story checks are still required
                        ([HM] log lines). Field moves picked from the party menu still need a Pokemon that knows them.
repel_prompt=1          1 = when a Repel wears off and you have another of that kind, "Use another <item>?" appears.
wireless=0              (2026-10-26, TEST) 1 = local wireless between two 3DSes running this port (or the Platinum
                        port): trades and battles at the Pokemon Center 2F counters. 3DS to 3DS only, not with a
                        real DS. Used from the next time the game starts wireless. Back up your save first and test
                        on copies. Play tab: "Local wireless (test)". Log lines: [WIRELESS].
cheat_no_encounters=0   1 = no random wild battles in grass, caves, water, fishing or Rock Smash ([CHEAT] log line).
                        Sweet Scent, Headbutt, roamers and scripted battles are unchanged.

bottom_bump=1           (2026-10-28; Settings > Input > X tile cursor) 1 = X moves a highlight over the Bottom menu tiles, A taps it, B drops it (old: lit the seven action buttons one
                        after another (a quick wave), so the tiles act as the main menu. The game's own Start menu opens under the tiles and
                        is closed again by itself. Log: [BMENU] X pressed: button wave. 0 = no wave.
bottom_menu=1           (ss3ds, default 1 since run 17; Settings > Controls > Bottom menu) 1 = in the overworld the touch screen shows tiles for the
                        Start-menu entries (Pokedex, Pokemon, Bag, Pokegear, Trainer card, Save, Options). A tap opens the game's own
                        Start menu and selects that entry (pspoke patch 0060), so saving etc. work normally. The game gets no touch
                        while the tiles are up; the real Start menu (X) hides them. Greyed = not in your menu yet. Log tag [BMENU].
bottom_layout / bottom_map  (removed 2026-10-28; still accepted in config.txt but ignored) The 4x2 Bottom menu and the INFO/WORLD/LOCAL
                        panels are deleted. The JOHTO screen is the only Bottom menu: Pokegear map on the left, silver/blue column on the right
                        (POKEDEX, POKEMON, BAG, POKEGEAR, TRAINER, SAVE, OPTIONS, MAP-recentre button, RUN and a Y ITEM square).
                        Log: [BMENU] layout JOHTO, [BMENU] tap ..., [BMENU] tap Y ITEM.
  run 10 (JOHTO layout): the map panel is the Pokegear's own Johto/Kanto map, read from your ROM at runtime (pspoke hook
                        0081; never written to the SD card or committed). Places you have not been yet are dimmed; a place
                        lights up the first time you enter it and stays lit. The lit set is kept in visited.bin next to
                        config.txt (24 bytes: "VB01", trainer ID, 100 bits), never in the save; another trainer ID (new
                        game) starts empty, and a missing file means nothing visited. Tap a place: name, VISITED / NOT YET
                        and the Pokegear's description; tap it again (or the card) to close; drag sideways to pan. MAP
                        button: JOHTO -> INFO -> WORLD -> LOCAL. RUN square (shoe): run on/off latch (as if B were held),
                        only once the game has given the Running Shoes; ZL does the same with zl_run=1.
pg_kanto=0              (run 16, default 0; Settings > Controls > Kanto on map) JOHTO map: 0 = GATED, Kanto stays hidden until you have
                        been to a Kanto place (like the real Pokegear after the S.S. Aqua; recognised by name, places west of Kanto
                        such as Route 26/27 and Victory Road stay visible). 1 = ALWAYS show the whole map. Log tag [PGMAP].

pg_extras=1             (2026-10-28, default 1; Settings > Input > Pokegear tabs) JOHTO panel: MAP / PHONE / RADIO tabs on the top bar. PHONE = the
                        Pokegear's registered numbers (drag/fling to scroll, drag the right edge to scrub, SORT chip, tap a name to find the
                        place on the map), RADIO = stations tunable here and what is on air now, MAP gets contact pins. 0 = plain map as before.
                        Needs pspoke hook 0122 for game data. Log tags [PHONE] / [RADIO].

Visible wild Pokemon (SoulSilver hook 0110, Settings > Play, all live, default OFF until tested on hardware)
  wild_vis=1          1 = wild Pokemon walk around on the map: grass, caves, any tile with wild encounters, and water
                      (surf table). Species and level come from the game's own tables (morning/day/night, swarms,
                      Hoenn/Sinnoh radio); roamers (Raikou, Entei...) on the current map show up too. Stepping onto one
                      starts the game's own battle for exactly that Pokemon (Repel still applies). Sprites: sdmc:/3ds/soulsilver3ds/followers.bin, built
                      on the 3DS from soulsilver.nds the first time it is needed (a few seconds, once; log "[FOL] build").
                      They stay through menus, the bike, scripts and other battles; one you ran from stays too (and
                      keeps away from you). A map change, walking 12+ tiles away, or winning/catching removes them. Not in the Safari Zone or the Bug-Catching Contest. Nothing is saved.
  wild_vis_count=2    how many at once (1..6)
  wild_vis_shiny=1    1 = one can spawn shiny (1/8192): it sparkles, a chime plays, "SHINY NEARBY!", and its battle is shiny
  wild_vis_replace=0  1 = no random land/surf encounters while wild_vis is on (only the visible ones battle). Fishing,
                      Rock Smash, Headbutt and scripted Pokemon are unchanged.
  wild_vis_cries=1    1 = a Pokemon that notices you (3 tiles) plays its cry (it always hops and shows a "!")
  Behaviour: about half wander, some walk up to you and wait (curious), some run away (shy; roamers always run).
  In tall grass the lower part is hidden; elsewhere on land they cast a soft shadow. Log: [WILD] spawn / noticed / contact / removed lines.
