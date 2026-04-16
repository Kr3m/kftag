// Copyright (C) 1999-2000 Id Software, Inc.
// g_rtf.c - RTF gametype custom state tracking

#include "g_local.h"

/* Forward declarations for g_team.c internals used only by g_rtf.c */
void QDECL PrintMsg( gentity_t *ent, const char *fmt, ... );
void Team_ForceGesture( team_t team );
void Team_CaptureFlagSound( gentity_t *ent, team_t team );

rtf_game_state_t rtf_state;

// Forward declarations
static void RTF_UpdateBaseVisibility( rtf_flag_t *flag );
static void RTF_CleanupDroppedEntity( rtf_flag_t *flag );

/*
================
RTF_Init

Initialize the RTF state tracking system
================
*/
void RTF_Init( void ) {
    gentity_t *ent;
    int redCount, blueCount;
    int flagId = 0;

    memset(&rtf_state, 0, sizeof(rtf_state));

    redCount = 0;
    blueCount = 0;

    // Find all red flag base entities (skip dropped items)
    ent = NULL;
    while ((ent = G_Find(ent, FOFS(classname), "team_CTF_redflag")) != NULL) {
        if (ent->flags & FL_DROPPED_ITEM) continue;
        if (flagId < MAX_RTF_FLAGS) {
            rtf_flag_t *flag = &rtf_state.flags[flagId];
            flag->flagId = flagId;
            flag->baseEntity = ent;
            flag->team = TEAM_RED;
            flag->flagIndex = redCount++;
            flag->state = RTF_FLAG_AT_BASE;
            flag->carrier = -1;
            flag->baseVisible = qtrue;
            flag->droppedEntity = NULL;
            flag->stateChangeTime = level.time;

            // Ensure base entity is visible
            ent->s.eFlags &= ~EF_NODRAW;
            ent->r.svFlags &= ~SVF_NOCLIENT;
            trap_LinkEntity(ent);

            flagId++;
        }
    }

    // Find all blue flag base entities (skip dropped items)
    ent = NULL;
    while ((ent = G_Find(ent, FOFS(classname), "team_CTF_blueflag")) != NULL) {
        if (ent->flags & FL_DROPPED_ITEM) continue;
        if (flagId < MAX_RTF_FLAGS) {
            rtf_flag_t *flag = &rtf_state.flags[flagId];
            flag->flagId = flagId;
            flag->baseEntity = ent;
            flag->team = TEAM_BLUE;
            flag->flagIndex = blueCount++;
            flag->state = RTF_FLAG_AT_BASE;
            flag->carrier = -1;
            flag->baseVisible = qtrue;
            flag->droppedEntity = NULL;
            flag->stateChangeTime = level.time;

            // Ensure base entity is visible
            ent->s.eFlags &= ~EF_NODRAW;
            ent->r.svFlags &= ~SVF_NOCLIENT;
            trap_LinkEntity(ent);

            flagId++;
        }
    }

    rtf_state.numFlags = flagId;
    rtf_state.initialized = qtrue;
    rtf_state.lastFlagId = flagId;

    // Initialize player states
    {
        int i;
        for (i = 0; i < MAX_CLIENTS; i++) {
            rtf_state.players[i].carryingFlags[0] = -1;
            rtf_state.players[i].carryingFlags[1] = -1;
            rtf_state.players[i].numCarried = 0;
            rtf_state.players[i].lastPickupTime = 0;
        }
    }

    G_Printf("RTF: Initialized with %d red flags, %d blue flags\n", redCount, blueCount);
}

/*
================
RTF_FindFlagByBaseEntity

Find flag tracking entry by base entity pointer
================
*/
static rtf_flag_t *RTF_FindFlagByBaseEntity( gentity_t *ent ) {
    int i;
    for (i = 0; i < rtf_state.numFlags; i++) {
        if (rtf_state.flags[i].baseEntity == ent) {
            return &rtf_state.flags[i];
        }
    }
    return NULL;
}

/*
================
RTF_FindFlagByDroppedEntity

Find flag tracking entry by dropped entity pointer
================
*/
static rtf_flag_t *RTF_FindFlagByDroppedEntity( gentity_t *ent ) {
    int i;
    for (i = 0; i < rtf_state.numFlags; i++) {
        if (rtf_state.flags[i].droppedEntity == ent) {
            return &rtf_state.flags[i];
        }
    }
    return NULL;
}

/*
================
RTF_FindFlagByCarrier

Find flag tracking entry by carrier client number and team
================
*/
static rtf_flag_t *RTF_FindFlagByCarrier( int clientNum, team_t team ) {
    int i;
    for (i = 0; i < rtf_state.numFlags; i++) {
        if (rtf_state.flags[i].team == team &&
            rtf_state.flags[i].carrier == clientNum &&
            rtf_state.flags[i].state == RTF_FLAG_CARRIED) {
            return &rtf_state.flags[i];
        }
    }
    return NULL;
}

/*
================
RTF_UpdateBaseVisibility

Update the base entity's visibility based on flag state
================
*/
static void RTF_UpdateBaseVisibility( rtf_flag_t *flag ) {
    if (!flag->baseEntity) return;

    if (flag->state == RTF_FLAG_AT_BASE) {
        flag->baseEntity->s.eFlags &= ~EF_NODRAW;
        flag->baseEntity->r.svFlags &= ~SVF_NOCLIENT;
        flag->baseVisible = qtrue;
    } else {
        flag->baseEntity->s.eFlags |= EF_NODRAW;
        flag->baseEntity->r.svFlags |= SVF_NOCLIENT;
        flag->baseVisible = qfalse;
    }
    trap_LinkEntity(flag->baseEntity);
}

/*
================
RTF_CleanupDroppedEntity

Remove dropped entity and update tracking
================
*/
static void RTF_CleanupDroppedEntity( rtf_flag_t *flag ) {
    if (flag->droppedEntity) {
        if (flag->droppedEntity->inuse) {
            G_FreeEntity(flag->droppedEntity);
        }
        flag->droppedEntity = NULL;
    }
}


/*
================
RTF_PickupFlag

Handle player picking up a flag (base or dropped)
Returns qtrue if successful
================
*/
qboolean RTF_PickupFlag( gentity_t *player, gentity_t *flagEnt ) {
    rtf_flag_t *flag;
    rtf_player_state_t *ps;
    int flag_pw;

    if (!player->client) return qfalse;

    /* Find which RTF slot this entity belongs to. */
    flag = RTF_FindFlagByBaseEntity(flagEnt);
    if (!flag) {
        flag = RTF_FindFlagByDroppedEntity(flagEnt);
    }
    if (!flag) return qfalse;

    ps = &rtf_state.players[player->s.number];
    flag_pw = (flag->team == TEAM_RED) ? PW_REDFLAG : PW_BLUEFLAG;

    /* Already carrying this team's flag — no stacking same-team flags. */
    if (player->client->ps.powerups[flag_pw]) {
        return qfalse;
    }

    /* Hard cap: carry at most 2 flags total. */
    if (ps->numCarried >= 2) {
        return qfalse;
    }

    /* Own flag at base cannot be picked up — only enemy flags and dropped
       own flags trigger the enemy/dropped pickup path. */
    if (flag->team == player->client->sess.sessionTeam &&
        flag->state == RTF_FLAG_AT_BASE) {
        return qfalse;
    }

    G_Printf("RTF: %s picked up %s flag (state=%d)\n",
             player->client->pers.netname, TeamName(flag->team), flag->state);

    /* If picking up a dropped flag, clear its entity reference —
       Touch_Item will free the dropped entity automatically on return -1. */
    if (flag->state == RTF_FLAG_DROPPED) {
        flag->droppedEntity = NULL;
    }

    /* Update tracking state. */
    flag->state = RTF_FLAG_CARRIED;
    flag->carrier = player->s.number;
    flag->stateChangeTime = level.time;

    /* Update player carry list. */
    ps->carryingFlags[ps->numCarried++] = flag->flagId;
    ps->lastPickupTime = level.time;

    /* Set powerup. */
    player->client->ps.powerups[flag_pw] = INT_MAX;
    player->client->pers.teamState.flagsince = level.time;

    /* Hide the base entity now that the flag is away. */
    RTF_UpdateBaseVisibility(flag);

    /* Sounds and HUD. */
    Team_SetFlagStatus(flag->team, FLAG_TAKEN);
    if (flag->team != player->client->sess.sessionTeam) {
        PrintMsg(NULL, "%s" S_COLOR_WHITE " got the %s flag!\n",
                 player->client->pers.netname, TeamName(flag->team));
        Team_TakeFlagSound(flagEnt, flag->team);
    } else {
        PrintMsg(NULL, "%s" S_COLOR_WHITE " secured the %s flag!\n",
                 player->client->pers.netname, TeamName(flag->team));
    }

    return qtrue;
}

/*
================
RTF_ReturnFlag

Return a carried flag to the touched pole (poleFlag).
If poleFlag is a different slot from the flag's current home, the two
slots' baseEntity pointers are swapped so the flag visually lives on the
pole it was physically returned to.  Safe: single-threaded, atomic here.
================
*/
static qboolean RTF_ReturnFlag( gentity_t *player, rtf_flag_t *flag, rtf_flag_t *poleFlag ) {
    int flag_pw = (flag->team == TEAM_RED) ? PW_REDFLAG : PW_BLUEFLAG;
    rtf_player_state_t *ps;
    gentity_t *swapEnt;
    int i, j;

    /* Sanity: must be CARRIED by this player. */
    if (flag->state != RTF_FLAG_CARRIED || flag->carrier != player->s.number) {
        return qfalse;
    }

    /* If player returned the flag to a different (empty) pole, swap the
       two slots' baseEntity bindings so the flag stays on that pole. */
    if (poleFlag != flag) {
        swapEnt            = flag->baseEntity;
        flag->baseEntity   = poleFlag->baseEntity;
        poleFlag->baseEntity = swapEnt;
    }

    G_Printf("RTF: %s returning %s flag to %s\n",
             player->client->pers.netname, TeamName(flag->team),
             (poleFlag == flag) ? "home pole" : "alternate pole");

    flag->state = RTF_FLAG_AT_BASE;
    flag->carrier = -1;
    flag->stateChangeTime = level.time;
    RTF_UpdateBaseVisibility(flag);

    /* Update player carry list. */
    ps = &rtf_state.players[player->s.number];
    for (i = 0; i < ps->numCarried; i++) {
        if (ps->carryingFlags[i] == flag->flagId) {
            for (j = i; j < ps->numCarried - 1; j++) {
                ps->carryingFlags[j] = ps->carryingFlags[j+1];
            }
            ps->carryingFlags[ps->numCarried - 1] = -1;
            ps->numCarried--;
            break;
        }
    }

    player->client->ps.powerups[flag_pw] = 0;

    AddScore(player, flag->baseEntity->r.currentOrigin, CTF_RECOVERY_BONUS);
    player->client->pers.teamState.flagrecovery++;
    player->client->pers.teamState.lastreturnedflag = level.time;

    Team_ReturnFlagSound(flag->baseEntity, flag->team);
    PrintMsg(NULL, "%s" S_COLOR_WHITE " returned the %s flag!\n",
             player->client->pers.netname, TeamName(flag->team));

    Team_SetFlagStatus(flag->team, FLAG_ATBASE);

    return qtrue;
}

/*
================
RTF_CaptureFlag

Capture an enemy flag at own base
================
*/
static void RTF_CaptureFlag( gentity_t *player, rtf_flag_t *enemyFlag, rtf_flag_t *capturePole ) {
    int i, j;
    gentity_t *teammate;
    rtf_player_state_t *ps = &rtf_state.players[player->s.number];
    int enemy_pw = (enemyFlag->team == TEAM_RED) ? PW_REDFLAG : PW_BLUEFLAG;

    G_Printf("RTF: %s captured %s flag at pole %d\n",
             player->client->pers.netname, TeamName(enemyFlag->team), capturePole->flagIndex);

    // Remove enemy flag from player
    for (i = 0; i < ps->numCarried; i++) {
        if (ps->carryingFlags[i] == enemyFlag->flagId) {
            ps->carryingFlags[i] = -1;
            for (j = i; j < ps->numCarried - 1; j++) {
                ps->carryingFlags[j] = ps->carryingFlags[j+1];
            }
            ps->numCarried--;
            break;
        }
    }

    player->client->ps.powerups[enemy_pw] = 0;

    // Return captured flag to its base
    enemyFlag->state = RTF_FLAG_AT_BASE;
    enemyFlag->carrier = -1;
    enemyFlag->stateChangeTime = level.time;
    RTF_UpdateBaseVisibility(enemyFlag);

    // Award capture
    teamgame.last_flag_capture = level.time;
    teamgame.last_capture_team = player->client->sess.sessionTeam;
    AddTeamScore(capturePole->baseEntity->s.pos.trBase, player->client->sess.sessionTeam, 1);
    Team_ForceGesture(player->client->sess.sessionTeam);

    player->client->pers.teamState.captures++;
    player->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP);
    player->client->ps.eFlags |= EF_AWARD_CAP;
    player->client->rewardTime = level.time + REWARD_SPRITE_TIME;
    player->client->ps.persistant[PERS_CAPTURES]++;
    AddScore(player, capturePole->baseEntity->r.currentOrigin, CTF_CAPTURE_BONUS);
    Team_CaptureFlagSound(capturePole->baseEntity, player->client->sess.sessionTeam);

    /* Update configstring so scoreboard/HUD reflect the returned enemy flag. */
    Team_SetFlagStatus(enemyFlag->team, FLAG_ATBASE);

    // Assist bonuses for teammates
    for (i = 0; i < level.maxclients; i++) {
        teammate = &g_entities[i];
        if (!teammate->inuse || !teammate->client || teammate == player)
            continue;
        if (teammate->client->sess.sessionTeam != player->client->sess.sessionTeam)
            continue;

        if (teammate->client->pers.teamState.lastreturnedflag + CTF_RETURN_FLAG_ASSIST_TIMEOUT > level.time) {
            AddScore(teammate, capturePole->baseEntity->r.currentOrigin, CTF_RETURN_FLAG_ASSIST_BONUS);
            player->client->pers.teamState.assists++;
            teammate->client->ps.persistant[PERS_ASSIST_COUNT]++;
            teammate->client->ps.eFlags |= EF_AWARD_ASSIST;
            teammate->client->rewardTime = level.time + REWARD_SPRITE_TIME;
        }
        if (teammate->client->pers.teamState.lastfraggedcarrier + CTF_FRAG_CARRIER_ASSIST_TIMEOUT > level.time) {
            AddScore(teammate, capturePole->baseEntity->r.currentOrigin, CTF_FRAG_CARRIER_ASSIST_BONUS);
            player->client->pers.teamState.assists++;
            teammate->client->ps.persistant[PERS_ASSIST_COUNT]++;
            teammate->client->ps.eFlags |= EF_AWARD_ASSIST;
            teammate->client->rewardTime = level.time + REWARD_SPRITE_TIME;
        }
    }

    PrintMsg(NULL, "%s" S_COLOR_WHITE " captured the %s flag!\n",
             player->client->pers.netname, TeamName(enemyFlag->team));

    CalculateRanks();
}

/*
================
RTF_TouchBasePole

Handle player touching a base pole (for returning/capturing)
================
*/
void RTF_TouchBasePole( gentity_t *player, gentity_t *pole ) {
    rtf_flag_t *poleFlag;
    rtf_flag_t *carriedOwnFlag;
    rtf_flag_t *carriedEnemyFlag;
    team_t playerTeam;

    if (!player->client) return;

    poleFlag = RTF_FindFlagByBaseEntity(pole);
    if (!poleFlag) return;

    playerTeam = player->client->sess.sessionTeam;
    if (poleFlag->team != playerTeam) return;

    carriedOwnFlag   = RTF_FindFlagByCarrier(player->s.number, playerTeam);
    carriedEnemyFlag = RTF_FindFlagByCarrier(player->s.number, OtherTeam(playerTeam));

    if (poleFlag->state != RTF_FLAG_AT_BASE) {
        /* -------------------------------------------------------------------
           Touched pole is EMPTY.
           Rule 3: both own + enemy flag → return own here, then capture on
           the now-occupied pole (atomic, single touch).
           Plain return: own flag only → flag stays on this pole.
           Either case accepts any empty same-team pole (baseEntity swap
           inside RTF_ReturnFlag handles pole reassignment cleanly).
           ------------------------------------------------------------------- */
        if (carriedOwnFlag) {
            RTF_ReturnFlag(player, carriedOwnFlag, poleFlag);
            if (carriedEnemyFlag) {
                /* poleFlag is now AT_BASE after the return above. */
                RTF_CaptureFlag(player, carriedEnemyFlag, poleFlag);
            }
        }
    } else {
        /* -------------------------------------------------------------------
           Touched pole is OCCUPIED (AT_BASE).
           Rule 2: carrying enemy flag → capture here.
           ------------------------------------------------------------------- */
        if (carriedEnemyFlag) {
            RTF_CaptureFlag(player, carriedEnemyFlag, poleFlag);
        }
    }
}

/*
================
RTF_DropFlag

Handle flag being dropped (player death or manual drop)
================
*/
/* RTF_NotifyFlagDropped

   Called from TossClientItems BEFORE Drop_Item so we can record the
   soon-to-be-created dropped entity.  The actual Drop_Item call and
   powerup clearing is done by TossClientItems — we only update the
   RTF state here to avoid double-dropping.

   Pass droppedEnt = NULL when the flag is being returned to base
   (nodrop zone); in that case state becomes AT_BASE via RTF_ReturnFlag. */
void RTF_NotifyFlagDropped( gentity_t *player, team_t team, gentity_t *droppedEnt ) {
    rtf_flag_t *flag;
    rtf_player_state_t *ps;
    int i, j;

    flag = RTF_FindFlagByCarrier(player->s.number, team);
    if (!flag) return;

    ps = &rtf_state.players[player->s.number];

    if (droppedEnt) {
        /* Flag lands on ground — stays in play. */
        flag->droppedEntity = droppedEnt;
        flag->state = RTF_FLAG_DROPPED;
        /* No auto-return timer: flags never auto-return in RTF unless
           they fall into a nodrop/void zone (Team_FreeEntity handles that). */
        droppedEnt->think = NULL;
        droppedEnt->nextthink = 0;
    } else {
        /* Nodrop / void — flag goes back to base. */
        flag->droppedEntity = NULL;
        flag->state = RTF_FLAG_AT_BASE;
        RTF_UpdateBaseVisibility(flag);
        Team_SetFlagStatus(team, FLAG_ATBASE);
        PrintMsg(NULL, "The %s flag has returned!\n", TeamName(team));
    }

    flag->carrier = -1;
    flag->stateChangeTime = level.time;

    /* Remove from player carry list. */
    for (i = 0; i < ps->numCarried; i++) {
        if (ps->carryingFlags[i] == flag->flagId) {
            for (j = i; j < ps->numCarried - 1; j++) {
                ps->carryingFlags[j] = ps->carryingFlags[j+1];
            }
            ps->carryingFlags[ps->numCarried - 1] = -1;
            ps->numCarried--;
            break;
        }
    }

    G_Printf("RTF: %s dropped/returned %s flag\n",
             player->client->pers.netname, TeamName(team));
}

/*
================
RTF_Reset

Reset all RTF state (for round restart)
================
*/
void RTF_Reset( void ) {
    int i;
    rtf_flag_t *flag;

    for (i = 0; i < rtf_state.numFlags; i++) {
        flag = &rtf_state.flags[i];

        RTF_CleanupDroppedEntity(flag);

        flag->state = RTF_FLAG_AT_BASE;
        flag->carrier = -1;
        flag->stateChangeTime = level.time;

        RTF_UpdateBaseVisibility(flag);
    }

    for (i = 0; i < MAX_CLIENTS; i++) {
        rtf_state.players[i].carryingFlags[0] = -1;
        rtf_state.players[i].carryingFlags[1] = -1;
        rtf_state.players[i].numCarried = 0;
        rtf_state.players[i].lastPickupTime = 0;
    }

    G_Printf("RTF: State reset\n");
}

/*
================
RTF_PlayerDied

Handle player death - drop all carried flags
================
*/
/* RTF_PlayerDied

   Called from TossClientItems (via g_combat.c) BEFORE Drop_Item runs,
   so RTF state transitions happen before the entity is spawned.
   We call RTF_NotifyFlagDropped with NULL for the dropped entity and
   let TossClientItems create the actual item; then we patch the
   droppedEntity pointer in afterwards via RTF_NotifyFlagDropped being
   called again with the real entity from TossClientItems.

   Actually the simplest correct approach: RTF_PlayerDied just marks
   the carry state as DROPPED with no entity; TossClientItems spawns
   the dropped entity.  Team_CheckDroppedItem (called by LaunchItem)
   will call Team_SetFlagStatus(FLAG_DROPPED) which is fine.
   The droppedEntity pointer in the slot stays NULL until the flag
   is picked up again — at that point RTF_PickupFlag finds the slot
   by the dropped entity via RTF_FindFlagByDroppedEntity, so we must
   patch the pointer.  We do that by scanning all DROPPED slots whose
   droppedEntity is NULL and linking them to the newly created entity
   in RTF_LinkDroppedEntity, called from LaunchItem. */
/* Mark one carried flag as DROPPED (no entity yet).
   Called for TEAM_RED and TEAM_BLUE separately. */
static void RTF_PreDropFlag( gentity_t *player, team_t team ) {
    rtf_flag_t *flag;
    rtf_player_state_t *ps;
    int i, j;

    flag = RTF_FindFlagByCarrier(player->s.number, team);
    if (!flag) return;

    ps = &rtf_state.players[player->s.number];

    flag->state         = RTF_FLAG_DROPPED;
    flag->droppedEntity = NULL;   /* TossClientItems will create the entity */
    flag->carrier       = -1;
    flag->stateChangeTime = level.time;

    /* Remove from player carry list. */
    for (i = 0; i < ps->numCarried; i++) {
        if (ps->carryingFlags[i] == flag->flagId) {
            for (j = i; j < ps->numCarried - 1; j++) {
                ps->carryingFlags[j] = ps->carryingFlags[j+1];
            }
            ps->carryingFlags[ps->numCarried - 1] = -1;
            ps->numCarried--;
            break;
        }
    }

    G_Printf("RTF: %s died, %s flag now dropped (entity pending)\n",
             player->client->pers.netname, TeamName(team));
}

void RTF_PlayerDied( gentity_t *player ) {
    if (!player->client) return;

    /* Pre-mark flags as DROPPED so state is consistent when TossClientItems
       runs Drop_Item.  Powerups are left intact so TossClientItems can find
       and drop them normally.  RTF_LinkDroppedEntity patches in the entity
       pointer once Drop_Item creates the entity. */
    if (player->client->ps.powerups[PW_REDFLAG]) {
        RTF_PreDropFlag(player, TEAM_RED);
    }
    if (player->client->ps.powerups[PW_BLUEFLAG]) {
        RTF_PreDropFlag(player, TEAM_BLUE);
    }
}

/*
================
RTF_LinkDroppedEntity

Called from LaunchItem when a flag entity is created (dropped on death).
Links the new dropped entity to the correct DROPPED slot so that
RTF_FindFlagByDroppedEntity works for subsequent relay pickups.
================
*/
void RTF_LinkDroppedEntity( gentity_t *dropped ) {
    int flag_pw;
    team_t team;
    int i;

    if (!rtf_state.initialized) return;
    if (!dropped || !dropped->item) return;

    flag_pw = dropped->item->giTag;
    if (flag_pw == PW_REDFLAG) team = TEAM_RED;
    else if (flag_pw == PW_BLUEFLAG) team = TEAM_BLUE;
    else return;

    /* Find the first DROPPED slot for this team that has no entity yet. */
    for (i = 0; i < rtf_state.numFlags; i++) {
        if (rtf_state.flags[i].team == team &&
            rtf_state.flags[i].state == RTF_FLAG_DROPPED &&
            rtf_state.flags[i].droppedEntity == NULL) {
            rtf_state.flags[i].droppedEntity = dropped;
            /* No auto-return in RTF — clear any inherited timer. */
            dropped->think = NULL;
            dropped->nextthink = 0;
            return;
        }
    }
}

/*
================
RTF_FlagDroppedIntoVoid

Called when a dropped flag entity is freed (e.g. fell into a void or
nodrop zone).  The flag returns automatically to its base.
================
*/
void RTF_FlagDroppedIntoVoid( gentity_t *droppedEnt ) {
    rtf_flag_t *flag;

    if (!rtf_state.initialized || !droppedEnt) return;

    flag = RTF_FindFlagByDroppedEntity(droppedEnt);
    if (!flag) return;

    flag->droppedEntity   = NULL;
    flag->state           = RTF_FLAG_AT_BASE;
    flag->carrier         = -1;
    flag->stateChangeTime = level.time;

    RTF_UpdateBaseVisibility(flag);
    Team_SetFlagStatus(flag->team, FLAG_ATBASE);
    Team_ReturnFlagSound(flag->baseEntity, flag->team);
    PrintMsg(NULL, "The %s flag has returned!\n", TeamName(flag->team));

    G_Printf("RTF: Dropped %s flag returned to base (out of world)\n",
             TeamName(flag->team));
}

/*
================
RTF_PlayerNodropDeath

Called when a player dies in a nodrop zone while carrying RTF flags.
Each carried flag is returned to base without spawning a dropped entity.
================
*/
void RTF_PlayerNodropDeath( gentity_t *player ) {
    rtf_player_state_t *ps;
    rtf_flag_t *flag;
    int flag_pw;
    int flagId;
    int i;

    if (!rtf_state.initialized || !player->client) return;

    ps = &rtf_state.players[player->s.number];

    for (i = 0; i < ps->numCarried; i++) {
        flagId = ps->carryingFlags[i];
        if (flagId < 0 || flagId >= rtf_state.numFlags) continue;

        flag    = &rtf_state.flags[flagId];
        flag_pw = (flag->team == TEAM_RED) ? PW_REDFLAG : PW_BLUEFLAG;

        player->client->ps.powerups[flag_pw] = 0;

        flag->state           = RTF_FLAG_AT_BASE;
        flag->droppedEntity   = NULL;
        flag->carrier         = -1;
        flag->stateChangeTime = level.time;

        RTF_UpdateBaseVisibility(flag);
        Team_SetFlagStatus(flag->team, FLAG_ATBASE);
        Team_ReturnFlagSound(flag->baseEntity, flag->team);
        PrintMsg(NULL, "The %s flag has returned!\n", TeamName(flag->team));

        G_Printf("RTF: %s died in nodrop zone, %s flag returned\n",
                 player->client->pers.netname, TeamName(flag->team));
    }

    ps->carryingFlags[0] = -1;
    ps->carryingFlags[1] = -1;
    ps->numCarried = 0;
}
