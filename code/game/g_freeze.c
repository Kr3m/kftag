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

		G_LogPrintf("CALL: UpdateSpectatorLastPlayerState from Set_spectator\n");
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

			G_LogPrintf( "Thaw: %i %i %i: %s thawed %s by %s\n", e->s.number, self->target_ent->s.number, MOD_UNKNOWN, e->client->pers.netname, self->target_ent->client->pers.netname, "MOD_UNKNOWN" );
			e->client->pers.stats.thaws++;
			AddScore( e, self->s.pos.trBase, 2 );
			G_LogPrintf("CALL: CheckLastPlayerAlive from Body_Explode\n");
			CheckLastPlayerAlive( e->client->sess.sessionTeam );

			G_Damage( self, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
		}
		return;
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

	VectorCopy( self->r.currentOrigin, point );
	point[ 2 ] -= 23;

	contents = trap_PointContents( point, -1 );
	if ( contents & ( CONTENTS_LAVA | CONTENTS_SLIME ) ) {
		if ( level.time - self->timestamp > 5000 ) {
			G_Damage( self, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
		}
		return;
	}
	if ( self->s.pos.trType == TR_STATIONARY && contents & CONTENTS_NODROP ) {
		if ( level.time - self->timestamp > 5000 ) {
			Body_free( self );
		}
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
	if ( level.time - self->timestamp > 150000 || ( ( g_dmflags.integer & 1024 || g_gametype.integer == GT_CTF ) && level.time - self->timestamp >= ( g_autoThawTime.integer * 1000 ) ) ) {
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

            VectorCopy(self->r.currentOrigin, groundCheck);
            groundCheck[2] -= 32; // Check 32 units below

            trap_Trace(&trace, self->r.currentOrigin, self->r.mins, self->r.maxs,
                       groundCheck, self->s.number, MASK_PLAYERSOLID);

            // If we're not on solid ground, switch to gravity
            if (trace.fraction >= 1.0f || trace.startsolid) {
                G_LogPrintf("DEBUG: Body falling into void, switching to gravity\n");
                self->s.pos.trType = TR_GRAVITY;
                self->s.pos.trTime = level.time;
                VectorCopy(self->r.currentOrigin, self->s.pos.trBase);

                // Give it some initial downward velocity to help gravity take effect
                if (self->s.pos.trDelta[2] > -50) {
                    self->s.pos.trDelta[2] = -50; // Initial falling velocity
                }

                // Clear ground entity so it falls properly
                self->s.groundEntityNum = ENTITYNUM_NONE;

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
                        G_LogPrintf("DEBUG: Body stopped sliding, speed was %.2f\n", speed);
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
                G_LogPrintf("DEBUG: Body landed, switching back to sliding\n");
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
			if ( g_freezeKnockback.integer ) {
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

	if ( g_gametype.integer == GT_CTF ) {
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
	if ( g_gametype.integer != GT_TEAM && g_gametype.integer != GT_CTF ) {
		return;
	}

	if ( self != attacker && OnSameTeam( self, attacker ) ) {
		return;
	}
	if ( self != attacker && g_gametype.integer == GT_CTF && redflag && blueflag ) {
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
	check_time = ( level.time - (g_lavaThawTime.integer * 1000) ) + 200;

	self->takedamage = qfalse;
	self->s.eType = ET_INVISIBLE;
	self->r.contents = 0;
	self->health = GIB_HEALTH;

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

	G_LogPrintf("CALL: CheckLastPlayerAlive from player_freeze target\n");
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
	int			losers;
	vec3_t spawnOrigin, spawnAngles;

	// Determine the losing team
    if (team == TEAM_RED) {
        losers = TEAM_BLUE;
    } else if (team == TEAM_BLUE) {
        losers = TEAM_RED;
    } else {
        // Handle unexpected cases (e.g., invalid team)
        losers = -1; // Invalid team
    }

	//ResetLastPlayerStates( losers, -1 );
	//ResetLastPlayerStates( team, -1 );

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

		// Always handle flags at round end
		if ( cl->ps.powerups[ PW_REDFLAG ] ) {
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

	if ( level.numPlayingClients < 2 || g_gametype.integer == GT_CTF ) {
		return;
	}

	te = G_TempEntity( vec3_origin, EV_GLOBAL_TEAM_SOUND );
	if ( team == TEAM_RED ) {
		teamstr = "Red";
		te->s.eventParm = GTS_BLUE_CAPTURE;
	} else {
		teamstr = "Blue";
		te->s.eventParm = GTS_RED_CAPTURE;
	}
	te->r.svFlags |= SVF_BROADCAST;

	trap_SendServerCommand( -1, va( "cp \"" S_COLOR_MAGENTA "%s " S_COLOR_WHITE "team scores!\n\"", teamstr ) );
	trap_SendServerCommand( -1, va( "print \"%s team scores!\n\"", teamstr ) );

	AddTeamScore( vec3_origin, team, 1 );
	Team_ForceGesture( team );
	G_LogPrintf("CALL: CheckLastPlayerAlive from team_wins\n");
	CheckLastPlayerAlive( team );

	CalculateRanks();
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

	if ( check_time > level.time - 3000 ) {
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
	case IT_TEAM:
		if ( item->giTag == PW_BLUEFLAG ) {
			VectorCopy( ent->r.currentOrigin, blueflag );
		} else if ( item->giTag == PW_REDFLAG ) {
			VectorCopy( ent->r.currentOrigin, redflag );
		}
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

void CheckLastPlayerAlive(int team) {
    int i, aliveCount = 0, lastPlayer = -1;
	int teamCount = 0;
    gentity_t *ent;

    #define SPAWN_GRACE_PERIOD 2000

	if (team != TEAM_RED && team != TEAM_BLUE) {
		return; // Invalid team
	}

	// Count the number of players on the team
    for (i = 0; i < level.maxclients; i++) {
        ent = &g_entities[i];
        if (!ent->inuse || !ent->client || ent->client->sess.sessionTeam != team)
            continue;

        // Clear justLost if grace period has expired
        if (ent->justLost && ent->gracePeriodEnd > 0 && level.time >= ent->gracePeriodEnd) {
            ent->justLost = qfalse;
            ent->gracePeriodEnd = 0;
            G_LogPrintf("DEBUG: Player %d (%s) justLost cleared - grace period expired\n",
                        i, ent->client->pers.netname);
        }

        teamCount++;
    }

    // Return early if less than 2 players on the team
    if (teamCount < 2) {
        G_LogPrintf("DEBUG: Not enough players on team %d (%d found), skipping CheckLastPlayerAlive.\n", team, teamCount);
        return;
    }

    // Debugging: Start logging
    G_LogPrintf("DEBUG: CheckLastPlayerAlive called for team %d at level.time %d\n", team, level.time);

    // Skip processing if warmup or intermission is active
    if (level.time < g_warmup.integer * 1000 || level.intermissiontime || level.intermissionQueued) {
		G_LogPrintf("DEBUG: Skipping CheckLastPlayerAlive due to warmup or intermission. level.time: %d\n", level.time);
        return;
    }

    // Calculate the grace period expiration time
    ent->gracePeriodEnd = level.time + SPAWN_GRACE_PERIOD;
    G_LogPrintf("DEBUG: Grace period ends at %d (SPAWN_GRACE_PERIOD: %d ms)\n", ent->gracePeriodEnd, SPAWN_GRACE_PERIOD);

    // Iterate through all clients to determine alive players and update states
    for (i = 0; i < level.maxclients; i++) {
        ent = &g_entities[i];

        // Skip invalid entities or those not on the correct team
        if (!ent->inuse || !ent->client || ent->client->sess.sessionTeam != team) {
            continue;
        }

        // Update justLost state based on the grace period
        ent->justLost = (!ent->freezeState &&
                         ent->client->respawnTime > level.time &&
                         ent->client->respawnTime <= ent->gracePeriodEnd);

        // Debugging: Log player details
		G_LogPrintf("DEBUG: Player %d (%s) - freezeState: %d, respawnTime: %d, health: %d, justLost: %d, lastState: %d\n",
            i, ent->client->pers.netname, ent->freezeState, ent->client->respawnTime, ent->health, ent->justLost, ent->lastState);

		// Count alive players and temporarily set lastPlayer
        if (!ent->freezeState || ent->justLost) {
            aliveCount++;
            lastPlayer = i;
        } else {
			ent->justLost = qfalse; // Reset justLost if the player is frozen
		}

        // Reset lastState for players who just lost
        if (ent->justLost && ent->lastState) {
            trap_SendServerCommand(ent - g_entities, "lastplayer 0");
			ent->lastState = qfalse; // Reset lastState for all players
            G_LogPrintf("DEBUG: Player %d (%s) just lost. Resetting lastplayer state.\n", i, ent->client->pers.netname);
        }
    }

	if (aliveCount > teamCount) {
		aliveCount = teamCount; // Ensure aliveCount does not exceed total players
	}

    // Debugging: Log alive player count
    G_LogPrintf("DEBUG: Alive player count for team %d: %d\n", team, aliveCount);

    // Determine if there is a single last player
    if (aliveCount != 1) {
        lastPlayer = -1; // Reset lastPlayer if there are multiple or no alive players
    }

    // Handle the last player logic
    if (lastPlayer != -1) {
        G_LogPrintf("DEBUG: Last player alive for team %d: Player %d (%s)\n", team, lastPlayer, g_entities[lastPlayer].client->pers.netname);
        HandleLastPlayerLogic(lastPlayer);
    } else {
        G_LogPrintf("DEBUG: No single last player alive for team %d. Resetting states.\n", team);
        ResetLastPlayerStates(team, lastPlayer);
    }
}

void HandleLastPlayerLogic(int lastPlayer) {
    gentity_t *lastEnt = &g_entities[lastPlayer];
    int i;
	qboolean isFollowing = qfalse;

    // Notify spectators watching the last player
    for (i = 0; i < level.maxclients; i++) {
		gentity_t *spectator = &g_entities[i];
		if (!spectator->inuse || !spectator->client) {
			continue;
		}

		// Notify both spectators and frozen players who are following the last player
		isFollowing = (
			(spectator->client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ||
			spectator->freezeState) && // also include frozen players
			spectator->client->sess.spectatorState == SPECTATOR_FOLLOW &&
			spectator->client->sess.spectatorClient == lastPlayer
		);

		// if ( isFollowing && spectator->lastState ) {
		// 	spectator->lastState = qfalse; // Reset lastState for all players
		// }

		if (isFollowing) {
			if (!spectator->lastState) {
				UpdateSpectatorClient(&spectator->client->ps, lastPlayer);
				trap_SendServerCommand(spectator - g_entities, "lastplayer 1");
				G_LogPrintf("DEBUG: Spectator/Frozen %d (%s) notified of last player %d (%s).\n",
							i, spectator->client->pers.netname, lastPlayer, lastEnt->client->pers.netname);
				spectator->lastState = qtrue;
			}
		} else {
			if (spectator->client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
				trap_SendServerCommand(spectator - g_entities, "lastplayer 0");
				spectator->lastState = qfalse; // Reset lastState for all spectators
				G_LogPrintf("DEBUG: Spectator %d (%s) reset lastplayer state.\n", i, spectator->client->pers.netname);
			}
		}
	}

    // Set the last player's state after notifying spectators
    if (!lastEnt->lastState) {
        trap_SendServerCommand(lastEnt - g_entities, "lastplayer 1");
        lastEnt->lastState = qtrue; // Update the state
        G_LogPrintf("DEBUG: Last player %d (%s) state set to lastplayer 1.\n", lastPlayer, lastEnt->client->pers.netname);
    }
}

void ResetLastPlayerStates(int team, int lastPlayer) {
    int i;
	gentity_t *followedEnt;

    for (i = 0; i < level.maxclients; i++) {
        gentity_t *ent = &g_entities[i];
        if (!ent->inuse || !ent->client || ent->client->sess.sessionTeam != team) {
            continue;
        }

        // Reset lastState for players on the same team
		if (ent->client->sess.sessionTeam == team) {
			trap_SendServerCommand(ent - g_entities, "lastplayer 0");
			if (ent->lastState) {
				G_LogPrintf("DEBUG: Resetting lastplayer state for player %d (%s) on team %d.\n", i, ent->client->pers.netname, team);
			}
			ent->lastState = qfalse; // Always reset
		}

        if (ent->client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
			ent->client->sess.spectatorState == SPECTATOR_FOLLOW &&
			ent->client->sess.spectatorClient == lastPlayer) {
			followedEnt = &g_entities[lastPlayer];
			if (followedEnt->lastState) {
				G_LogPrintf("DEBUG: Resetting lastplayer state for spectator %d (%s) following player %d.\n",
							i, ent->client->pers.netname, lastPlayer);
				trap_SendServerCommand(ent - g_entities, "lastplayer 0");
				ent->lastState = qfalse; // Always reset
			}
		}

        // Reset spectators following any player on the same team
        if (ent->client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
            ent->client->sess.spectatorState == SPECTATOR_FOLLOW) {
            int followedPlayer = ent->client->sess.spectatorClient;
            if (followedPlayer >= 0 && followedPlayer < level.maxclients) {
                gentity_t *followedEnt = &g_entities[followedPlayer];
                if (followedEnt->client && followedEnt->client->sess.sessionTeam == team) {
                    if(followedEnt->lastState) {
						G_LogPrintf("DEBUG: Resetting lastplayer state for spectator %d (%s) following player %d (%s) on team %d.\n",
                                i, ent->client->pers.netname, followedPlayer, followedEnt->client->pers.netname, team);
						trap_SendServerCommand(ent - g_entities, "lastplayer 0");
						followedEnt->lastState = qfalse; // Reset the state
					}
                }
            }
        }
    }
	
	for (i = 0; i < level.maxclients; i++) {
		gentity_t *ent = &g_entities[i];
		if (!ent->inuse || !ent->client) continue;
		if (ent->client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
			if (ent->lastState) {
				trap_SendServerCommand(ent - g_entities, "lastplayer 0");
				ent->lastState = qfalse;
			}
		}
	}
}

void UpdateSpectatorLastPlayerState(gentity_t *spectator) {
    int followedPlayer;
    gentity_t *followed;

    if (!spectator || !spectator->client || !spectator->freezeState) return;

    // Only handle actual spectators, not frozen players
    if (spectator->client->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR) return;
    if (spectator->client->sess.spectatorState != SPECTATOR_FOLLOW) return;

    followedPlayer = spectator->client->sess.spectatorClient;
    if (followedPlayer < 0 || followedPlayer >= level.maxclients) return;

    followed = &g_entities[followedPlayer];
    if (!followed->inuse || !followed->client) return;

    // Check if the followed player is the last one standing on their team
    if (followed->lastState) {
        // The followed player is last standing, notify the spectator
        if (!spectator->lastState) {
            trap_SendServerCommand(spectator - g_entities, "lastplayer 1");
            spectator->lastState = qtrue;
            G_LogPrintf("DEBUG: Spectator %d (%s) notified of last player %d (%s) via follow cycle.\n",
                       spectator - g_entities, spectator->client->pers.netname,
                       followedPlayer, followed->client->pers.netname);
        }
    } else {
        // The followed player is not last standing, reset spectator state
        if (spectator->lastState) {
            trap_SendServerCommand(spectator - g_entities, "lastplayer 0");
            spectator->lastState = qfalse;
            G_LogPrintf("DEBUG: Spectator %d (%s) reset lastplayer state via follow cycle.\n",
                       spectator - g_entities, spectator->client->pers.netname);
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
