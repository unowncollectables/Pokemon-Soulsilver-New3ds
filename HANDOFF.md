
- 0147 (unbuilt, Grok): NULL guard in SysTask_TimedPaletteFade for the run 67 crash leaving Kurt's house after the Fast Ball. User builds and tests.
- 0148 (unbuilt, Grok): seed overlay_01_021F1348.s, drop the ov01_021F1A34 stub (run 67 Slowpoke Well crash, [SS-MISSING] ov01_021F1A34).
- 0149 (unbuilt, Grok): seed unk_0206979C.s, drop 3 stubs (Slowpoke Well [SS-MISSING] sub_02069998 after 0148).
- 0150 (unbuilt, Grok): native-provided sub_02020DA4 (link dupe after 0149, same as 0144).
- 0151 (unbuilt, Grok): sub_020154C4 only SPL_Deletes an emitter still in act_emtr_list (run 67 battle crash pc in SPLList_PopFront via SPL_Delete).
- 0152 (unbuilt, Grok): SPLManager_DeleteEmitter skips emitters not in activeEmitters (0151 only covered sub_020154C4; Supersonic still crashed). libss-particles.a must rebuild.
- 0153 (Grok): ss3ds_game.sh re-syncs particle glue.c and applies 0152 to a staged build.py, then rebuilds libss-particles.a (0151/0152 never reached an already-staged tree).
- 0153 is now also a pspoke patch: the builder runs pspoke/scripts/ss3ds_game.sh (live copy), not scripts/ss3ds_game.sh (reference). Put build-script fixes in a pspoke patch.
