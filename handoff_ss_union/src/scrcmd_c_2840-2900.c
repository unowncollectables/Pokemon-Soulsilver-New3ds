        return FALSE;
    }
}

BOOL sub_02044434(ScriptContext *ctx);

BOOL ScrCmd_273(ScriptContext *ctx) {
    u16 var_idx = ScriptReadHalfword(ctx);
    ctx->data[0] = var_idx;
    SetupNativeScript(ctx, sub_02044434);
    return TRUE;
}

BOOL sub_02044434(ScriptContext *ctx) {
    u16 *p_ret = GetVarPointer(ctx->fieldSystem, ctx->data[0]);
    int r0 = sub_0205A39C(ctx->fieldSystem->unk80);
    if (gSystem.newKeys & PAD_BUTTON_B) {
        r0 = sub_0205A47C(ctx->fieldSystem->unk80, 8);
    }
    if (r0 != 0) {
        *p_ret = r0;
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL ScrCmd_286(ScriptContext *ctx) {
    sub_02054030(ctx->fieldSystem->taskman);
    return TRUE;
}

BOOL ScrCmd_BufferUnionRoomAvatarChoices(ScriptContext *ctx) {
    PlayerProfile *profile = Save_PlayerData_GetProfile(FieldSystem_GetSaveData(ctx->fieldSystem));
    MessageFormat **p_msgFmt = FieldSysGetAttrAddr(ctx->fieldSystem, SCRIPTENV_MESSAGE_FORMAT);
    BufferUnionRoomAvatarChoicesNames(
        PlayerProfile_GetTrainerID(profile),
        PlayerProfile_GetTrainerGender(profile),
        *p_msgFmt);
    return FALSE;
}

BOOL ScrCmd_UnionRoomAvatarIdxToTrainerClass(ScriptContext *ctx) {
    PlayerProfile *profile = Save_PlayerData_GetProfile(FieldSystem_GetSaveData(ctx->fieldSystem));
    u16 choice = ScriptGetVar(ctx);
    u16 *p_ret = ScriptGetVarPointer(ctx);
    *p_ret = UnionRoomAvatarIdxToSprite(
        PlayerProfile_GetTrainerID(profile),
        PlayerProfile_GetTrainerGender(profile),
        choice);
    *p_ret = GetUnionRoomAvatarAttrBySprite(
        PlayerProfile_GetTrainerGender(profile),
        *p_ret,
        2);
    return FALSE;
}

BOOL ScrCmd_UnionRoomAvatarIdxToSprite(ScriptContext *ctx) {
    PlayerProfile *profile = Save_PlayerData_GetProfile(FieldSystem_GetSaveData(ctx->fieldSystem));
    u16 choice = ScriptGetVar(ctx);
    u16 *p_ret = ScriptGetVarPointer(ctx);
