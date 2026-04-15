#include "g_local.h"

int	check_time;
static vec3_t	redflag;
static vec3_t	blueflag;

qboolean is_spectator( gclient_t *client ) {
	if ( client == NULL ) return qfalse;
	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) return qtrue;
	if ( client->ps.persistant[ PERS_TEAM ] == TEAM_SPECTATOR ) return qtrue;
	if ( client->sess.spectatorState == SPECTATOR_FOLLOW ) return qtrue;
	return qfalse;
}

qboolean Set_spectator( gentity_t *ent ) {
	vec3_t	origin, angles;

	if ( level.intermissiontime ) return qfalse;
	if ( !ent->freezeState ) return qfalse;
	if ( ent->r.svFlags & SVF_BOT ) {
		ent->client->respawnTime = INT_MAX;
	} else if ( !is_spectator( ent->client ) ) {
		VectorCopy( ent->r.currentOrigin, origin );
		angles[ YAW ] = ent->client->ps.stats[ STAT_DEAD_YAW ];
		angles[ PITCH ] = 0;
		angles[ ROLL ] = 0;
		ClientSpawn( ent );
		VectorCopy( origin, ent->client->ps.origin );
		SetClientViewAngle( ent, angles );
		ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;
		ent->client->sess.spectatorTime = level.time;
		ent->client->sess.spectatorState = SPECTATOR_FREE;
		ent->client->sess.spectatorClient = 0;

		//G_LogPrintf("CALL: UpdateSpectatorLastPlayerState from Set_spectator\n");
		UpdateSpectatorLastPlayerState(ent);

		trap_UnlinkEntity( ent );
	}
	return qtrue;
}

qboolean Set_Client( gentity_t *ent ) {
	gclient_t	*cl;
	gentity_t	*tent;

	cl = ent->client;
	if ( cl->ps.pm_type != PM_SPECTATOR ) return qfalse;
	if ( cl->sess.sessionTeam == TEAM_SPECTATOR ) return qfalse;
	if ( ent->freezeState ) return qfalse;

	cl->sess.spectatorState = SPECTATOR_NOT;
	cl->sess.spectatorClient = 0;
	ClientSpawn( ent );

	tent = G_TempEntity( cl->ps.origin, EV_PLAYER_TELEPORT_IN );
	tent->s.clientNum = ent->s.clientNum;

	return qtrue;
}

void respawnSpectator( gentity_t *ent ) {
	gclient_t	*client;

	client = ent->client;
	if ( ent->freezeState ) return;
	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) return;

	if ( level.time > client->respawnTime ) {
		if ( g_forcerespawn.integer > 0 && level.time - client->respawnTime > g_forcerespawn.integer * 1000 ) {
			Cmd_FollowCycle_f( ent, 1 );
		}
	}
}

void Persistant_spectator( gentity_t *ent, gclient_t *cl ) {
	int	i;
	int	persistant[ MAX_PERSISTANT ];
	int	savedPing;

	savedPing = ent->client->ps.ping;
	for ( i = 0; i < MAX_PERSISTANT; i++ ) {
		persistant[ i ] = ent->client->ps.persistant[ i ];
	}
	ent->client->ps = cl->ps;
	ent->client->ps.ping = savedPing;
	for ( i = 0; i < MAX_PERSISTANT; i++ ) {
		switch ( i ) {
		case PERS_HITS:
		case PERS_TEAM:
		case PERS_ATTACKER:
			continue;
		}
		ent->client->ps.persistant[ i ] = persistant[ i ];
	}
}

static void FollowClient( gentity_t *ent, gentity_t *other ) {
	// Check if g_specLock is enabled and the attacker is on the opposing team
    if ( g_specLock.integer && ent->target_ent->client->sess.sessionTeam != other->client->sess.sessionTeam ) {
	   return; // Do not allow spectating the attacker
	}

	if ( ent->target_ent == other ) return;
	if ( is_spectator( ent->target_ent->client ) ) {
		ent->target_ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		ent->target_ent->client->sess.spectatorClient = other->s.number;
	}
}

static void player_free( gentity_t *ent ) {
	gentity_t *event;

	if ( !ent || !ent->inuse ) return;
	if ( !ent->freezeState ) return;

	// Reset freeze state
	ent->freezeState = qfalse;
	ent->client->respawnTime = level.time + 1700;

	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		StopFollowing( ent, qtrue );
		ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		ent->client->ps.pm_time = 100;
	}
	ent->client->inactivityTime = level.time + g_inactivity.integer * 1000;
}

void Body_free( gentity_t *self ) {
	if ( self->freezeState ) {
		self->wasFrozen = qfalse;
		player_free( self->target_ent );
	}
#ifdef MISSIONPACK
	int	i;
	gentity_t	*ent;

	if ( self->s.eFlags & EF_KAMIKAZE ) {
		for ( i = 0; i < MAX_GENTITIES; i++ ) {
			ent = &g_entities[ i ];
			if ( !ent->inuse ) continue;
			if ( ent->activator != self ) continue;
			if ( strcmp( ent->classname, "kamikaze timer" ) ) continue;
			G_FreeEntity( ent );
			break;
		}
	}
#endif
	self->s.powerups = 0;
	G_FreeEntity( self );
}

static void Body_Explode( gentity_t *self ) {
	int	i;
	gentity_t	*e, *tent;
	vec3_t	point;

	for ( i = 0; i < g_maxclients.integer; i++ ) {
		e = g_entities + i;
		if ( !e->inuse ) continue;
		if ( e->health < 1 ) continue;
		if ( e->client->sess.sessionTeam != self->spawnflags ) continue;
		VectorSubtract( self->s.pos.trBase, e->s.pos.trBase, point );
		if ( VectorLength( point ) > g_thawRadius.integer ) continue;
		if ( is_spectator( e->client ) ) continue;
		if ( !self->count ) {
			if ( g_dmflags.integer & 1024 || g_gametype.integer == GT_CTF ) {
				self->count = level.time + g_thawTime.value * 1000;
			} else {
				self->count = level.time + g_thawTime.value * 1000;
			}
			G_Sound( self, CHAN_AUTO, self->noise_index );

			self->activator = e;

			// Set the thaw time on the frozen player so the client can display it
			if (self->target_ent && self->target_ent->client) {
				self->target_ent->client->ps.stats[STAT_THAW_TIME] = self->count;
			}

		} else if ( self->count < level.time ) {
			if ( self->activator == e ) {
			} else if ( !self->activator->inuse ) {
			} else if ( self->activator->health < 1 ) {
			} else {
				VectorSubtract( self->s.pos.trBase, self->activator->s.pos.trBase, point );
				if ( VectorLength( point ) > g_thawRadius.integer ) {
				} else if ( is_spectator( self->activator->client ) ) {
				} else {
					e = self->activator;
				}
			}

			tent = G_TempEntity( self->target_ent->r.currentOrigin, EV_OBITUARY );
			tent->s.eventParm = MOD_UNKNOWN;
			tent->s.otherEntityNum = self->target_ent->s.number;
			tent->s.otherEntityNum2 = e->s.number;
			tent->r.svFlags = SVF_BROADCAST;

			//G_LogPrintf( "Thaw: %i %i %i: %s thawed %s by %s\n", e->s.number, self->target_ent->s.number, MOD_UNKNOWN, e->client->pers.netname, self->target_ent->client->pers.netname, "MOD_UNKNOWN" );
			e->client->pers.stats.thaws++;
			AddScore( e, self->s.pos.trBase, 2 );
			//G_LogPrintf("CALL: CheckLastPlayerAlive from Body_Explode\n");
			CheckLastPlayerAlive( e->client->sess.sessionTeam );

			// Clear the thaw time since thawing is complete
			if (self->target_ent && self->target_ent->client) {
				self->target_ent->client->ps.stats[STAT_THAW_TIME] = 0;
			}

			G_Damage( self, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
		}
		return;
	}
	// Clear thaw time if thawing was interrupted
	if (self->target_ent && self->target_ent->client) {
		self->target_ent->client->ps.stats[STAT_THAW_TIME] = 0;
	}
	self->count = 0;
}

static void Body_WorldEffects( gentity_t *self ) {
	vec3_t	point;
	int	contents;
	int	i, num;
	int	touch[ MAX_GENTITIES ];
	gentity_t	*hit;
	vec3_t	mins, maxs;
	int	previous_waterlevel;
	gentity_t	*event;

	VectorCopy( self->r.currentOrigin, point );
	point[ 2 ] -= 23;

	contents = trap_PointContents( point, -1 );
	if ( contents & ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NODROP ) ) {
		event = G_TempEntity(self->r.currentOrigin, EV_FREEZE_TIME);
		self->freezeTime = level.time + (g_lavaThawTime.integer * 1000);
		event->s.time = self->freezeTime; // Store the freezeTime value in the event
		event->r.svFlags |= SVF_SINGLECLIENT; // Send the event only to the specific client
		event->r.singleClient = self->target_ent->s.clientNum;
		event->s.eventParm = self->target_ent->s.clientNum;
		self->target_ent->count = self->freezeTime;
		self->target_ent->think = Body_free;
		self->target_ent->nextthink = self->target_ent->count;
		self->target_ent->client->freezeEvent = event;

		if ( level.time - self->timestamp > 5000 ) {
			G_Damage( self, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
		}

		return;
	}
	if ( self->s.pos.trType == TR_STATIONARY && contents & CONTENTS_NODROP ) {
        self->s.pos.trType == TR_GRAVITY;
		// if ( level.time - self->timestamp > 5000 ) {
		// 	Body_free( self );
		// }
		event = G_TempEntity(self->r.currentOrigin, EV_FREEZE_TIME);
		self->freezeTime = level.time + (g_lavaThawTime.integer * 1000);
		event->s.time = self->freezeTime; // Store the freezeTime value in the event
		event->r.svFlags |= SVF_SINGLECLIENT; // Send the event only to the specific client
		event->r.singleClient = self->target_ent->s.clientNum;
		event->s.eventParm = self->target_ent->s.clientNum;
		self->target_ent->count = self->freezeTime;
		self->target_ent->think = Body_free;
		self->target_ent->nextthink = self->target_ent->count;
		self->target_ent->client->freezeEvent = event;

		return;
	}

	previous_waterlevel = self->waterlevel;
	self->waterlevel = 0;
	if ( contents & MASK_WATER ) {
		self->waterlevel = 3;
	}
	self->watertype = contents;
	if ( !previous_waterlevel && self->waterlevel ) {
		G_AddEvent( self, EV_WATER_TOUCH, 0 );
	}
	if ( previous_waterlevel && !self->waterlevel ) {
		G_AddEvent( self, EV_WATER_LEAVE, 0 );
	}

	VectorAdd( self->r.currentOrigin, self->r.mins, mins );
	VectorAdd( self->r.currentOrigin, self->r.maxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for ( i = 0; i < num; i++ ) {
		hit = &g_entities[ touch[ i ] ];
		if ( !hit->touch ) {
			continue;
		}
		switch ( hit->s.eType ) {
		case ET_PUSH_TRIGGER:
			if ( self->s.pos.trDelta[ 2 ] < 100 ) {
				G_Sound( self, CHAN_AUTO, G_SoundIndex( "sound/world/jumppad.wav" ) );
			}
			VectorCopy( hit->s.origin2, self->s.pos.trDelta );

			self->s.pos.trType = TR_GRAVITY;
			self->s.pos.trTime = level.time;
			break;
		case ET_TELEPORT_TRIGGER:
			if ( !( hit->spawnflags & 1 ) && g_teleporterThaws.integer ) {
				G_TempEntity( self->r.currentOrigin, EV_PLAYER_TELEPORT_OUT );
				Body_free( self );
				return;
			}
			break;
		}
	}
}

void Kamikaze_DeathTimer( gentity_t *self );

static void TossBody( gentity_t *self ) {
	int	anim;
	static int	n;

	self->timestamp = level.time;
	self->nextthink = level.time + 5000;
#ifdef MISSIONPACK
	if ( self->s.eFlags & EF_KAMIKAZE ) {
		Kamikaze_DeathTimer( self );
	}
#endif
	self->s.eFlags |= EF_DEAD;
	self->s.powerups = 0;
	self->r.maxs[ 2 ] = -8;
	self->r.contents = CONTENTS_CORPSE;
	self->freezeState = qfalse;
	self->s.weapon = WP_NONE;

	switch ( n ) {
	case 0:
		anim = BOTH_DEATH1;
		break;
	case 1:
		anim = BOTH_DEATH2;
		break;
	case 2:
	default:
		anim = BOTH_DEATH3;
		break;
	}
	n = ( n + 1 ) % 3;

	self->s.torsoAnim = self->s.legsAnim = anim;

	if ( !g_blood.integer ) {
		self->takedamage = qfalse;
	}

	trap_LinkEntity( self );
}

static void Body_think( gentity_t *self ) {
	self->nextthink = level.time + FRAMETIME;

	if ( !self->target_ent || !self->target_ent->client || !self->target_ent->inuse ) {
		Body_free( self );
		return;
	}
	if ( self->s.otherEntityNum != self->target_ent->s.number ) {
		Body_free( self );
		return;
	}
	if ( level.intermissiontime || level.intermissionQueued ) {
		return;
	}
	if ( level.time - self->timestamp >= ( g_autoThawTime.integer * 1000 ) ) {
		gentity_t	*tent;
		tent = G_TempEntity( self->r.currentOrigin, EV_GIB_PLAYER );
		if ( self->freezeState ) {
			tent->s.eventParm = 255;
		}
		player_free( self->target_ent );
		TossBody( self );
		return;
	}

    if (self->freezeState) {
        float friction = g_frozenFriction.value;
        float speed;

        // Handle different trajectory types
        if (self->s.pos.trType == TR_LINEAR) {
            // Check if we're still on solid ground
            vec3_t groundCheck;
            trace_t trace;
			int contents = trap_PointContents(self->r.currentOrigin, -1);

            VectorCopy(self->r.currentOrigin, groundCheck);
            groundCheck[2] -= 32; // Check 32 units below

            trap_Trace(&trace, self->r.currentOrigin, self->r.mins, self->r.maxs,
                       groundCheck, self->s.number, MASK_PLAYERSOLID);

            // If we're not on solid ground, switch to gravity
            if (trace.fraction >= 1.0f || trace.startsolid || (contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NODROP))) {
                //G_LogPrintf("DEBUG: Body falling into void, switching to gravity\n");
                self->s.pos.trType = TR_GRAVITY;
                self->s.pos.trTime = level.time;
                VectorCopy(self->r.currentOrigin, self->s.pos.trBase);

                // Give it some initial downward velocity to help gravity take effect
                if (self->s.pos.trDelta[2] > -50) {
                    self->s.pos.trDelta[2] = -50; // Initial falling velocity
                }

                // Clear ground entity so it falls properly
                self->s.groundEntityNum = ENTITYNUM_NONE;
                if (contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_NODROP)) {
                    Body_WorldEffects(self);
                }

            } else {
                // Still on ground, apply friction to sliding movement
                speed = VectorLength(self->s.pos.trDelta);

                if (speed > 0.1f) {
                    // Apply friction
                    self->s.pos.trDelta[0] *= friction;
                    self->s.pos.trDelta[1] *= friction;

                    // Update trajectory
                    VectorCopy(self->r.currentOrigin, self->s.pos.trBase);
                    self->s.pos.trTime = level.time;

                    // Check if we should stop
                    speed = VectorLength(self->s.pos.trDelta);
                    if (speed < 5.0f) {
                        VectorClear(self->s.pos.trDelta);
                        self->s.pos.trType = TR_STATIONARY;
                        //G_LogPrintf("DEBUG: Body stopped sliding, speed was %.2f\n", speed);
                    }
                } else {
                    // Already stopped
                    VectorClear(self->s.pos.trDelta);
                    self->s.pos.trType = TR_STATIONARY;
                }
            }
        } else if (self->s.pos.trType == TR_GRAVITY) {
            // Apply horizontal friction only to gravity-based movement
            self->s.pos.trDelta[0] *= friction;
            self->s.pos.trDelta[1] *= friction;

            // Check if we've landed and should switch back to sliding
            if (self->s.groundEntityNum != ENTITYNUM_NONE &&
                VectorLength(self->s.pos.trDelta) > 10.0f &&
                self->s.pos.trDelta[2] > -100) { // Not falling too fast

                // We've landed and still have horizontal momentum - switch back to sliding
                self->s.pos.trDelta[2] = 0; // Remove vertical velocity
                self->s.pos.trType = TR_LINEAR;
                self->s.pos.trTime = level.time;
                VectorCopy(self->r.currentOrigin, self->s.pos.trBase);
                //G_LogPrintf("DEBUG: Body landed, switching back to sliding\n");
            }
        }

        if (!self->target_ent->freezeState) {
            TossBody(self);
            return;
        }
        Body_Explode(self);
        if (self->last_move_time < level.time - 1000) {
            Body_WorldEffects(self);
            self->last_move_time = level.time;
        }
        return;
    }

    if ( level.time - self->timestamp > 6500 ) {
		Body_free( self );
	} else {
		self->s.pos.trBase[ 2 ] -= 1;
	}
}

static void Body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	gentity_t	*tent;

	if ( self->health > GIB_HEALTH ) {
		return;
	}

	if ( self->freezeState && !g_blood.integer ) {
		tent = G_TempEntity( self->r.currentOrigin, EV_GIB_PLAYER );
		if ( self->freezeState ) {
			tent->s.eventParm = 255;
		}
		player_free( self->target_ent );
		TossBody( self );
		return;
	}

	tent = G_TempEntity( self->r.currentOrigin, EV_GIB_PLAYER );
	if ( self->freezeState ) {
		tent->s.eventParm = 255;
	}
	Body_free( self );
}

qboolean DamageBody( gentity_t *targ, gentity_t *attacker, vec3_t dir, int mod, int knockback ) {
	static float	mass;
	vec3_t	kvel;

	if ( !mass ) {
		char	info[ 1024 ];
		static char	mapname[ 128 ];

		trap_GetServerinfo( info, sizeof ( info ) );
		strncpy( mapname, Info_ValueForKey( info, "mapname" ), sizeof ( mapname ) - 1 );
		mapname[ sizeof ( mapname ) - 1 ] = '\0';

		if ( !Q_stricmp( mapname, "q3tourney3" ) ||
			!Q_stricmp( mapname, "q3dm16" ) ||
			!Q_stricmp( mapname, "q3dm17" ) ||
			!Q_stricmp( mapname, "q3dm18" ) ||
			!Q_stricmp( mapname, "q3dm19" ) ||
			!Q_stricmp( mapname, "q3tourney6" ) ||
			!Q_stricmp( mapname, "q3ctf4" ) ||
			!Q_stricmp( mapname, "mpq3ctf4" ) ||
			!Q_stricmp( mapname, "mpq3tourney6" ) ||
			!Q_stricmp( mapname, "mpteam6" ) ) {
			mass = 300;
		} else {
			mass = 200;
		}
		if ( g_dmflags.integer & 1024 ) mass = 300;
	}

	if ( attacker->client && targ->freezeState ) {
		if ( knockback ) {
			if ( g_freezeKnockback.integer && mod != MOD_GRAPPLE ) {
				G_FrozenPlayerKnockback( targ, 1000, dir );
			}
			else {
				VectorScale( dir, g_knockback.value * (float) knockback / mass, kvel );
				if ( mass == 200 ) kvel[ 2 ] += 24;
				VectorAdd( targ->s.pos.trDelta, kvel, targ->s.pos.trDelta );

				targ->s.pos.trType = TR_GRAVITY;
				targ->s.pos.trTime = level.time;
			}

			targ->pain_debounce_time = level.time;
		}
		if ( mod == MOD_GAUNTLET || mod == MOD_RAILGUN ) {
			FollowClient( targ, attacker );
		}
		return qtrue;
	}
	return qfalse;
}

qboolean is_body( gentity_t *ent ) {
	if ( !ent || !ent->inuse ) return qfalse;
	return ( ent->classname && !Q_stricmp( ent->classname, "freezebody" ) );
}

qboolean is_body_freeze( gentity_t *ent ) {
	if ( is_body( ent ) ) {
		return ent->freezeState;
	}
	return qfalse;
}

#ifdef MISSIONPACK
void G_ExplodeMissile( gentity_t *ent );

static void ProximityMine_ExplodeOnBody( gentity_t *mine ) {
	gentity_t	*body;

	if ( !is_body_freeze( mine->enemy ) ) {
		mine->think = G_FreeEntity;
		mine->nextthink = level.time;
		return;
	}

	body = mine->enemy;
	body->s.eFlags &= ~EF_TICKING;

	body->s.loopSound = 0;

	G_SetOrigin( mine, body->s.pos.trBase );
	mine->r.svFlags &= ~SVF_NOCLIENT;
	mine->splashMethodOfDeath = MOD_PROXIMITY_MINE;
	G_ExplodeMissile( mine );
}

void ProximityMine_Body( gentity_t *mine, gentity_t *body ) {
	if ( mine->s.eFlags & EF_NODRAW )
		return;

	G_AddEvent( mine, EV_PROXIMITY_MINE_STICK, 0 );

	if ( body->s.eFlags & EF_TICKING ) {
		body->activator->splashDamage += mine->splashDamage;
		body->activator->splashRadius *= 1.50;
		mine->think = G_FreeEntity;
		mine->nextthink = level.time;
		return;
	}

	body->s.loopSound = G_SoundIndex( "sound/weapons/proxmine/wstbtick.wav" );

	body->s.eFlags |= EF_TICKING;
	body->activator = mine;

	mine->s.eFlags |= EF_NODRAW;
	mine->r.svFlags |= SVF_NOCLIENT;
	mine->s.pos.trType = TR_LINEAR;
	VectorClear( mine->s.pos.trDelta );

	mine->enemy = body;
	mine->think = ProximityMine_ExplodeOnBody;
	mine->nextthink = level.time + 10 * 1000;
}
#endif

static void CopyToBody( gentity_t *ent ) {
	gentity_t	*body;

	body = G_Spawn();
	body->classname = "freezebody";
	body->s = ent->s;
	body->s.eFlags = 0;
#ifdef MISSIONPACK
	if ( ent->s.eFlags & EF_KAMIKAZE ) {
		body->s.eFlags |= EF_KAMIKAZE;
	}
#endif
	body->s.powerups = 1 << PW_BATTLESUIT;
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;

	G_SetOrigin( body, ent->r.currentOrigin );
	body->s.pos.trType = TR_GRAVITY;
	body->s.pos.trTime = level.time;
	VectorCopy( ent->client->ps.velocity, body->s.pos.trDelta );
	body->s.event = 0;

	switch ( body->s.legsAnim & ~ANIM_TOGGLEBIT ) {
	case LEGS_WALKCR:
	case LEGS_WALK:
	case LEGS_RUN:
	case LEGS_BACK:
	case LEGS_SWIM:
	case LEGS_IDLE:
	case LEGS_IDLECR:
	case LEGS_TURN:
	case LEGS_BACKCR:
	case LEGS_BACKWALK:
		switch ( rand() % 4 ) {
		case 0:
			body->s.legsAnim = LEGS_JUMP;
			break;
		case 1:
			body->s.legsAnim = LEGS_LAND;
			break;
		case 2:
			body->s.legsAnim = LEGS_JUMPB;
			break;
		case 3:
			body->s.legsAnim = LEGS_LANDB;
			break;
		}
	}

	body->r.svFlags = ent->r.svFlags;
	VectorCopy( ent->r.mins, body->r.mins );
	VectorCopy( ent->r.maxs, body->r.maxs );
	VectorCopy( ent->r.absmin, body->r.absmin );
	VectorCopy( ent->r.absmax, body->r.absmax );

	body->clipmask = MASK_PLAYERSOLID;
	body->r.contents = CONTENTS_BODY;

	body->think = Body_think;
	body->nextthink = level.time + FRAMETIME;

	body->die = Body_die;
	body->takedamage = qtrue;

	body->target_ent = ent;
	ent->target_ent = body;
	body->s.otherEntityNum = ent->s.number;
	body->noise_index = G_SoundIndex( "sound/player/tankjr/jump1.wav" );
	body->freezeState = qtrue;
	body->spawnflags = ent->client->sess.sessionTeam;
	body->waterlevel = ent->waterlevel;
	body->count = 0;

	trap_LinkEntity( body );
}

static qboolean NearbyBody( gentity_t *targ ) {
	gentity_t	*spot;
	vec3_t	delta;

	if ( g_gametype.integer == GT_CTF || g_gametype.integer == GT_RTF ) {
		return qfalse;
	}

	spot = NULL;
	while ( ( spot = G_Find( spot, FOFS( classname ), "freezebody" ) ) != NULL ) {
		if ( !spot->freezeState ) continue;
		if ( spot->spawnflags != targ->client->sess.sessionTeam ) continue;
		VectorSubtract( spot->s.pos.trBase, targ->s.pos.trBase, delta );
		if ( VectorLength( delta ) > 100 ) continue;
		if ( level.time - spot->timestamp > 400 ) {
			return qtrue;
		}
	}
	return qfalse;
}

void player_freeze( gentity_t *self, gentity_t *attacker, int mod ) {
    gentity_t	*event;

	if ( level.warmupTime ) {
		return;
	}
	if ( g_gametype.integer != GT_TEAM && g_gametype.integer != GT_CTF && g_gametype.integer != GT_RTF ) {
		return;
	}

	if ( self != attacker && OnSameTeam( self, attacker ) ) {
		return;
	}

	//G_LogPrintf("FREEZE_DEBUG: player_freeze called - Player: %d (%s), Attacker: %d (%s), MOD: %d\n",
				//self->s.clientNum,
				//self->client ? self->client->pers.netname : "NULL_CLIENT",
				//attacker ? attacker->s.clientNum : -1,
				//(attacker && attacker->client) ? attacker->client->pers.netname : "NULL_ATTACKER",
				//mod);

	if ( self != attacker && ( g_gametype.integer == GT_CTF || g_gametype.integer == GT_RTF ) && redflag && blueflag ) {
		vec3_t	dist1, dist2;

		VectorSubtract( redflag, self->s.pos.trBase, dist1 );
		VectorSubtract( blueflag, self->s.pos.trBase, dist2 );

		if ( self->client->sess.sessionTeam == TEAM_RED ) {
			if ( VectorLength( dist1 ) < VectorLength( dist2 ) ) {
				return;
			}
		} else if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
			if ( VectorLength( dist2 ) < VectorLength( dist1 ) ) {
				return;
			}
		}
	}

	switch ( mod ) {
	case MOD_UNKNOWN:
	case MOD_WATER:
	case MOD_CRUSH:
	case MOD_TELEFRAG:
	//case MOD_FALLING:
	case MOD_SUICIDE:
	case MOD_TARGET_LASER:
	//case MOD_TRIGGER_HURT:
	// case MOD_LAVA:
	// case MOD_SLIME:
#ifdef MISSIONPACK
	case MOD_JUICED:
#endif
	//case MOD_GRAPPLE:
		return;
	}

	CopyToBody( self );
	self->r.maxs[ 2 ] = -8;
	self->freezeState = qtrue;
	self->wasFrozen = qtrue;
	self->lastState = qfalse;
	self->takedamage = qfalse;
	self->s.eType = ET_INVISIBLE;
	self->r.contents = 0;
	self->health = GIB_HEALTH;

	// Rule 6: drop any carried flags when frozen (same as on death).
	if ( g_gametype.integer == GT_CTF || g_gametype.integer == GT_RTF ) {
		gitem_t   *fl;
		int        j;
		for ( j = 1; j < PW_NUM_POWERUPS; j++ ) {
			if ( self->client->ps.powerups[j] > level.time ) {
				fl = BG_FindItemForPowerup( j );
				if ( !fl || fl->giType != IT_TEAM ) {
					continue;
				}
				Drop_Item( self, fl, 0 );
				self->client->ps.powerups[j] = 0;
			}
		}
	}

    // Create a temporary event entity to carry the freezeTime value
    ResetFreezeTimeEvent( self, self->s.clientNum );

    if(self->target_ent && self->client->sess.sessionTeam != TEAM_SPECTATOR) {
        if (mod == MOD_LAVA || mod == MOD_SLIME || mod == MOD_TRIGGER_HURT) {
            event = G_TempEntity(self->r.currentOrigin, EV_FREEZE_TIME);
            self->freezeTime = level.time + (g_lavaThawTime.integer * 1000);
            event->s.time = self->freezeTime; // Store the freezeTime value in the event
            event->r.svFlags |= SVF_SINGLECLIENT; // Send the event only to the specific client
            event->r.singleClient = self->s.clientNum;
            event->s.eventParm = self->s.clientNum;
            self->target_ent->count = self->freezeTime;
            self->target_ent->think = Body_free;
            self->target_ent->nextthink = self->target_ent->count;

            return;
        } else {
            event = G_TempEntity(self->r.currentOrigin, EV_FREEZE_TIME);
            self->freezeTime = level.time + (g_autoThawTime.integer * 1000);
            event->s.time = self->freezeTime; // Store the freezeTime value in the event
            event->r.svFlags |= SVF_SINGLECLIENT; // Send the event only to the specific client
            event->r.singleClient = self->s.clientNum;
            event->s.eventParm = self->s.clientNum;
        }
        // Track the temporary entity
        self->client->freezeEvent = event;
    }

	//G_LogPrintf("CALL: CheckLastPlayerAlive from player_freeze target\n");
	CheckLastPlayerAlive(self->client->sess.sessionTeam);

	if ( attacker->client && self != attacker && NearbyBody( self ) ) {
		attacker->client->ps.persistant[ PERS_DEFEND_COUNT ]++;
		attacker->client->ps.eFlags &= ~( EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
		attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
		attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;
	}
}

qboolean readyCheck( void ) {
	int	i;
	gentity_t	*e;

	if ( level.warmupTime == 0 ) return qfalse;
	if ( !g_doReady.integer ) return qfalse;

	for ( i = 0; i < g_maxclients.integer; i++ ) {
		e = g_entities + i;
		if ( !e->inuse ) continue;
		if ( e->r.svFlags & SVF_BOT ) continue;
		if ( e->client->pers.connected == CON_DISCONNECTED ) continue;
		if ( e->client->sess.sessionTeam == TEAM_SPECTATOR ) continue;
		if ( !e->readyBegin ) return qtrue;
	}
	return qfalse;
}

//qlone - added code instead of just prototype
#define	MAX_SPAWN_POINTS 128
gentity_t *SelectRandomDeathmatchSpawnPoint( void ) {
	gentity_t	*spot;
	int		count;
	int		selection;
	gentity_t	*spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
		spots[ count ] = spot;
		count++;
	}

	if ( !count ) { // no spots that won't telefrag
		return G_Find( NULL, FOFS(classname), "info_player_deathmatch");
	}

	selection = rand() % count;
	return spots[ selection ];
}
//qlone - added

void Team_ForceGesture( int team );

void team_wins( int team ) {
	int	i;
	gentity_t	*e;
	char	*teamstr;
	gentity_t	*spawnPoint;
	int	j;
	int	flight;
	gclient_t	*cl;
	gentity_t	*te;
	vec3_t spawnOrigin, spawnAngles;

	for ( i = 0; i < g_maxclients.integer; i++ ) {
		e = g_entities + i;
		cl = e->client;
		if ( !e->inuse ) continue;
		if ( e->freezeState ) {
			if ( !( g_dmflags.integer & 128 ) || cl->sess.sessionTeam != team ) {
				e->lastState = qfalse;
				player_free( e );
			}
			continue;
		}
		if ( e->health < 1 ) continue;
		if ( is_spectator( cl ) ) continue;
		if ( g_dmflags.integer & 64 ) continue;

		if (g_freezeSpawns.integer == 1) {
			spawnPoint = SelectFreezeSpawnPoint(e, team, TEAM_BEGIN, spawnOrigin, spawnAngles);
		} else {
			spawnPoint = SelectRandomDeathmatchSpawnPoint();
		}

		if ( e->health < cl->ps.stats[ STAT_MAX_HEALTH ] ) {
			e->health = cl->ps.stats[ STAT_MAX_HEALTH ];
		}

		memset( cl->ps.ammo, 0, sizeof ( cl->ps.ammo ) );

		cl->ps.stats[ STAT_WEAPONS ] = 1 << WP_MACHINEGUN;
		cl->ps.ammo[ WP_MACHINEGUN ] = 50;

		cl->ps.stats[ STAT_WEAPONS ] |= 1 << WP_GAUNTLET;
		cl->ps.ammo[ WP_GAUNTLET ] = -1;
		cl->ps.ammo[ WP_GRAPPLING_HOOK ] = -1;

		cl->ps.weapon = WP_MACHINEGUN;
		cl->ps.weaponstate = WEAPON_READY;

		G_SpawnWeapon( cl );
		if ( g_dmflags.integer & 1024 ) G_SetInfiniteAmmo( cl );

		// Save flight powerup state
		flight = cl->ps.powerups[ PW_FLIGHT ];

		// Always handle flags at round end.
		// In RTF a player may simultaneously carry both their own flag and the
		// enemy flag, so we must preserve BOTH powerups instead of just one.
		if ( g_gametype.integer == GT_RTF ) {
			int savedRed  = cl->ps.powerups[ PW_REDFLAG  ];
			int savedBlue = cl->ps.powerups[ PW_BLUEFLAG ];
			if ( savedRed || savedBlue ) {
				memset( cl->ps.powerups, 0, sizeof( cl->ps.powerups ) );
				if ( savedRed  ) cl->ps.powerups[ PW_REDFLAG  ] = INT_MAX;
				if ( savedBlue ) cl->ps.powerups[ PW_BLUEFLAG ] = INT_MAX;
			} else if ( g_powerupReset.integer ) {
				memset( cl->ps.powerups, 0, sizeof( cl->ps.powerups ) );
			}
		} else if ( cl->ps.powerups[ PW_REDFLAG ] ) {
			memset( cl->ps.powerups, 0, sizeof ( cl->ps.powerups ) );
			cl->ps.powerups[ PW_REDFLAG ] = INT_MAX;
		} else if ( cl->ps.powerups[ PW_BLUEFLAG ] ) {
			memset( cl->ps.powerups, 0, sizeof ( cl->ps.powerups ) );
			cl->ps.powerups[ PW_BLUEFLAG ] = INT_MAX;
		} else if ( cl->ps.powerups[ PW_NEUTRALFLAG ] ) {
			memset( cl->ps.powerups, 0, sizeof ( cl->ps.powerups ) );
			cl->ps.powerups[ PW_NEUTRALFLAG ] = INT_MAX;
		} else if (g_powerupReset.integer) {
			// Only reset other powerups if g_powerupReset is enabled
			memset( cl->ps.powerups, 0, sizeof ( cl->ps.powerups ) );
		}
		cl->ps.powerups[ PW_FLIGHT ] = flight;

		// Always reset armor
		cl->ps.stats[ STAT_ARMOR ] = 0;

		if ( !( g_dmflags.integer & 1024 ) ) G_UseTargets( spawnPoint, e );
		cl->ps.weapon = 1;
		for ( j = WP_NUM_WEAPONS - 1; j > 0; j-- ) {
			if ( cl->ps.stats[ STAT_WEAPONS ] & ( 1 << j ) ) {
				cl->ps.weapon = j;
				break;
			}
		}
		if ( cl->ps.stats[ STAT_WEAPONS ] & ( 1 << WP_ROCKET_LAUNCHER ) ) {
			cl->ps.weapon = WP_ROCKET_LAUNCHER;
		}
	}

	if ( level.numPlayingClients < 2 || g_gametype.integer == GT_CTF || g_gametype.integer == GT_RTF ) {
		return;
	}

	te = G_TempEntity( vec3_origin, EV_GLOBAL_TEAM_SOUND );
	if ( team == TEAM_RED ) {
		teamstr = "Red";
		if ( g_gametype.integer == GT_CTF ) {
			te->s.eventParm = GTS_BLUE_CAPTURE;
		} else {
			te->s.eventParm = GTS_REDTEAM_SCORED;
		}
	} else {
		teamstr = "Blue";
		if (g_gametype.integer == GT_CTF) {
			te->s.eventParm = GTS_RED_CAPTURE;
		} else {
			te->s.eventParm = GTS_BLUETEAM_SCORED;
		}
	}
	te->r.svFlags |= SVF_BROADCAST;

	trap_SendServerCommand( -1, va( "cp \"" S_COLOR_MAGENTA "%s " S_COLOR_WHITE "team scores!\n\"", teamstr ) );
	trap_SendServerCommand( -1, va( "print \"%s team scores!\n\"", teamstr ) );

	AddTeamScore( vec3_origin, team, 1 );
	Team_ForceGesture( team );

	CalculateRanks();
	//G_LogPrintf("CALL: CheckLastPlayerAlive from team_wins\n");
	// Reset last player states for BOTH teams when a round ends
	// We need to reset both to handle cases where players are respawning on both teams
	ResetLastPlayerStates(TEAM_RED, -1);
	ResetLastPlayerStates(TEAM_BLUE, -1);
	CheckLastPlayerAlive( team );
}

static qboolean CalculateScores( int team ) {
	int	i;
	gentity_t	*e;
	qboolean	modified = qfalse;

	for ( i = 0; i < g_maxclients.integer; i++ ) {
		e = g_entities + i;
		if ( !e->inuse ) continue;
		if ( e->client->sess.sessionTeam != team ) continue;
		if ( e->freezeState ) {
			modified = qtrue;
			continue;
		}
		if ( e->client->pers.connected == CON_CONNECTING ) continue;
		if ( ( e->health < 1 || is_spectator( e->client ) ) && level.time > e->client->respawnTime ) continue;
		return qfalse;
	}
	if ( modified ) {
		team_wins( OtherTeam( team ) );
	}
	return modified;
}

void CheckDelay( void ) {
	int	i;
	gentity_t	*e;
	int	readyMask;

	readyMask = 0;
	for ( i = 0; i < g_maxclients.integer; i++ ) {
		e = g_entities + i;
		if ( !e->inuse ) continue;
		if ( level.warmupTime != 0 && !e->readyBegin ) continue;
		if ( level.warmupTime == 0 && !e->freezeState ) continue;
		if ( i < 16 ) {
			readyMask |= 1 << i;
		}
	}
	for ( i = 0; i < g_maxclients.integer; i++ ) {
		e = g_entities + i;
		if ( !e->inuse ) continue;
		e->client->ps.stats[ STAT_CLIENTS_READY ] = readyMask;
	}

	if ( check_time > level.time - 100 ) {
		return;
	}
	check_time = level.time;

	if ( !CalculateScores( TEAM_RED ) ) {
		CalculateScores( TEAM_BLUE );
	}
}

void SP_target_location( gentity_t *self );

void locationSpawn( gentity_t *ent, gitem_t *item ) {
	gentity_t	*e;

	// Always track flag base positions so the "don't freeze in own base" check
	// in player_freeze works correctly in CTF/RTF regardless of freeze tag.
	if ( item->giType == IT_TEAM ) {
		if ( item->giTag == PW_BLUEFLAG ) {
			VectorCopy( ent->r.currentOrigin, blueflag );
		} else if ( item->giTag == PW_REDFLAG ) {
			VectorCopy( ent->r.currentOrigin, redflag );
		}
	}

	// target_location entities are only used by the freeze tag item-location HUD.
	if ( !g_freezeTag.integer ) {
		return;
	}

	switch ( item->giType ) {
	case IT_AMMO:
		return;
	case IT_ARMOR:
		if ( Q_stricmp( item->classname, "item_armor_shard" ) ) {
			break;
		}
		return;
	case IT_HEALTH:
		if ( !Q_stricmp( item->classname, "item_health_mega" ) ) {
			break;
		}
		return;
	case IT_PERSISTANT_POWERUP:
		return;
	default:
		break;
	}

	e = G_Spawn();
	e->classname = "target_location";
	e->message = item->pickup_name;
	e->count = 255;
	VectorCopy( ent->r.currentOrigin, e->s.origin );

	SP_target_location( e );
}

//void Weapon_GrapplingHook_Fire(	gentity_t *ent );

/* void Hook_Fire( gentity_t *ent ) {
	gclient_t	*client;
	usercmd_t	*ucmd;

	if (g_grapple.integer == 0) {
		return;
	}

	client = ent->client;
	if ( client->ps.weapon == WP_GRAPPLING_HOOK ) {
		return;
	}
	if ( client->ps.pm_type != PM_NORMAL ) {
		return;
	}

	ucmd = &client->pers.cmd;
	if ( client->hook && !( ucmd->buttons & 32 ) ) {
		Weapon_HookFree( client->hook );
	}
	if ( !client->hook && ( ucmd->buttons & 32 ) ) {
		if ( ent->timestamp > level.time) { //timestamp holds time fired + g_grappleDelayTime<
			return;
		}
		client->fireHeld = qfalse;
		Weapon_GrapplingHook_Fire( ent );
	}
} */

char *ConcatArgs( int start );

void Cmd_Drop_f( gentity_t *ent ) {
	char	*name;
	gitem_t	*it;
	gentity_t	*drop;
	int	quantity;
	int	j;

	if ( is_spectator( ent->client ) ) {
		return;
	}
	if ( ent->health <= 0 ) {
		return;
	}
	name = ConcatArgs( 1 );
	it = BG_FindItem( name );
	if ( !Registered( it ) ) {
		return;
	}

	j = it->giTag;
	switch ( it->giType ) {
	case IT_WEAPON:
		if ( g_dmflags.integer & 256 ) {
			return;
		}
		if ( !( ent->client->ps.stats[ STAT_WEAPONS ] & ( 1 << j ) ) ) {
			return;
		}
		if ( ent->client->ps.weaponstate != WEAPON_READY ) {
			return;
		}
		if ( j == ent->s.weapon ) {
			return;
		}
		if ( j > WP_MACHINEGUN && j != WP_GRAPPLING_HOOK && ent->client->ps.ammo[ j ] ) {
			drop = Drop_Item( ent, it, 0 );
			drop->count = 1;
			drop->s.otherEntityNum = ent->s.clientNum + 1;
			ent->client->ps.stats[ STAT_WEAPONS ] &= ~( 1 << j );
			ent->client->ps.ammo[ j ] -= 1;
		}
		break;
	case IT_AMMO:
		quantity = ent->client->ps.ammo[ j ];
		if ( !quantity ) {
			return;
		}
		if ( quantity > it->quantity ) {
			quantity = it->quantity;
		}
		drop = Drop_Item( ent, it, 0 );
		drop->count = quantity;
		drop->s.otherEntityNum = ent->s.clientNum + 1;
		ent->client->ps.ammo[ j ] -= quantity;
		break;
	case IT_POWERUP:
		if ( ent->client->ps.powerups[ j ] > level.time ) {
			drop = Drop_Item( ent, it, 0 );
			drop->count = ( ent->client->ps.powerups[ j ] - level.time ) / 1000;
			if ( drop->count < 1 ) {
				drop->count = 1;
			}
			drop->s.otherEntityNum = ent->s.clientNum + 1;
			ent->client->ps.powerups[ j ] = 0;
		}
		break;
	case IT_HOLDABLE:
		if ( j == HI_KAMIKAZE ) {
			return;
		}
		if ( bg_itemlist[ ent->client->ps.stats[ STAT_HOLDABLE_ITEM ] ].giTag == j ) {
			drop = Drop_Item( ent, it, 0 );
			drop->s.otherEntityNum = ent->s.clientNum + 1;
			ent->client->ps.stats[ STAT_HOLDABLE_ITEM ] = 0;
		}
		break;
	default:
		break;
	}
}

void Cmd_Ready_f( gentity_t *ent ) {
	ent->readyBegin = qtrue;
	trap_SendServerCommand( ent - g_entities, "print \"ready\n\"" );
}


// qlone - dedicated function as original RegisterWeapon is now a generic
// game function (G_RegisterWeapon)
void FT_ResetFlags ( void ) {
	VectorClear( redflag );
	VectorClear( blueflag );
}

void ResetFreezeTimeEvent(gentity_t *ent, int clientNum) {
	// Check if the player is a bot
    if (ent->r.svFlags & SVF_BOT || ent->s.clientNum != clientNum) {
        return; // Skip output for bots
    }

    if (ent->client->freezeEvent) {
        G_FreeEntity(ent->client->freezeEvent);
        ent->client->freezeEvent = NULL;
    }

    ent->freezeTime = 0;
	//ent->lastTime = level.time;
}

#define SPAWN_GRACE_PERIOD 2000 // 2 seconds

/**
 * Checks if there's a single last player alive on a team and handles notifications
 */
void CheckLastPlayerAlive(int team) {
    int i, aliveCount = 0, lastPlayer = -1;
    int teamCount = 0;
    gentity_t *ent;
    static int lastPlayerCache[4] = {-1, -1, -1, -1}; // Cache for each team
    int teamIndex = (team == TEAM_RED) ? 0 : (team == TEAM_BLUE) ? 1 : -1;

    //G_LogPrintf("CALL: CheckLastPlayerAlive for team %d\n", team);

    // Validate team
    if (team != TEAM_RED && team != TEAM_BLUE) {
        return;
    }

    if (teamIndex == -1) {
        return;
    }

    // Skip during warmup or intermission
    if (level.time < g_warmup.integer * 1000 || level.intermissiontime || level.intermissionQueued) {
        //G_LogPrintf("DEBUG: Skipping CheckLastPlayerAlive - warmup or intermission active\n");
        return;
    }

    // Count total team members
    for (i = 0; i < level.maxclients; i++) {
        ent = &g_entities[i];
        if (!ent->inuse || !ent->client || ent->client->sess.sessionTeam != team) {
            continue;
        }

        // Clear expired grace periods
        if (ent->justLost && ent->gracePeriodEnd > 0 && level.time >= ent->gracePeriodEnd) {
            ent->justLost = qfalse;
            ent->gracePeriodEnd = 0;
            //G_LogPrintf("DEBUG: Player %d (%s) grace period expired\n",
                //i, ent->client->pers.netname);
        }

        teamCount++;
    }

    // Need at least 2 players on a team for last player status
    if (teamCount < 2) {
        //G_LogPrintf("DEBUG: Not enough players on team %d (%d found)\n", team, teamCount);
        if (lastPlayerCache[teamIndex] != -1) {
            ResetLastPlayerStates(team, -1);
            lastPlayerCache[teamIndex] = -1;
        }
        return;
    }

    // Find alive players and identify the last one
    for (i = 0; i < level.maxclients; i++) {
        ent = &g_entities[i];
        if (!ent->inuse || !ent->client || ent->client->sess.sessionTeam != team) {
            continue;
        }

        // Players in spawn grace period count as alive
        if (!ent->freezeState && ent->client->respawnTime > level.time) {
            if (!ent->justLost) {
                ent->gracePeriodEnd = level.time + SPAWN_GRACE_PERIOD;
                ent->justLost = qtrue;
                //G_LogPrintf("DEBUG: Player %d (%s) entered grace period\n",
                    //i, ent->client->pers.netname);
            }

            if (ent->gracePeriodEnd >= level.time) {
                aliveCount++;
                lastPlayer = i;
                //G_LogPrintf("DEBUG: Player %d (%s) in grace period counted as alive\n",
                    //i, ent->client->pers.netname);
            }
        }
        // Non-frozen players are alive
        else if (!ent->freezeState) {
            aliveCount++;
            lastPlayer = i;
            //G_LogPrintf("DEBUG: Player %d (%s) is alive\n", i, ent->client->pers.netname);
        }
        // Reset justLost for frozen players
        else if (ent->freezeState && ent->justLost) {
            ent->justLost = qfalse;
            ent->gracePeriodEnd = 0;
        }
    }

    // Single last player check
    if (aliveCount != 1) {
        lastPlayer = -1;
        //G_LogPrintf("DEBUG: Team %d has %d alive players (not last player scenario)\n",
            //team, aliveCount);
    } else {
        //G_LogPrintf("DEBUG: Team %d has exactly 1 alive player: %d (%s)\n",
            //team, lastPlayer, g_entities[lastPlayer].client->pers.netname);
    }

    // Handle last player state changes
    if (lastPlayerCache[teamIndex] != lastPlayer) {
        //G_LogPrintf("DEBUG: Last player changed for team %d: %d -> %d\n",
            //team, lastPlayerCache[teamIndex], lastPlayer);

        // Reset previous last player
        if (lastPlayerCache[teamIndex] != -1) {
            ResetLastPlayerStates(team, lastPlayer);
        }

        // Update cache
        lastPlayerCache[teamIndex] = lastPlayer;

        // Process new last player
        if (lastPlayer != -1) {
            HandleLastPlayerLogic(lastPlayer);
        }
    }
    // Re-check notifications for existing last player
    else if (lastPlayer != -1) {
        //G_LogPrintf("DEBUG: Same last player for team %d: %d (%s)\n",
            //team, lastPlayer, g_entities[lastPlayer].client->pers.netname);
        HandleLastPlayerLogic(lastPlayer);
    }
}

/**
 * Handles notifications for the last player and frozen teammates following them
 */
void HandleLastPlayerLogic(int lastPlayer) {
    gentity_t *lastEnt = &g_entities[lastPlayer];
    int i;

    //G_LogPrintf("CALL: HandleLastPlayerLogic for player %d (%s)\n",
        //lastPlayer, lastEnt->client->pers.netname);

    if (!lastEnt->inuse || !lastEnt->client) {
        return;
    }

    // First, notify the last player
    if (!lastEnt->lastState) {
        trap_SendServerCommand(lastPlayer, "lastplayer 1");
        lastEnt->lastState = qtrue;
        //G_LogPrintf("DEBUG: Notified player %d (%s) they are last standing\n",
            //lastPlayer, lastEnt->client->pers.netname);
    }

    // Next, handle frozen teammates who are following the last player
    for (i = 0; i < level.maxclients; i++) {
        gentity_t *frozenPlayer = &g_entities[i];

        if (!frozenPlayer->inuse || !frozenPlayer->client) {
            continue;
        }

        // Only handle frozen teammates - THIS IS IMPORTANT
        if (!frozenPlayer->freezeState ||
            frozenPlayer->client->sess.sessionTeam != lastEnt->client->sess.sessionTeam ||
            frozenPlayer == lastEnt) {
            continue;
        }

        // Log all frozen player states for debugging
        //G_LogPrintf("DEBUG: Frozen %d (%s) - spectatorState: %d, spectatorClient: %d\n",
            //i, frozenPlayer->client->pers.netname,
            //frozenPlayer->client->sess.spectatorState,
            //frozenPlayer->client->sess.spectatorClient);

        // If this frozen player is following the last player, notify them
        if (frozenPlayer->client->sess.spectatorState == SPECTATOR_FOLLOW &&
            frozenPlayer->client->sess.spectatorClient == lastPlayer) {

            if (!frozenPlayer->client->notifiedLastPlayer) {
                trap_SendServerCommand(i, "lastplayer 1");
                frozenPlayer->client->notifiedLastPlayer = qtrue;
                //G_LogPrintf("DEBUG: Notified frozen %d (%s) that %d (%s) is last standing\n",
                //    i, frozenPlayer->client->pers.netname,
                //    lastPlayer, lastEnt->client->pers.netname);
            }
        }
        // Only reset notifications for teammates who should be following this last player
        else if (frozenPlayer->client->notifiedLastPlayer &&
                 (frozenPlayer->client->sess.spectatorState != SPECTATOR_FOLLOW ||
                  frozenPlayer->client->sess.spectatorClient != lastPlayer)) {
            trap_SendServerCommand(i, "lastplayer 0");
            frozenPlayer->client->notifiedLastPlayer = qfalse;
            //G_LogPrintf("DEBUG: Cleared last player notification for frozen %d (%s)\n",
                //i, frozenPlayer->client->pers.netname);
        }
    }
}

/**
 * Resets last player states for a team
 */
void ResetLastPlayerStates(int team, int newLastPlayer) {
    int i;
    gentity_t *ent;

    //G_LogPrintf("CALL: ResetLastPlayerStates for team %d (new last: %d)\n",
        //team, newLastPlayer);

    // Reset all players on the team
    for (i = 0; i < level.maxclients; i++) {
        ent = &g_entities[i];

        if (!ent->inuse || !ent->client) {
            continue;
        }

        // Only handle players on this team
        if (ent->client->sess.sessionTeam != team) {
            continue;
        }

        // Clear last player status
        if (ent->lastState && (i != newLastPlayer)) {
            trap_SendServerCommand(i, "lastplayer 0");
            ent->lastState = qfalse;
            //G_LogPrintf("DEBUG: Reset last player status for %d (%s)\n",
                //i, ent->client->pers.netname);
        }

        // Clear notifications for frozen players not following the new last player
        if (ent->freezeState && ent->client->notifiedLastPlayer) {
            ent->client->notifiedLastPlayer = qfalse;
            if (ent->client->sess.spectatorState != SPECTATOR_FOLLOW ||
                ent->client->sess.spectatorClient != newLastPlayer) {

                trap_SendServerCommand(i, "lastplayer 0");
                //G_LogPrintf("DEBUG: Reset notification for frozen %d (%s)\n",
                    //i, ent->client->pers.netname);
            }
        }
    }
}

/**
 * Updates the last player notification when a frozen player changes who they're following
 */
void UpdateSpectatorLastPlayerState(gentity_t *spectator) {
    int followedPlayer;
    gentity_t *followed;

    //G_LogPrintf("CALL: UpdateSpectatorLastPlayerState for %d (%s)\n",
        //spectator->s.clientNum, spectator->client->pers.netname);

    if (!spectator || !spectator->client || !spectator->freezeState) {
        return;
    }

    //G_LogPrintf("DEBUG: UpdateSpectatorLastPlayerState processing player %d (%s)\n",
                //spectator->s.clientNum, spectator->client->pers.netname);

    // Only handle players in follow mode
    if (spectator->client->sess.spectatorState != SPECTATOR_FOLLOW) {
        if (spectator->client->notifiedLastPlayer) {
            trap_SendServerCommand(spectator->s.clientNum, "lastplayer 0");
            spectator->client->notifiedLastPlayer = qfalse;
            //G_LogPrintf("DEBUG: Cleared notification for %d (%s) - not following anyone\n",
                //spectator->s.clientNum, spectator->client->pers.netname);
        }
        return;
    }

    // Get the player being followed
    followedPlayer = spectator->client->sess.spectatorClient;
    if (followedPlayer < 0 || followedPlayer >= level.maxclients) {
        return;
    }

    followed = &g_entities[followedPlayer];
    if (!followed->inuse || !followed->client) {
        return;
    }

    //G_LogPrintf("DEBUG: Player %d following %d (%s) - followed->lastState: %d, notifiedLastPlayer: %d\n",
                //spectator->s.clientNum, followedPlayer, followed->client->pers.netname,
                //followed->lastState, spectator->client->notifiedLastPlayer);

    // Only care about following teammates
    if (followed->client->sess.sessionTeam != spectator->client->sess.sessionTeam) {
        return;
    }

    //G_LogPrintf("DEBUG: %d (%s) following %d (%s) - lastState: %d\n",
        //spectator->s.clientNum, spectator->client->pers.netname,
        //followedPlayer, followed->client->pers.netname,
        //followed->lastState);

    // Update notification based on followed player's last state
    if (followed->lastState) {
        if (!spectator->client->notifiedLastPlayer) {
            trap_SendServerCommand(spectator->s.clientNum, "lastplayer 1");
            spectator->client->notifiedLastPlayer = qtrue;
            //G_LogPrintf("DEBUG: Notified %d (%s) that followed player %d (%s) is last\n",
                //spectator->s.clientNum, spectator->client->pers.netname,
                //followedPlayer, followed->client->pers.netname);
        }
    } else {
        if (spectator->client->notifiedLastPlayer) {
            trap_SendServerCommand(spectator->s.clientNum, "lastplayer 0");
            spectator->client->notifiedLastPlayer = qfalse;
            //G_LogPrintf("DEBUG: Cleared notification for %d (%s) - followed player not last\n",
                //spectator->s.clientNum, spectator->client->pers.netname);
        }
    }
}

void G_FrozenPlayerKnockback(gentity_t *frozenRemnant, int knockback, vec3_t dir) {
    float mass;
    vec3_t kvel;
    float currentSpeed;
    float maxSlideSpeed;
    float newSpeed;
    float dirLength = VectorLength(dir);

    if (g_freezeKnockback.value <= 0) {
        return;
    }

    mass = 3.0f; // Lighter for easier sliding

    // Set up physics for sliding - TR_LINEAR for constant velocity movement
    frozenRemnant->s.pos.trType = TR_LINEAR;
    frozenRemnant->s.pos.trTime = level.time;
    VectorCopy(frozenRemnant->r.currentOrigin, frozenRemnant->s.pos.trBase);

    if (dirLength < 0.001f) {
        // Fallback: use a default forward direction
        VectorSet(dir, 1, 0, 0);
    } else {
        VectorNormalize(dir);
    }

    // Calculate new velocity to add (mostly horizontal for sliding)
    VectorScale(dir, g_freezeKnockback.value * (float)knockback / mass, kvel);

    // Minimal vertical component - we want sliding, not bouncing
    kvel[2] = 0; // Keep it purely horizontal for ice sliding

    // Blend with existing velocity for realistic momentum transfer
    currentSpeed = VectorLength(frozenRemnant->s.pos.trDelta);

    if (currentSpeed > 0) {
        // Blend velocities for realistic physics
        vec3_t blendedVel;
        VectorScale(frozenRemnant->s.pos.trDelta, 0.7f, blendedVel);
        VectorMA(blendedVel, 0.3f, kvel, frozenRemnant->s.pos.trDelta);
    } else {
        // Apply scaled impulse for initial sliding
        VectorScale(kvel, 0.8f, frozenRemnant->s.pos.trDelta);
    }

    // Cap maximum sliding speed
    maxSlideSpeed = 300.0f;
    newSpeed = VectorLength(frozenRemnant->s.pos.trDelta);
    if (newSpeed > maxSlideSpeed) {
        VectorScale(frozenRemnant->s.pos.trDelta, maxSlideSpeed / newSpeed, frozenRemnant->s.pos.trDelta);
    }
}
