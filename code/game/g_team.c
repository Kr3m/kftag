// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "g_local.h"

typedef struct teamgame_s {
	float			last_flag_capture;
	int				last_capture_team;
	flagStatus_t	redStatus;	// CTF
	flagStatus_t	blueStatus;	// CTF
	flagStatus_t	flagStatus;	// One Flag CTF
	int				redTakenTime;
	int				blueTakenTime;
	int				redObeliskAttackedTime;
	int				blueObeliskAttackedTime;
} teamgame_t;

teamgame_t teamgame;

gentity_t	*neutralObelisk;

// RTF state tracking
static rtf_team_state_t rtf_redState;
static rtf_team_state_t rtf_blueState;
static qboolean rtf_tracking_initialized = qfalse;

static void Team_SetFlagStatus( team_t team, flagStatus_t status );

// RTF helper function declarations
static void RTF_InitFlagTracking( void );
static rtf_flag_t *RTF_FindFlagByEntity( gentity_t *flag );
static rtf_flag_t *RTF_FindCarriedFlagByPlayer( gentity_t *player, team_t team );
static void RTF_MarkFlagTaken( gentity_t *flag, gentity_t *carrier );
static void RTF_MarkFlagReturned( gentity_t *flag );
static void RTF_MarkFlagCaptured( gentity_t *flag );
static void RTF_RemoveFlagFromCarrier( gentity_t *player, team_t team );
// Sound/message helpers needed by Team_RTF_ReturnPlayerFlags (defined later)
static void Team_ReturnFlagSound( gentity_t *ent, team_t team );
void QDECL PrintMsg( gentity_t *ent, const char *fmt, ... );

void Team_InitGame( void ) {
	memset(&teamgame, 0, sizeof teamgame);

	switch( g_gametype.integer ) {
	case GT_CTF:
	case GT_RTF:
		teamgame.redStatus = -1; // Invalid to force update
		Team_SetFlagStatus( TEAM_RED, FLAG_ATBASE );
		teamgame.blueStatus = -1; // Invalid to force update
		Team_SetFlagStatus( TEAM_BLUE, FLAG_ATBASE );

		// Initialize RTF flag tracking if this is RTF mode
		if ( g_gametype.integer == GT_RTF ) {
			RTF_InitFlagTracking();
		}
		break;
#ifdef MISSIONPACK
	case GT_1FCTF:
		teamgame.flagStatus = -1; // Invalid to force update
		Team_SetFlagStatus( TEAM_FREE, FLAG_ATBASE );
		break;
#endif
	default:
		break;
	}
}

/*
================
RTF_InitFlagTracking

Initialize tracking for all flags on the map
================
*/
static void RTF_InitFlagTracking( void ) {
	gentity_t *ent;
	int redCount, blueCount;

	memset(&rtf_redState, 0, sizeof(rtf_redState));
	memset(&rtf_blueState, 0, sizeof(rtf_blueState));

	rtf_redState.team = TEAM_RED;
	rtf_blueState.team = TEAM_BLUE;

	redCount = 0;
	blueCount = 0;

	// Find all red BASE flags (skip dropped items - they share classname but have FL_DROPPED_ITEM set)
	ent = NULL;
	while ((ent = G_Find(ent, FOFS(classname), "team_CTF_redflag")) != NULL) {
		if (ent->flags & FL_DROPPED_ITEM) {
			continue; // skip dropped flag entities; they are transient
		}
		if (redCount < MAX_FLAGS_PER_TEAM) {
			rtf_redState.flags[redCount].ent = ent;
			rtf_redState.flags[redCount].flagIndex = redCount;
			// A base entity is at-base if it is visible (not hidden by pickup)
			rtf_redState.flags[redCount].isAtBase = !(ent->s.eFlags & EF_NODRAW);
			rtf_redState.flags[redCount].isCarried = qfalse;
			rtf_redState.flags[redCount].carrier = -1;
			rtf_redState.flags[redCount].takenTime = 0;
			if (rtf_redState.flags[redCount].isAtBase) {
				rtf_redState.flagsAtBase++;
			}
			redCount++;
		}
	}
	rtf_redState.numFlags = redCount;

	// Find all blue BASE flags (skip dropped items)
	ent = NULL;
	while ((ent = G_Find(ent, FOFS(classname), "team_CTF_blueflag")) != NULL) {
		if (ent->flags & FL_DROPPED_ITEM) {
			continue; // skip dropped flag entities; they are transient
		}
		if (blueCount < MAX_FLAGS_PER_TEAM) {
			rtf_blueState.flags[blueCount].ent = ent;
			rtf_blueState.flags[blueCount].flagIndex = blueCount;
			// A base entity is at-base if it is visible (not hidden by pickup)
			rtf_blueState.flags[blueCount].isAtBase = !(ent->s.eFlags & EF_NODRAW);
			rtf_blueState.flags[blueCount].isCarried = qfalse;
			rtf_blueState.flags[blueCount].carrier = -1;
			rtf_blueState.flags[blueCount].takenTime = 0;
			if (rtf_blueState.flags[blueCount].isAtBase) {
				rtf_blueState.flagsAtBase++;
			}
			blueCount++;
		}
	}
	rtf_blueState.numFlags = blueCount;

	rtf_tracking_initialized = qtrue;

	G_Printf("RTF: Found %d red flags, %d blue flags\n", redCount, blueCount);
}

/*
================
RTF_FindFlagByEntity

Find which flag slot corresponds to a given entity
================
*/
static rtf_flag_t *RTF_FindFlagByEntity( gentity_t *flag ) {
	int i;

	if (!flag) return NULL;

	// Check red flags
	for (i = 0; i < rtf_redState.numFlags; i++) {
		if (rtf_redState.flags[i].ent == flag) {
			return &rtf_redState.flags[i];
		}
	}

	// Check blue flags
	for (i = 0; i < rtf_blueState.numFlags; i++) {
		if (rtf_blueState.flags[i].ent == flag) {
			return &rtf_blueState.flags[i];
		}
	}

	return NULL;
}

/*
================
RTF_FindCarriedFlagByPlayer

Find which flag entity a player is carrying for a specific team
================
*/
static rtf_flag_t *RTF_FindCarriedFlagByPlayer( gentity_t *player, team_t team ) {
	rtf_team_state_t *state;
	int flag_pw;
	int i;

	flag_pw = (team == TEAM_RED) ? PW_REDFLAG : PW_BLUEFLAG;

	if (!player->client->ps.powerups[flag_pw]) {
		return NULL;
	}

	state = (team == TEAM_RED) ? &rtf_redState : &rtf_blueState;

	for (i = 0; i < state->numFlags; i++) {
		if (state->flags[i].isCarried && state->flags[i].carrier == player->s.number) {
			return &state->flags[i];
		}
	}

	return NULL;
}

/*
================
RTF_TeamOfFlag

Determine the team from a flag tracking slot's classname.
Returns TEAM_RED or TEAM_BLUE.
================
*/
static team_t RTF_TeamOfFlag( rtf_flag_t *flagInfo ) {
	return (flagInfo->ent->item && flagInfo->ent->item->giTag == PW_REDFLAG) ? TEAM_RED : TEAM_BLUE;
}

/*
================
RTF_MarkFlagTaken

Mark a specific flag as taken by a carrier.
Call ONLY for the base entity (not for a dropped item entity –
dropped items are not in the tracking table).
================
*/
static void RTF_MarkFlagTaken( gentity_t *flag, gentity_t *carrier ) {
	rtf_flag_t *flagInfo;
	rtf_team_state_t *state;

	flagInfo = RTF_FindFlagByEntity(flag);
	if (!flagInfo) {
		G_Printf("RTF ERROR: RTF_MarkFlagTaken: could not find flag info for entity #%d\n",
		         (int)(flag - g_entities));
		return;
	}

	if (flagInfo->isCarried) {
		G_Printf("RTF WARNING: RTF_MarkFlagTaken: flag already marked carried\n");
	}

	state = (RTF_TeamOfFlag(flagInfo) == TEAM_RED) ? &rtf_redState : &rtf_blueState;

	if (flagInfo->isAtBase) {
		flagInfo->isAtBase = qfalse;
		if (state->flagsAtBase > 0) state->flagsAtBase--;
	}
	flagInfo->isCarried = qtrue;
	flagInfo->carrier = carrier->s.number;
	flagInfo->takenTime = level.time;
	state->flagsCarried++;

	/* Hide the base entity while the flag is away. */
	flag->s.eFlags |= EF_NODRAW;
	flag->r.svFlags |= SVF_NOCLIENT;
	trap_LinkEntity(flag);
}

/*
================
RTF_MarkFlagReturned

Mark a specific flag as returned to base.
Shows the base entity directly rather than going through
RespawnItem (which has teamed-entity random-selection side effects).
================
*/
static void RTF_MarkFlagReturned( gentity_t *flag ) {
	rtf_flag_t *flagInfo;
	rtf_team_state_t *state;

	flagInfo = RTF_FindFlagByEntity(flag);
	if (!flagInfo) {
		G_Printf("RTF ERROR: RTF_MarkFlagReturned: could not find flag info for entity #%d\n",
		         (int)(flag - g_entities));
		return;
	}

	state = (RTF_TeamOfFlag(flagInfo) == TEAM_RED) ? &rtf_redState : &rtf_blueState;

	if (flagInfo->isCarried && state->flagsCarried > 0) {
		state->flagsCarried--;
	}
	flagInfo->isAtBase = qtrue;
	flagInfo->isCarried = qfalse;
	flagInfo->carrier = -1;
	state->flagsAtBase++;

	/* Make the base pole entity visible and touchable again. */
	flag->r.contents = CONTENTS_TRIGGER;
	flag->s.eFlags &= ~EF_NODRAW;
	flag->r.svFlags &= ~SVF_NOCLIENT;
	flag->r.svFlags |= SVF_BROADCAST;
	flag->nextthink = 0;
	flag->think = NULL;
	trap_LinkEntity(flag);
}

/*
================
RTF_MarkFlagCaptured

When a capture happens, return only the specific enemy flag that was captured
to its base.  Other flags are NOT affected.
================
*/
static void RTF_MarkFlagCaptured( gentity_t *flag ) {
	rtf_flag_t *flagInfo;
	rtf_team_state_t *state;

	flagInfo = RTF_FindFlagByEntity(flag);
	if (!flagInfo) {
		G_Printf("RTF ERROR: RTF_MarkFlagCaptured: could not find flag info for entity #%d\n",
		         (int)(flag - g_entities));
		return;
	}

	state = (RTF_TeamOfFlag(flagInfo) == TEAM_RED) ? &rtf_redState : &rtf_blueState;

	if (flagInfo->isCarried && state->flagsCarried > 0) {
		state->flagsCarried--;
	}
	flagInfo->isAtBase = qtrue;
	flagInfo->isCarried = qfalse;
	flagInfo->carrier = -1;
	state->flagsAtBase++;

	/* Restore only this flag's base entity. */
	flag->r.contents = CONTENTS_TRIGGER;
	flag->s.eFlags &= ~EF_NODRAW;
	flag->r.svFlags &= ~SVF_NOCLIENT;
	flag->r.svFlags |= SVF_BROADCAST;
	flag->nextthink = 0;
	flag->think = NULL;
	trap_LinkEntity(flag);
}

/*
================
RTF_RemoveFlagFromCarrier

Force remove a flag from a carrier when they die or disconnect.
Updates RTF tracking state but does NOT clear the powerup (caller
handles that via TossClientItems / Drop_Item).
================
*/
static void RTF_RemoveFlagFromCarrier( gentity_t *player, team_t team ) {
	rtf_flag_t *flagInfo;
	rtf_team_state_t *state;
	int flag_pw;

	flag_pw = (team == TEAM_RED) ? PW_REDFLAG : PW_BLUEFLAG;

	if (!player->client->ps.powerups[flag_pw]) {
		return;
	}

	flagInfo = RTF_FindCarriedFlagByPlayer(player, team);
	if (flagInfo) {
		state = (team == TEAM_RED) ? &rtf_redState : &rtf_blueState;
		flagInfo->isCarried = qfalse;
		flagInfo->carrier = -1;
		/* isAtBase stays false — the flag is now a dropped entity on the map */
		if (state->flagsCarried > 0) {
			state->flagsCarried--;
		}
	}
}

/*
================
Team_RTF_DropFlags

Called when a player drops their flags (on death/disconnect) in RTF mode.
Updates the RTF tracking state for all flags the player was carrying.
Must be called BEFORE the powerups are cleared from ps.powerups so that
RTF_RemoveFlagFromCarrier can check the powerup to confirm ownership.
================
*/
void Team_RTF_DropFlags( gentity_t *player ) {
	if ( g_gametype.integer != GT_RTF ) {
		return;
	}
	RTF_RemoveFlagFromCarrier( player, TEAM_RED );
	RTF_RemoveFlagFromCarrier( player, TEAM_BLUE );
}

/*
================
Team_RTF_ReturnPlayerFlags

Return any flags the player is carrying to their bases.  Intended for use
when the player dies in a NODROP area so no dropped entity is spawned.
Mirrors the nodrop "Team_ReturnFlag" path but operates per-carrier rather
than resetting all flags of the team.
================
*/
void Team_RTF_ReturnPlayerFlags( gentity_t *player ) {
	int i;
	rtf_team_state_t *states[2];
	team_t teams[2];

	if ( g_gametype.integer != GT_RTF || !rtf_tracking_initialized ) {
		return;
	}

	states[0] = &rtf_redState;  teams[0] = TEAM_RED;
	states[1] = &rtf_blueState; teams[1] = TEAM_BLUE;

	for (i = 0; i < 2; i++) {
		rtf_team_state_t *state = states[i];
		int j;
		for (j = 0; j < state->numFlags; j++) {
			rtf_flag_t *slot = &state->flags[j];
			if (slot->isCarried && slot->carrier == player->s.number) {
				RTF_MarkFlagReturned(slot->ent);
				Team_ReturnFlagSound(slot->ent, teams[i]);
				PrintMsg(NULL, "The %s flag has returned!\n", TeamName(teams[i]));
			}
		}
	}
}

int OtherTeam( team_t team ) {
	if ( team == TEAM_RED )
		return TEAM_BLUE;
	else if ( team == TEAM_BLUE )
		return TEAM_RED;
	return team;
}

const char *TeamName( team_t team ) {
	if ( team == TEAM_RED )
		return "RED";
	else if ( team == TEAM_BLUE )
		return "BLUE";
	else if ( team == TEAM_SPECTATOR )
		return "SPECTATOR";
	return "FREE";
}

const char *OtherTeamName( team_t team ) {
	if ( team == TEAM_RED )
		return "BLUE";
	else if ( team == TEAM_BLUE )
		return "RED";
	else if ( team == TEAM_SPECTATOR )
		return "SPECTATOR";
	return "FREE";
}

const char *TeamColorString( team_t team ) {
	if ( team == TEAM_RED )
		return S_COLOR_RED;
	else if ( team == TEAM_BLUE )
		return S_COLOR_BLUE;
	else if ( team == TEAM_SPECTATOR )
		return S_COLOR_YELLOW;
	return S_COLOR_WHITE;
}

// NULL for everyone
void QDECL PrintMsg( gentity_t *ent, const char *fmt, ... ) {
	char		msg[1024];
	va_list		argptr;
	char		*p;

	va_start (argptr,fmt);
	if ( ED_vsprintf( msg, fmt, argptr ) >= sizeof( msg ) ) {
		G_Error ( "PrintMsg overrun" );
	}
	va_end (argptr);

	// double quotes are bad
	while ((p = strchr(msg, '"')) != NULL)
		*p = '\'';

	trap_SendServerCommand ( ( (ent == NULL) ? -1 : ent-g_entities ), va("print \"%s\"", msg ));
}

/*
==============
AddTeamScore

 used for gametype > GT_TEAM
 for gametype GT_TEAM the level.teamScores is updated in AddScore in g_combat.c
==============
*/
void AddTeamScore( vec3_t origin, team_t team, int score ) {
	int			eventParm;
	int			otherTeam;
	gentity_t	*te;

	if ( score == 0 ) {
		return;
	}

	eventParm = -1;
	otherTeam = OtherTeam( team );

	if ( level.teamScores[ team ] + score == level.teamScores[ otherTeam ] ) {
		//teams are tied sound
		eventParm = GTS_TEAMS_ARE_TIED;
	} else if ( level.teamScores[ team ] >= level.teamScores[ otherTeam ] &&
				level.teamScores[ team ] + score < level.teamScores[ otherTeam ] ) {
		// other team took the lead sound (negative score)
		eventParm = ( otherTeam == TEAM_RED ) ? GTS_REDTEAM_TOOK_LEAD : GTS_BLUETEAM_TOOK_LEAD;
	} else if ( level.teamScores[ team ] <= level.teamScores[ otherTeam ] &&
				level.teamScores[ team ] + score > level.teamScores[ otherTeam ] ) {
		// this team took the lead sound
		eventParm = ( team == TEAM_RED ) ? GTS_REDTEAM_TOOK_LEAD : GTS_BLUETEAM_TOOK_LEAD;
	} else if ( score > 0 && g_gametype.integer != GT_TEAM ) {
		// team scored sound
		eventParm = ( team == TEAM_RED ) ? GTS_REDTEAM_SCORED : GTS_BLUETEAM_SCORED;
	}

	if ( eventParm != -1 ) {
		te = G_TempEntity(origin, EV_GLOBAL_TEAM_SOUND );
		te->r.svFlags |= SVF_BROADCAST;
		te->s.eventParm = eventParm;
	}

	level.teamScores[ team ] += score;
}

/*
==============
OnSameTeam
==============
*/
qboolean OnSameTeam( gentity_t *ent1, gentity_t *ent2 ) {
	if ( !ent1->client || !ent2->client ) {
		return qfalse;
	}

	if ( g_gametype.integer < GT_TEAM ) {
		return qfalse;
	}

	if ( ent1->client->sess.sessionTeam == ent2->client->sess.sessionTeam ) {
		return qtrue;
	}

	return qfalse;
}

static char ctfFlagStatusRemap[] = { '0', '1', '*', '*', '2' };
static char oneFlagStatusRemap[] = { '0', '1', '2', '3', '4' };

static void Team_SetFlagStatus( team_t team, flagStatus_t status ) {
	qboolean modified = qfalse;

	switch( team ) {
	case TEAM_RED:	// CTF
		if ( teamgame.redStatus != status ) {
			teamgame.redStatus = status;
			modified = qtrue;
		}
		break;

	case TEAM_BLUE:	// CTF
		if ( teamgame.blueStatus != status ) {
			teamgame.blueStatus = status;
			modified = qtrue;
		}
		break;

	case TEAM_FREE:	// One Flag CTF
		if ( teamgame.flagStatus != status ) {
			teamgame.flagStatus = status;
			modified = qtrue;
		}
		break;

	default:
		return;
	}

	if ( modified ) {
		char st[4];

		if ( g_gametype.integer == GT_CTF || g_gametype.integer == GT_RTF ) {
			st[0] = ctfFlagStatusRemap[teamgame.redStatus];
			st[1] = ctfFlagStatusRemap[teamgame.blueStatus];
			st[2] = '\0';
		} else {	// GT_1FCTF
			st[0] = oneFlagStatusRemap[teamgame.flagStatus];
			st[1] = '\0';
		}

		trap_SetConfigstring( CS_FLAGSTATUS, st );
	}
}

void Team_CheckDroppedItem( gentity_t *dropped ) {
	if( dropped->item->giTag == PW_REDFLAG ) {
		Team_SetFlagStatus( TEAM_RED, FLAG_DROPPED );
	}
	else if( dropped->item->giTag == PW_BLUEFLAG ) {
		Team_SetFlagStatus( TEAM_BLUE, FLAG_DROPPED );
	}
	else if( dropped->item->giTag == PW_NEUTRALFLAG ) {
		Team_SetFlagStatus( TEAM_FREE, FLAG_DROPPED );
	}
}

/*
================
Team_ForceGesture
================
*/
void Team_ForceGesture( team_t team ) {
	int i;
	gentity_t *ent;

	for ( i = 0; i < level.maxclients; i++ ) {
		ent = &g_entities[i];
		if ( !ent->inuse )
			continue;
		if ( !ent->client )
			continue;
		if ( ent->client->sess.sessionTeam != team )
			continue;
		//
		ent->flags |= FL_FORCE_GESTURE;
	}
}

/*
================
Team_FragBonuses

Calculate the bonuses for flag defense, flag carrier defense, etc.
Note that bonuses are not cumulative.  You get one, they are in importance
order.
================
*/
void Team_FragBonuses(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker)
{
	int i;
	gentity_t *ent;
	int flag_pw, enemy_flag_pw;
	int otherteam;
	int tokens;
	gentity_t *flag, *carrier = NULL;
	char *c;
	vec3_t v1, v2;
	int team;

	// no bonus for fragging yourself or team mates
	if (!targ->client || !attacker->client || targ == attacker || OnSameTeam(targ, attacker))
		return;

	team = targ->client->sess.sessionTeam;
	otherteam = OtherTeam(targ->client->sess.sessionTeam);
	if (otherteam < 0)
		return; // whoever died isn't on a team

	// same team, if the flag at base, check to he has the enemy flag
	if (team == TEAM_RED) {
		flag_pw = PW_REDFLAG;
		enemy_flag_pw = PW_BLUEFLAG;
	} else {
		flag_pw = PW_BLUEFLAG;
		enemy_flag_pw = PW_REDFLAG;
	}

#ifdef MISSIONPACK
	if (g_gametype.integer == GT_1FCTF) {
		enemy_flag_pw = PW_NEUTRALFLAG;
	}
#endif

	// did the attacker frag the flag carrier?
	tokens = 0;
#ifdef MISSIONPACK
	if( g_gametype.integer == GT_HARVESTER ) {
		tokens = targ->client->ps.generic1;
	}
#endif
	if (targ->client->ps.powerups[enemy_flag_pw]) {
		attacker->client->pers.teamState.lastfraggedcarrier = level.time;
		AddScore(attacker, targ->r.currentOrigin, CTF_FRAG_CARRIER_BONUS);
		attacker->client->pers.teamState.fragcarrier++;
		PrintMsg(NULL, "%s" S_COLOR_WHITE " fragged %s's flag carrier!\n",
			attacker->client->pers.netname, TeamName(team));

		// the target had the flag, clear the hurt carrier
		// field on the other team
		for (i = 0; i < level.maxclients; i++) {
			ent = g_entities + i;
			if (ent->inuse && ent->client->sess.sessionTeam == otherteam)
				ent->client->pers.teamState.lasthurtcarrier = 0;
		}
		return;
	}

	// did the attacker frag a head carrier? other->client->ps.generic1
	if (tokens) {
		attacker->client->pers.teamState.lastfraggedcarrier = level.time;
		AddScore(attacker, targ->r.currentOrigin, CTF_FRAG_CARRIER_BONUS * tokens * tokens);
		attacker->client->pers.teamState.fragcarrier++;
		PrintMsg(NULL, "%s" S_COLOR_WHITE " fragged %s's skull carrier!\n",
			attacker->client->pers.netname, TeamName(team));

		// the target had the flag, clear the hurt carrier
		// field on the other team
		for (i = 0; i < level.maxclients; i++) {
			ent = g_entities + i;
			if (ent->inuse && ent->client->sess.sessionTeam == otherteam)
				ent->client->pers.teamState.lasthurtcarrier = 0;
		}
		return;
	}

	if (targ->client->pers.teamState.lasthurtcarrier &&
		level.time - targ->client->pers.teamState.lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT &&
		!attacker->client->ps.powerups[flag_pw]) {
		// attacker is on the same team as the flag carrier and
		// fragged a guy who hurt our flag carrier
		AddScore(attacker, targ->r.currentOrigin, CTF_CARRIER_DANGER_PROTECT_BONUS);

		attacker->client->pers.teamState.carrierdefense++;
		targ->client->pers.teamState.lasthurtcarrier = 0;

		attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
		team = attacker->client->sess.sessionTeam;
		// add the sprite over the player's head
		attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
		attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
		attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

		return;
	}

	if (targ->client->pers.teamState.lasthurtcarrier &&
		level.time - targ->client->pers.teamState.lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT) {
		// attacker is on the same team as the skull carrier and
		AddScore(attacker, targ->r.currentOrigin, CTF_CARRIER_DANGER_PROTECT_BONUS);

		attacker->client->pers.teamState.carrierdefense++;
		targ->client->pers.teamState.lasthurtcarrier = 0;

		attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
		team = attacker->client->sess.sessionTeam;
		// add the sprite over the player's head
		attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
		attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
		attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

		return;
	}

	// flag and flag carrier area defense bonuses

	// we have to find the flag and carrier entities

#ifdef MISSIONPACK
	if( g_gametype.integer == GT_OBELISK ) {
		// find the team obelisk
		switch (attacker->client->sess.sessionTeam) {
		case TEAM_RED:
			c = "team_redobelisk";
			break;
		case TEAM_BLUE:
			c = "team_blueobelisk";
			break;
		default:
			return;
		}

	} else if (g_gametype.integer == GT_HARVESTER ) {
		// find the center obelisk
		c = "team_neutralobelisk";
	} else {
#endif
	// find the flag
	switch (attacker->client->sess.sessionTeam) {
	case TEAM_RED:
		c = "team_CTF_redflag";
		break;
	case TEAM_BLUE:
		c = "team_CTF_blueflag";
		break;
	default:
		return;
	}
	// find attacker's team's flag carrier
	for (i = 0; i < level.maxclients; i++) {
		carrier = g_entities + i;
		if (carrier->inuse && carrier->client->ps.powerups[flag_pw])
			break;
		carrier = NULL;
	}
#ifdef MISSIONPACK
	}
#endif
	flag = NULL;
	while ((flag = G_Find (flag, FOFS(classname), c)) != NULL) {
		if (!(flag->flags & FL_DROPPED_ITEM))
			break;
	}

	if (!flag)
		return; // can't find attacker's flag

	// ok we have the attackers flag and a pointer to the carrier

	// check to see if we are defending the base's flag
	VectorSubtract(targ->r.currentOrigin, flag->r.currentOrigin, v1);
	VectorSubtract(attacker->r.currentOrigin, flag->r.currentOrigin, v2);

	if ( ( ( VectorLength(v1) < CTF_TARGET_PROTECT_RADIUS &&
		trap_InPVS(flag->r.currentOrigin, targ->r.currentOrigin ) ) ||
		( VectorLength(v2) < CTF_TARGET_PROTECT_RADIUS &&
		trap_InPVS(flag->r.currentOrigin, attacker->r.currentOrigin ) ) ) &&
		attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam) {

		// we defended the base flag
		AddScore(attacker, targ->r.currentOrigin, CTF_FLAG_DEFENSE_BONUS);
		attacker->client->pers.teamState.basedefense++;

		attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
		// add the sprite over the player's head
		attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
		attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
		attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

		return;
	}

	if (carrier && carrier != attacker) {
		VectorSubtract(targ->r.currentOrigin, carrier->r.currentOrigin, v1);
		VectorSubtract(attacker->r.currentOrigin, carrier->r.currentOrigin, v1);

		if ( ( ( VectorLength(v1) < CTF_ATTACKER_PROTECT_RADIUS &&
			trap_InPVS(carrier->r.currentOrigin, targ->r.currentOrigin ) ) ||
			( VectorLength(v2) < CTF_ATTACKER_PROTECT_RADIUS &&
				trap_InPVS(carrier->r.currentOrigin, attacker->r.currentOrigin ) ) ) &&
			attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam) {
			AddScore(attacker, targ->r.currentOrigin, CTF_CARRIER_PROTECT_BONUS);
			attacker->client->pers.teamState.carrierdefense++;

			attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
			// add the sprite over the player's head
			attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
			attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
			attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

			return;
		}
	}
}

/*
================
Team_CheckHurtCarrier

Check to see if attacker hurt the flag carrier.  Needed when handing out bonuses for assistance to flag
carrier defense.
================
*/
void Team_CheckHurtCarrier(gentity_t *targ, gentity_t *attacker)
{
	int flag_pw;

	if (!targ->client || !attacker->client)
		return;

	if (targ->client->sess.sessionTeam == TEAM_RED)
		flag_pw = PW_BLUEFLAG;
	else
		flag_pw = PW_REDFLAG;

	// flags
	if (targ->client->ps.powerups[flag_pw] &&
		targ->client->sess.sessionTeam != attacker->client->sess.sessionTeam)
		attacker->client->pers.teamState.lasthurtcarrier = level.time;

	// skulls
	if (targ->client->ps.generic1 &&
		targ->client->sess.sessionTeam != attacker->client->sess.sessionTeam)
		attacker->client->pers.teamState.lasthurtcarrier = level.time;
}

static gentity_t *Team_ResetFlag( team_t team ) {
	char *c;
	gentity_t *ent, *rent = NULL;

	switch (team) {
	case TEAM_RED:
		c = "team_CTF_redflag";
		break;
	case TEAM_BLUE:
		c = "team_CTF_blueflag";
		break;
	case TEAM_FREE:
		c = "team_CTF_neutralflag";
		break;
	default:
		return NULL;
	}

	ent = NULL;
	while ((ent = G_Find (ent, FOFS(classname), c)) != NULL) {
		if (ent->flags & FL_DROPPED_ITEM) {
			/* Remove stale dropped-flag entities. */
			G_FreeEntity(ent);
		} else {
			rent = ent;
			if ( g_gametype.integer == GT_RTF && rtf_tracking_initialized ) {
				/* In RTF, use the tracking helpers to restore each base entity
				   cleanly.  RTF_MarkFlagReturned makes the entity visible and
				   resets its trigger volume without calling RespawnItem
				   (which can randomly select a teamed-entity partner). */
				rtf_flag_t *flagInfo = RTF_FindFlagByEntity(ent);
				if (flagInfo && !flagInfo->isAtBase) {
					RTF_MarkFlagReturned(ent);
				} else {
					/* Already at base — just make sure it's visible and linked. */
					ent->r.contents = CONTENTS_TRIGGER;
					ent->s.eFlags &= ~EF_NODRAW;
					ent->r.svFlags &= ~SVF_NOCLIENT;
					ent->r.svFlags |= SVF_BROADCAST;
					ent->nextthink = 0;
					ent->think = NULL;
					trap_LinkEntity(ent);
				}
			} else {
				/* Standard CTF path. */
				if (ent->s.eFlags & EF_NODRAW) {
					RespawnItem(ent);
				}
				ent->s.eFlags &= ~EF_NODRAW;
				ent->r.svFlags &= ~SVF_NOCLIENT;
			}
		}
	}

	Team_SetFlagStatus( team, FLAG_ATBASE );

	return rent;
}

void Team_ResetFlags( void ) {
	if( g_gametype.integer == GT_CTF || g_gametype.integer == GT_RTF ) {
		Team_ResetFlag( TEAM_RED );
		Team_ResetFlag( TEAM_BLUE );
	}
#ifdef MISSIONPACK
	else if( g_gametype.integer == GT_1FCTF ) {
		Team_ResetFlag( TEAM_FREE );
	}
#endif
}

static void Team_ReturnFlagSound( gentity_t *ent, team_t team ) {
	gentity_t	*te;

	if (ent == NULL) {
		G_Printf ("Warning:  NULL passed to Team_ReturnFlagSound\n");
		return;
	}

	te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );
	if( team == TEAM_BLUE ) {
		te->s.eventParm = GTS_RED_RETURN;
	}
	else {
		te->s.eventParm = GTS_BLUE_RETURN;
	}
	te->r.svFlags |= SVF_BROADCAST;
}

static void Team_TakeFlagSound( gentity_t *ent, team_t team ) {
	gentity_t	*te;

	if ( ent == NULL ) {
		G_Printf( "Warning:  NULL passed to Team_TakeFlagSound\n" );
		return;
	}

	// In RTF every individual flag pickup must be announced -- the throttle
	// below would suppress the second pickup on 2-flag maps when the first
	// happened within 10 s and the team's own flag is away.
	if ( g_gametype.integer != GT_RTF ) {
		// only play sound when the flag was at the base
		// or not picked up the last 10 seconds
		switch ( team ) {
			case TEAM_RED:
				if( teamgame.blueStatus != FLAG_ATBASE ) {
					if (teamgame.blueTakenTime > level.time - 10000)
						return;
				}
				teamgame.blueTakenTime = level.time;
				break;

			case TEAM_BLUE:
				if( teamgame.redStatus != FLAG_ATBASE ) {
					if (teamgame.redTakenTime > level.time - 10000)
						return;
				}
				teamgame.redTakenTime = level.time;
				break;

			default:
				return;
		}
	} else {
		// Still gate on valid teams and update the taken-time bookkeeping.
		switch ( team ) {
			case TEAM_RED:   teamgame.blueTakenTime = level.time; break;
			case TEAM_BLUE:  teamgame.redTakenTime  = level.time; break;
			default:         return;
		}
	}

	te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );
	if( team == TEAM_BLUE ) {
		te->s.eventParm = GTS_RED_TAKEN;
	}
	else {
		te->s.eventParm = GTS_BLUE_TAKEN;
	}
	te->r.svFlags |= SVF_BROADCAST;
}

static void Team_CaptureFlagSound( gentity_t *ent, team_t team ) {
	gentity_t	*te;

	if (ent == NULL) {
		G_Printf ("Warning:  NULL passed to Team_CaptureFlagSound\n");
		return;
	}

	te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );
	if( team == TEAM_BLUE ) {
		te->s.eventParm = GTS_BLUE_CAPTURE;
	}
	else {
		te->s.eventParm = GTS_RED_CAPTURE;
	}
	te->r.svFlags |= SVF_BROADCAST;
}

void Team_ReturnFlag( team_t team ) {
	Team_ReturnFlagSound(Team_ResetFlag(team), team);
	if( team == TEAM_FREE ) {
		PrintMsg(NULL, "The flag has returned!\n" );
	}
	else {
		PrintMsg(NULL, "The %s flag has returned!\n", TeamName(team));
	}
}

void Team_FreeEntity( gentity_t *ent ) {
	team_t flag_team;

	if( ent->item->giTag == PW_REDFLAG ) {
		flag_team = TEAM_RED;
	} else if( ent->item->giTag == PW_BLUEFLAG ) {
		flag_team = TEAM_BLUE;
	} else if( ent->item->giTag == PW_NEUTRALFLAG ) {
		Team_ReturnFlag( TEAM_FREE );
		return;
	} else {
		return;
	}

	if ( g_gametype.integer == GT_RTF && rtf_tracking_initialized ) {
		/* In RTF a dropped flag falling into void returns only its own base
		   slot, not all flags of the team.  Find the matching unoccupied slot
		   (isAtBase==false, isCarried==false) and restore it. */
		rtf_team_state_t *state;
		int i;
		rtf_flag_t *slot;

		state = (flag_team == TEAM_RED) ? &rtf_redState : &rtf_blueState;
		slot  = NULL;

		for (i = 0; i < state->numFlags; i++) {
			if (!state->flags[i].isAtBase && !state->flags[i].isCarried) {
				slot = &state->flags[i];
				break;
			}
		}

		if (slot) {
			/* RTF_MarkFlagReturned works on the base entity pointer. */
			RTF_MarkFlagReturned(slot->ent);
			Team_ReturnFlagSound(slot->ent, flag_team);
			PrintMsg(NULL, "The %s flag has returned!\n", TeamName(flag_team));
		} else {
			/* Fallback: nothing matched, use legacy reset for the team. */
			Team_ReturnFlag(flag_team);
		}
	} else {
		Team_ReturnFlag(flag_team);
	}
}

/*
==============
Team_DroppedFlagThink

Automatically set in Launch_Item if the item is one of the flags

Flags are unique in that if they are dropped, the base flag must be respawned when they time out
==============
*/
void Team_DroppedFlagThink(gentity_t *ent) {
	int		team = TEAM_FREE;

	if( ent->item->giTag == PW_REDFLAG ) {
		team = TEAM_RED;
	}
	else if( ent->item->giTag == PW_BLUEFLAG ) {
		team = TEAM_BLUE;
	}
	else if( ent->item->giTag == PW_NEUTRALFLAG ) {
		team = TEAM_FREE;
	}

	Team_ReturnFlagSound( Team_ResetFlag( team ), team );
	// Reset Flag will delete this entity
}

/*
==============
Team_TouchOurFlag - RTF mode handler

Called when a player touches their own team's flag entity.
The entity may be:
  (a) FL_DROPPED_ITEM  – a dropped copy of their own flag lying on the map
  (b) a base entity    – the trigger at the flag-pole location (occupied OR empty)

RTF capture semantics:
  • You can capture the enemy flag on ANY own-flag pole, occupied or empty.
  • Occupied pole (your flag is there):
      – Enemy flag only: capture it. Your pole stays occupied.
      – Enemy flag + own flag:  capture the enemy flag here. You keep
        your own flag and can return it to a different (empty) pole.
      – No enemy flag: nothing to do.
  • Empty pole (your flag has been taken or dropped elsewhere):
      – Own flag only: return it.
      – Own flag + enemy flag: return own flag, then immediately capture
        the enemy flag on the now-filled pole.
      – Enemy flag only: NOT a valid capture — pole must be filled first.
  • A player can carry at most one flag of each team simultaneously.
==============
*/
static int Team_TouchOurFlag_RTF( gentity_t *ent, gentity_t *other, team_t team ) {
	gclient_t *cl;
	int enemy_flag;
	int own_flag;
	rtf_flag_t *flagInfo;
	rtf_flag_t *carriedEnemyFlag;
	gentity_t *enemyFlagEnt;

	cl = other->client;
	enemy_flag = (team == TEAM_RED) ? PW_BLUEFLAG : PW_REDFLAG;
	own_flag   = (team == TEAM_RED) ? PW_REDFLAG  : PW_BLUEFLAG;

	/* -------------------------------------------------------
	   Case A: picking up a DROPPED copy of our own flag.
	   ------------------------------------------------------- */
	if ( ent->flags & FL_DROPPED_ITEM ) {
		if ( cl->ps.powerups[own_flag] ) {
			/* Already carrying this team's flag — cannot stack. */
			return 0;
		}

		PrintMsg( NULL, "%s" S_COLOR_WHITE " picked up the %s flag!\n",
		          cl->pers.netname, TeamName(team));

		cl->ps.powerups[own_flag] = INT_MAX;
		cl->pers.teamState.flagsince = level.time;

		/* The dropped entity is NOT in the tracking table, but we need to
		   mark the correct base slot as carried.  Find the slot whose entity
		   we already hid when the flag was first taken. */
		{
			rtf_team_state_t *state;
			int i;
			state = (team == TEAM_RED) ? &rtf_redState : &rtf_blueState;
			for (i = 0; i < state->numFlags; i++) {
				if (!state->flags[i].isAtBase && !state->flags[i].isCarried) {
					/* This slot's flag was previously dropped — now it's carried. */
					state->flags[i].isCarried = qtrue;
					state->flags[i].carrier   = other->s.number;
					state->flags[i].takenTime = level.time;
					if (state->flagsCarried < state->numFlags) {
						state->flagsCarried++;
					}
					break;
				}
			}
		}

		Team_SetFlagStatus(team, FLAG_TAKEN);
		return -1; /* Remove the dropped entity. */
	}

	/* -------------------------------------------------------
	   Case B: touching a BASE entity (the pole trigger).
	   The base entity is always touchable in RTF (r.contents stays
	   CONTENTS_TRIGGER), whether or not the flag is currently at base.
	   ------------------------------------------------------- */
	flagInfo = RTF_FindFlagByEntity(ent);

	if (!flagInfo) {
		return 0; /* not a tracked base entity */
	}

	if (flagInfo->isAtBase) {
		/* -------------------------------------------------------
		   Case B-Occupied: pole has its own flag on it.
		   The only valid action is capturing the enemy flag here.
		   The player keeps any own flag they are also carrying.
		   ------------------------------------------------------- */
		if (!cl->ps.powerups[enemy_flag]) {
			return 0; /* nothing useful at a full pole without enemy flag */
		}

		{
			team_t enemy_team;
			enemy_team = OtherTeam(team);

			carriedEnemyFlag = RTF_FindCarriedFlagByPlayer(other, enemy_team);
			if (!carriedEnemyFlag) {
				G_Printf("RTF WARNING: enemy flag powerup without tracking entry for player %d\n",
				         other->s.number);
				return 0;
			}
			enemyFlagEnt = carriedEnemyFlag->ent;

			PrintMsg( NULL, "%s" S_COLOR_WHITE " captured the %s flag!\n",
			          cl->pers.netname, TeamName(enemy_team));

			cl->ps.powerups[enemy_flag] = 0;

			RTF_MarkFlagCaptured(enemyFlagEnt);

			{
				rtf_team_state_t *estate;
				estate = (enemy_team == TEAM_RED) ? &rtf_redState : &rtf_blueState;
				if (estate->flagsAtBase == estate->numFlags) {
					Team_SetFlagStatus(enemy_team, FLAG_ATBASE);
				}
			}

			teamgame.last_flag_capture = level.time;
			teamgame.last_capture_team = team;
			AddTeamScore(ent->s.pos.trBase, other->client->sess.sessionTeam, 1);
			Team_ForceGesture(other->client->sess.sessionTeam);
			other->client->pers.teamState.captures++;
			other->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP);
			other->client->ps.eFlags |= EF_AWARD_CAP;
			other->client->rewardTime = level.time + REWARD_SPRITE_TIME;
			other->client->ps.persistant[PERS_CAPTURES]++;
			AddScore(other, ent->r.currentOrigin, CTF_CAPTURE_BONUS);
			Team_CaptureFlagSound(ent, team);
			CalculateRanks();
		}
		return 0;
	}

	/* -------------------------------------------------------
	   Case B-Empty: pole is empty (flag was taken or dropped elsewhere).
	   ------------------------------------------------------- */

	/* --- Sub-case B1: player carries their own flag → return it here --- */
	if ( cl->ps.powerups[own_flag] ) {
		PrintMsg( NULL, "%s" S_COLOR_WHITE " returned the %s flag!\n",
		          cl->pers.netname, TeamName(team));

		cl->ps.powerups[own_flag] = 0;
		AddScore(other, ent->r.currentOrigin, CTF_RECOVERY_BONUS);
		other->client->pers.teamState.flagrecovery++;
		other->client->pers.teamState.lastreturnedflag = level.time;

		RTF_MarkFlagReturned(ent);

		/* Update the flag-status configstring only if needed. */
		{
			rtf_team_state_t *state;
			state = (team == TEAM_RED) ? &rtf_redState : &rtf_blueState;
			if (state->flagsAtBase == state->numFlags) {
				Team_SetFlagStatus(team, FLAG_ATBASE);
			}
		}

		Team_ReturnFlagSound(ent, team);

		/* --- Sub-case B1a: if player ALSO has enemy flag, capture it now --- */
		if ( cl->ps.powerups[enemy_flag] ) {
			team_t enemy_team;
			enemy_team = OtherTeam(team);

			carriedEnemyFlag = RTF_FindCarriedFlagByPlayer(other, enemy_team);
			if (carriedEnemyFlag) {
				enemyFlagEnt = carriedEnemyFlag->ent;

				PrintMsg( NULL, "%s" S_COLOR_WHITE " captured the %s flag!\n",
				          cl->pers.netname, TeamName(enemy_team));

				cl->ps.powerups[enemy_flag] = 0;

				RTF_MarkFlagCaptured(enemyFlagEnt);

				/* Only set FLAG_ATBASE for the enemy team if all their flags
				   are back; otherwise leave status as FLAG_TAKEN. */
				{
					rtf_team_state_t *estate;
					estate = (enemy_team == TEAM_RED) ? &rtf_redState : &rtf_blueState;
					if (estate->flagsAtBase == estate->numFlags) {
						Team_SetFlagStatus(enemy_team, FLAG_ATBASE);
					}
				}

				teamgame.last_flag_capture = level.time;
				teamgame.last_capture_team = team;
				AddTeamScore(ent->s.pos.trBase, other->client->sess.sessionTeam, 1);
				Team_ForceGesture(other->client->sess.sessionTeam);
				other->client->pers.teamState.captures++;
				other->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP);
				other->client->ps.eFlags |= EF_AWARD_CAP;
				other->client->rewardTime = level.time + REWARD_SPRITE_TIME;
				other->client->ps.persistant[PERS_CAPTURES]++;
				AddScore(other, ent->r.currentOrigin, CTF_CAPTURE_BONUS);
				Team_CaptureFlagSound(ent, team);
				CalculateRanks();
			}
		}
		return 0;
	}

	/* Player has neither flag, or has only the enemy flag without their own
	   to fill the pole — nothing to do at this empty pole. */
	return 0;
}

/*
==============
Team_TouchEnemyFlag_RTF - RTF mode handler

Called when a player touches an enemy flag entity.  The entity may be
the base entity (first pickup) or a dropped-flag entity (relay pickup).

Rules enforced:
  • A player cannot carry two flags of the same team.
  • Only one enemy flag may be carried at a time (powerup already enforces this).
==============
*/
static int Team_TouchEnemyFlag_RTF( gentity_t *ent, gentity_t *other, team_t team ) {
	gclient_t *cl;
	int flag_pw;
	rtf_team_state_t *state;
	int i;

	cl = other->client;
	flag_pw = (team == TEAM_RED) ? PW_REDFLAG : PW_BLUEFLAG;

	/* A player can carry at most one flag of this team. */
	if ( cl->ps.powerups[flag_pw] ) {
		return 0;
	}

	PrintMsg( NULL, "%s" S_COLOR_WHITE " got the %s flag!\n",
	          cl->pers.netname, TeamName(team));

	cl->ps.powerups[flag_pw] = INT_MAX;
	cl->pers.teamState.flagsince = level.time;

	if ( ent->flags & FL_DROPPED_ITEM ) {
		/* Dropped enemy flag: entity is NOT in tracking table.
		   Find the unoccupied base slot for this team's flag and mark it
		   carried by this player. */
		state = (team == TEAM_RED) ? &rtf_redState : &rtf_blueState;
		for (i = 0; i < state->numFlags; i++) {
			if (!state->flags[i].isAtBase && !state->flags[i].isCarried) {
				state->flags[i].isCarried = qtrue;
				state->flags[i].carrier   = other->s.number;
				state->flags[i].takenTime = level.time;
				if (state->flagsCarried < state->numFlags) {
					state->flagsCarried++;
				}
				break;
			}
		}
		Team_SetFlagStatus(team, FLAG_TAKEN);
		Team_TakeFlagSound(ent, team);
		return -1; /* Remove the dropped entity. */
	}

	/* Base entity pickup: use RTF_MarkFlagTaken which hides the base entity
	   and updates tracking correctly. */
	RTF_MarkFlagTaken(ent, other);
	Team_SetFlagStatus(team, FLAG_TAKEN);
	Team_TakeFlagSound(ent, team);

	return -1; /* Returning -1 tells Pickup_Team to "delete" this entity, but
	              for a base entity Touch_Item handles visibility; we already
	              hid it in RTF_MarkFlagTaken so returning 0 vs -1 both work.
	              Use -1 to match normal CTF behaviour and ensure freeAfterEvent
	              is NOT set for the base entity (it's handled separately). */
}

/*
==============
Team_TouchOurFlag - Original CTF handler
==============
*/
static int Team_TouchOurFlag_CTF( gentity_t *ent, gentity_t *other, team_t team ) {
	int			i;
	gentity_t	*player;
	gclient_t	*cl = other->client;
	int			enemy_flag;

	if (cl->sess.sessionTeam == TEAM_RED) {
		enemy_flag = PW_BLUEFLAG;
	} else {
		enemy_flag = PW_REDFLAG;
	}

	if ( ent->flags & FL_DROPPED_ITEM ) {
		PrintMsg( NULL, "%s" S_COLOR_WHITE " returned the %s flag!\n",
			cl->pers.netname, TeamName(team));
		AddScore(other, ent->r.currentOrigin, CTF_RECOVERY_BONUS);
		other->client->pers.teamState.flagrecovery++;
		other->client->pers.teamState.lastreturnedflag = level.time;
		Team_ReturnFlagSound(Team_ResetFlag(team), team);
		return 0;
	}

	if (!cl->ps.powerups[enemy_flag])
		return 0;

	PrintMsg( NULL, "%s" S_COLOR_WHITE " captured the %s flag!\n",
	          cl->pers.netname, TeamName(OtherTeam(team)));

	cl->ps.powerups[enemy_flag] = 0;

	teamgame.last_flag_capture = level.time;
	teamgame.last_capture_team = team;

	AddTeamScore(ent->s.pos.trBase, other->client->sess.sessionTeam, 1);
	Team_ForceGesture(other->client->sess.sessionTeam);

	other->client->pers.teamState.captures++;
	other->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
	other->client->ps.eFlags |= EF_AWARD_CAP;
	other->client->rewardTime = level.time + REWARD_SPRITE_TIME;
	other->client->ps.persistant[PERS_CAPTURES]++;
	AddScore(other, ent->r.currentOrigin, CTF_CAPTURE_BONUS);

	Team_CaptureFlagSound( ent, team );

	for (i = 0; i < level.maxclients; i++) {
		player = &g_entities[i];
		if (!player->inuse || player == other)
			continue;
		if (player->client->sess.sessionTeam != cl->sess.sessionTeam) {
			player->client->pers.teamState.lasthurtcarrier = -5;
		} else {
			if (player->client->pers.teamState.lastreturnedflag +
				CTF_RETURN_FLAG_ASSIST_TIMEOUT > level.time) {
				AddScore(player, ent->r.currentOrigin, CTF_RETURN_FLAG_ASSIST_BONUS);
				other->client->pers.teamState.assists++;
				player->client->ps.persistant[PERS_ASSIST_COUNT]++;
				player->client->ps.eFlags |= EF_AWARD_ASSIST;
				player->client->rewardTime = level.time + REWARD_SPRITE_TIME;
			}
			if (player->client->pers.teamState.lastfraggedcarrier +
				CTF_FRAG_CARRIER_ASSIST_TIMEOUT > level.time) {
				AddScore(player, ent->r.currentOrigin, CTF_FRAG_CARRIER_ASSIST_BONUS);
				other->client->pers.teamState.assists++;
				player->client->ps.persistant[PERS_ASSIST_COUNT]++;
				player->client->ps.eFlags |= EF_AWARD_ASSIST;
				player->client->rewardTime = level.time + REWARD_SPRITE_TIME;
			}
		}
	}

	Team_ResetFlags();
	CalculateRanks();

	return 0;
}

/*
==============
Team_TouchOurFlag - Dispatcher
==============
*/
static int Team_TouchOurFlag( gentity_t *ent, gentity_t *other, team_t team ) {
	if ( g_gametype.integer == GT_RTF ) {
		return Team_TouchOurFlag_RTF(ent, other, team);
	}
	return Team_TouchOurFlag_CTF(ent, other, team);
}

/*
==============
Team_TouchEnemyFlag - Dispatcher
==============
*/
static int Team_TouchEnemyFlag( gentity_t *ent, gentity_t *other, team_t team ) {
	gclient_t *cl = other->client;

#ifdef MISSIONPACK
	if( g_gametype.integer == GT_1FCTF ) {
		PrintMsg (NULL, "%s" S_COLOR_WHITE " got the flag!\n", other->client->pers.netname );
		cl->ps.powerups[PW_NEUTRALFLAG] = INT_MAX;
		if( team == TEAM_RED ) {
			Team_SetFlagStatus( TEAM_FREE, FLAG_TAKEN_RED );
		} else {
			Team_SetFlagStatus( TEAM_FREE, FLAG_TAKEN_BLUE );
		}
		AddScore(other, ent->r.currentOrigin, CTF_FLAG_BONUS);
		cl->pers.teamState.flagsince = level.time;
		Team_TakeFlagSound( ent, team );
		return -1;
	}
#endif

	if ( g_gametype.integer == GT_RTF ) {
		return Team_TouchEnemyFlag_RTF(ent, other, team);
	}

	// Standard CTF
	if (team == TEAM_RED && cl->ps.powerups[PW_REDFLAG])
		return 0;
	if (team == TEAM_BLUE && cl->ps.powerups[PW_BLUEFLAG])
		return 0;

	PrintMsg (NULL, "%s" S_COLOR_WHITE " got the %s flag!\n",
		other->client->pers.netname, TeamName(team));

	if (team == TEAM_RED)
		cl->ps.powerups[PW_REDFLAG] = INT_MAX;
	else
		cl->ps.powerups[PW_BLUEFLAG] = INT_MAX;

	Team_SetFlagStatus( team, FLAG_TAKEN );
	AddScore(other, ent->r.currentOrigin, CTF_FLAG_BONUS);
	cl->pers.teamState.flagsince = level.time;
	Team_TakeFlagSound( ent, team );

	return -1;
}

int Pickup_Team( gentity_t *ent, gentity_t *other ) {
	int team;
	gclient_t *cl = other->client;

#ifdef MISSIONPACK
	if( g_gametype.integer == GT_OBELISK ) {
		G_FreeEntity( ent );
		return 0;
	}
	if( g_gametype.integer == GT_HARVESTER ) {
		if( ent->spawnflags != cl->sess.sessionTeam ) {
			cl->ps.generic1 += 1;
		}
		G_FreeEntity( ent );
		return 0;
	}
#endif

	if( strcmp(ent->classname, "team_CTF_redflag") == 0 ) {
		team = TEAM_RED;
	}
	else if( strcmp(ent->classname, "team_CTF_blueflag") == 0 ) {
		team = TEAM_BLUE;
	}
#ifdef MISSIONPACK
	else if( strcmp(ent->classname, "team_CTF_neutralflag") == 0  ) {
		team = TEAM_FREE;
	}
#endif
	else {
		PrintMsg ( other, "Don't know what team the flag is on.\n");
		return 0;
	}

#ifdef MISSIONPACK
	if( g_gametype.integer == GT_1FCTF ) {
		if( team == TEAM_FREE ) {
			return Team_TouchEnemyFlag( ent, other, cl->sess.sessionTeam );
		}
		if( team != cl->sess.sessionTeam) {
			return Team_TouchOurFlag( ent, other, cl->sess.sessionTeam );
		}
		return 0;
	}
#endif

	if( team == cl->sess.sessionTeam) {
		return Team_TouchOurFlag( ent, other, team );
	}
	return Team_TouchEnemyFlag( ent, other, team );
}

/*
===========
Team_GetLocation
============
*/
gentity_t *Team_GetLocation(gentity_t *ent)
{
	gentity_t		*eloc, *best;
	float			bestlen, len;
	vec3_t			origin;

	best = NULL;
	bestlen = 3*8192.0*8192.0;

//qlone - freezetag
	if ( g_freezeTag.integer && ent->freezeState && is_body( ent->target_ent ) )
		VectorCopy( ent->target_ent->r.currentOrigin, origin );
	else
//qlone - freezetag
	VectorCopy( ent->r.currentOrigin, origin );

	for (eloc = level.locationHead; eloc; eloc = eloc->nextTrain) {
		len = ( origin[0] - eloc->r.currentOrigin[0] ) * ( origin[0] - eloc->r.currentOrigin[0] )
			+ ( origin[1] - eloc->r.currentOrigin[1] ) * ( origin[1] - eloc->r.currentOrigin[1] )
			+ ( origin[2] - eloc->r.currentOrigin[2] ) * ( origin[2] - eloc->r.currentOrigin[2] );

		if ( len > bestlen ) {
			continue;
		}

		if ( !trap_InPVS( origin, eloc->r.currentOrigin ) ) {
			continue;
		}

		bestlen = len;
		best = eloc;
	}

	return best;
}

qboolean Team_GetLocationMsg(gentity_t *ent, char *loc, int loclen)
{
	gentity_t *best;

	best = Team_GetLocation( ent );

	if (!best)
		return qfalse;

	if (best->count) {
		if (best->count < 0)
			best->count = 0;
		if (best->count > 7)
			best->count = 7;
		Com_sprintf(loc, loclen, "%c%c%s" S_COLOR_WHITE, Q_COLOR_ESCAPE, best->count + '0', best->message );
	} else
		Com_sprintf(loc, loclen, "%s", best->message);

	return qtrue;
}

/*---------------------------------------------------------------------------*/

#define	MAX_TEAM_SPAWN_POINTS	32

gentity_t *SelectRandomTeamSpawnPoint( gentity_t *ent, int teamstate, team_t team ) {
	gentity_t	*spot;
	int			selection;
	gentity_t	*spots[ MAX_TEAM_SPAWN_POINTS ];
	int			numSpots;
	int			checkMask;
	int			n;
	qboolean	checkState;
	qboolean	checkTelefrag;

	if ( team != TEAM_RED && team != TEAM_BLUE )
		return NULL;

	checkMask = 3;

__rescan:

	checkTelefrag = checkMask & 1;
	checkState = checkMask & 2;
	numSpots = 0;

	for ( n = 0 ; n < level.numSpawnSpots ; n++ ) {
		spot = level.spawnSpots[ n ];
		if ( spot->fteam != team )
			continue;
		if ( checkTelefrag && SpotWouldTelefrag( spot ) )
			continue;
		if ( checkState ) {
			if ( teamstate == TEAM_BEGIN ) {
				if ( spot->count != 0 )
					continue;
			} else {
				if ( spot->count == 0 )
					continue;
			}
		}
		spots[ numSpots++ ] = spot;
		if ( numSpots >= MAX_TEAM_SPAWN_POINTS )
			break;
	}

	if ( !numSpots ) {
		if ( checkMask <= 0 ) {
			return NULL;
		}
		checkMask--;
		goto __rescan;
	}

	selection = rand() % numSpots;
	return spots[ selection ];
}

gentity_t *SelectFreezeSpawnPoint ( gentity_t *ent, team_t team, int teamstate, vec3_t origin, vec3_t angles ) {
    gentity_t	*spot;

    spot = SelectFarFromEnemyTeam( team, origin, angles );

    if (!spot) {
        spot = SelectRandomTeamSpawnPoint ( ent, teamstate, team );
    }

    if (!spot) {
        return SelectSpawnPoint( NULL, vec3_origin, origin, angles );
    }

    VectorCopy (spot->s.origin, origin);
    origin[2] += 9;
    VectorCopy (spot->s.angles, angles);

    return spot;
}

gentity_t *SelectCTFSpawnPoint( gentity_t *ent, team_t team, int teamstate, vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;

	spot = SelectRandomTeamSpawnPoint( ent, teamstate, team );

	if ( !spot ) {
		return SelectSpawnPoint( ent, vec3_origin, origin, angles );
	}

	VectorCopy( spot->s.origin, origin );
	VectorCopy( spot->s.angles, angles );
	origin[2] += 9.0f;

	return spot;
}

/*---------------------------------------------------------------------------*/

static int QDECL SortClients( const void *a, const void *b ) {
	return *(int *)a - *(int *)b;
}

void TeamplayInfoMessage( gentity_t *ent ) {
	char		entry[ 128 ];
	char		string[ MAX_STRING_CHARS - 9 ];
	int			stringlength;
	int			i, j;
	gentity_t	*player;
	int			cnt;
	int			h, a;
	int			clients[TEAM_MAXOVERLAY];

	if ( !ent->client->pers.teamInfo )
		return;

	for (i = 0, cnt = 0; i < level.maxclients && cnt < TEAM_MAXOVERLAY; i++) {
		player = g_entities + level.sortedClients[i];
		if (player->inuse && player->client->sess.sessionTeam ==
			ent->client->sess.sessionTeam ) {
			clients[cnt++] = level.sortedClients[i];
		}
	}

	qsort( clients, cnt, sizeof( clients[0] ), SortClients );

	string[0] = '\0';
	stringlength = 0;

	for (i = 0, cnt = 0; i < level.maxclients && cnt < TEAM_MAXOVERLAY; i++) {
		player = g_entities + i;
		if ( player->inuse && player->client->sess.sessionTeam ==
			ent->client->sess.sessionTeam ) {

			h = player->client->ps.stats[STAT_HEALTH];
			a = player->client->ps.stats[STAT_ARMOR];
			if (h < 0) h = 0;
			if (a < 0) a = 0;
//qlone - freezetag
			if ( g_freezeTag.integer && player->freezeState )
				h = a = 0;
//qlone - freezetag

			j = BG_sprintf( entry, " %i %i %i %i %i %i",
				i, player->client->pers.teamState.location, h, a,
				player->client->ps.weapon, player->s.powerups);
			if ( stringlength + j >= sizeof( string ) )
				break;
			strcpy( string + stringlength, entry );
			stringlength += j;
			cnt++;
		}
	}

	trap_SendServerCommand( ent-g_entities, va( "tinfo %i %s", cnt, string ) );
}

void CheckTeamStatus( void ) {
	int i;
	gentity_t *loc, *ent;

	if (level.time - level.lastTeamLocationTime > TEAM_LOCATION_UPDATE_TIME) {

		level.lastTeamLocationTime = level.time;

		for (i = 0; i < level.maxclients; i++) {
			ent = g_entities + i;

			if ( ent->client->pers.connected != CON_CONNECTED ) {
				continue;
			}

			if (ent->inuse && (ent->client->sess.sessionTeam == TEAM_RED ||	ent->client->sess.sessionTeam == TEAM_BLUE)) {
				loc = Team_GetLocation( ent );
				if (loc)
					ent->client->pers.teamState.location = loc->health;
				else
					ent->client->pers.teamState.location = 0;
			}
		}

		for (i = 0; i < level.maxclients; i++) {
			ent = g_entities + i;

			if ( ent->client->pers.connected != CON_CONNECTED ) {
				continue;
			}

			if (ent->inuse && (ent->client->sess.sessionTeam == TEAM_RED ||	ent->client->sess.sessionTeam == TEAM_BLUE)) {
				TeamplayInfoMessage( ent );
			}
		}
	}
}

/*-----------------------------------------------------------------*/

/*QUAKED team_CTF_redplayer (1 0 0) (-16 -16 -16) (16 16 32) */
void SP_team_CTF_redplayer( gentity_t *ent ) {
//qlone - freezetag
	if ( g_freezeTag.integer && g_gametype.integer == GT_TEAM )
		ent->classname = "info_player_deathmatch";
//qlone - freezetag
}

/*QUAKED team_CTF_blueplayer (0 0 1) (-16 -16 -16) (16 16 32) */
void SP_team_CTF_blueplayer( gentity_t *ent ) {
//qlone - freezetag
	if ( g_freezeTag.integer && g_gametype.integer == GT_TEAM )
		ent->classname = "info_player_deathmatch";
//qlone - freezetag
}

/*QUAKED team_CTF_redspawn (1 0 0) (-16 -16 -24) (16 16 32) */
void SP_team_CTF_redspawn(gentity_t *ent) {
}

/*QUAKED team_CTF_bluespawn (0 0 1) (-16 -16 -24) (16 16 32) */
void SP_team_CTF_bluespawn(gentity_t *ent) {
}

#ifdef MISSIONPACK
/* (Missionpack code remains unchanged) */
#endif