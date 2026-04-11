// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_local.h -- local definitions for the bg (both games) files

#define	MIN_WALK_NORMAL	0.7f		// can't walk on very steep slopes

#define	JUMP_VELOCITY	270

#define	TIMER_LAND		130
#define	TIMER_GESTURE	(34*66+50)

#define	OVERCLIP		1.001f

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server
typedef struct {
	vec3_t		forward, right, up;
	float		frametime;

	int			msec;

	qboolean	walking;
	qboolean	groundPlane;
	trace_t		groundTrace;

	float		impactSpeed;

	vec3_t		previous_origin;
	vec3_t		previous_velocity;
	int			previous_waterlevel;
} pml_t;

extern	pmove_t		*pm;
extern	pml_t		pml;

// movement parameters
extern	float	pm_stopspeed;
extern	float	pm_duckScale;
extern	float	pm_swimScale;
extern	float	pm_wadeScale;

extern	float	pm_accelerate;
extern	float	pm_airaccelerate;
extern	float	pm_wateraccelerate;
extern	float	pm_flyaccelerate;

extern	float	pm_friction;
extern	float	pm_waterfriction;
extern	float	pm_flightfriction;

extern	int		c_pmove;

void PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce );
void PM_AddTouchEnt( int entityNum );
void PM_AddEvent( int newEvent );

qboolean	PM_SlideMove( qboolean gravity );
void		PM_StepSlideMove( qboolean gravity );
extern int modePromodePhysKoeff;
extern float modePromode_pm_airaccelerate_1;
extern int modePredictionKoeff2;
extern float modePromode_pm_airaccelerate_2;
extern float modeWishspeedLimit;
extern int modePredictionKoeff1;
extern float modeSwimScale1;
extern float modeSwimScale2;
extern float modeShotgunKoeff;
extern int modeShotgunNumberOfPellets;
extern float modeUnused8;
extern int modeMaxAmmoShotgun;
extern int modeGrenadeTime;
extern int modeMaxAmmoGrenade;
extern int modeMaxAmmoRocket;
extern int modeMaxAmmoRail;
extern int modeBeginWeaponChangeTime;
extern int modeFinishWeaponChangeTime;
extern int modePMNoAmmoTime;
extern int pm_armorPromode;
extern int modeHitLevelSounds;
extern int modePickupDistance;
extern int modeUnknown2;
extern int modeUnknown3;
extern int modeUnknown4;
extern int modeShotgunPromode;
//int			getCvarInt(const char* name);
