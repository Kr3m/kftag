// Edited by ZerTerO
//
// The Unofficial Patch

textures/REGION
{
	surfaceparm nolightmap
}
lagometer
{
	nopicmip
	{
		map gfx/2d/lag.tga
	}
}
disconnected
{
	nopicmip
	{
		map gfx/2d/net.tga
	}
}
white
{
	{
		map *white
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
console
{
	nopicmip
	nomipmaps
	{
		map gfx/misc/console01.tga
		blendFunc GL_ONE GL_ZERO
		tcMod scroll .02 0
		tcMod scale 2 1
	}
	{
		map gfx/misc/console02.tga
		blendFunc add
		tcMod turb 0 .1 0 .1
		tcMod scale 2 1
		tcMod scroll 0.2 .1
	} 
}
menuback
{
	nopicmip
	nomipmaps
	{
		map textures/sfx/logo1024.tga
		rgbGen identity
	}
}
menubacknologo
{
	nopicmip
	nomipmaps
	{
		map gfx/colors/black.tga
	}
}
menubackRagePro
{
	nopicmip
	nomipmaps
	{
		map textures/sfx/logo1024.tga
	}
}
levelShotDetail
{
	nopicmip
	{
		map textures/sfx2/detail1024.tga
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbGen identity
	}
}
powerups/battleSuit
{
	deformVertexes wave 100 sin 1 0 0 0
	{
		map textures/effects/envmapgold2.tga
		tcGen environment
		tcMod turb 0 0.15 0 0.3
		tcMod rotate 333
		tcMod scroll .3 .3
		blendFunc GL_ONE GL_ONE
	}
}
powerups/battleWeapon
{
	deformVertexes wave 100 sin 0.5 0 0 0
	{
		map textures/effects/envmapgold2.tga
		tcGen environment
		tcMod turb 0 0.15 0 0.3
		tcMod rotate 333
		tcMod scroll .3 .3
		blendFunc GL_ONE GL_ONE
	}
}
powerups/invisibility
{
	{
		map textures/effects/invismap.tga
		blendFunc GL_ONE GL_ONE
		tcMod turb 0 0.15 0 0.25
		tcGen environment
	}
}
powerups/quad
{
	deformVertexes wave 100 sin 3 0 0 0
	{
		map textures/effects/quadmap2.tga
		blendFunc GL_ONE GL_ONE
		tcGen environment
		tcMod rotate 30
		tcMod scroll 1 .1
	}
}
powerups/quadWeapon
{
	deformVertexes wave 100 sin 0.5 0 0 0
	{
		map textures/effects/quadmap2.tga
		blendFunc GL_ONE GL_ONE
		tcGen environment
		tcMod rotate 30
		tcMod scroll 1 .1
	}
}
powerups/regen
{
	deformVertexes wave 100 sin 3 0 0 0
	{
		map textures/effects/regenmap2.tga
		blendFunc GL_ONE GL_ONE
		tcGen environment
		tcMod rotate 30
		tcMod scroll 1 .1
	}
}
icons/teleporter
{
	nopicmip
	{
		map icons/teleporter.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/medkit
{
	nopicmip
	{
		map icons/medkit.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/kamikaze
{
	nopicmip
	{
		map icons/kamikaze.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/portal
{
	nopicmip
	{
		map icons/portal.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/envirosuit
{
	nopicmip
	{
		map icons/envirosuit.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/quad
{
	nopicmip
	{
		map icons/quad.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/haste
{
	nopicmip
	{
		map icons/haste.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/invis
{
	nopicmip
	{
		map icons/invis.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/regen
{
	nopicmip
	{
		map icons/regen.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/flight
{
	nopicmip
	{
		map icons/flight.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/scout
{
	nopicmip
	{
		map icons/scout.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/guard
{
	nopicmip
	{
		map icons/guard.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/doubler
{
	nopicmip
	{
		map icons/doubler.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/radar
{
	nopicmip
	{
		map icons/radar.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/ammo_regen
{
	nopicmip
	{
		map icons/ammo_regen.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/invulnerability
{
	nopicmip
	{
		map icons/invulnerability.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
medal_impressive
{
	nopicmip
	{
		clampmap menu/medals/medal_impressive.tga
		blendFunc blend
	}
}
medal_excellent
{
	nopicmip
	{
		clampmap menu/medals/medal_excellent.tga
		blendFunc blend
	}
}
medal_gauntlet
{
	nopicmip
	{
		clampmap menu/medals/medal_gauntlet.tga
		blendFunc blend
	}
}
medal_assist
{
	nopicmip
	{
		clampmap menu/medals/medal_assist.tga
		blendFunc blend
	}
}
medal_defend
{
	nopicmip
	{
		clampmap menu/medals/medal_defend.tga
		blendFunc blend
	}
}
medal_capture
{
	nopicmip
	{
		clampmap menu/medals/medal_capture.tga
		blendFunc blend
	}
}
icons/iconw_gauntlet
{
	nopicmip
	{
		map icons/iconw_gauntlet.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconw_machinegun
{
	nopicmip
	{
		map icons/iconw_machinegun.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconw_rocket
{
	nopicmip
	{
		map icons/iconw_rocket.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconw_shotgun
{
	nopicmip
	{
		map icons/iconw_shotgun.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identitylighting
	}
}
icons/iconw_grenade
{
	nopicmip
	{
		map icons/iconw_grenade.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconw_lightning
{
	nopicmip
	{
		map icons/iconw_lightning.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconw_plasma
{
	nopicmip
	{
		map icons/iconw_plasma.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconw_railgun
{
	nopicmip
	{
		map icons/iconw_railgun.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconw_bfg
{
	nopicmip
	{
		map icons/iconw_bfg.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconw_grapple
{
	nopicmip
	{
		map icons/iconw_grapple.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconw_nailgun
{
	nopicmip
	{
		map icons/nailgun128.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconw_chaingun
{
	nopicmip
	{
		map icons/chaingun128.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconw_proxlauncher
{
	nopicmip
	{
		map icons/proxmine.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/icona_machinegun
{
	nopicmip
	{
		map icons/icona_machinegun.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/icona_rocket
{
	nopicmip
	{
		map icons/icona_rocket.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/icona_shotgun
{
	nopicmip
	{
		map icons/icona_shotgun.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identitylighting
	}
}
icons/icona_grenade
{
	nopicmip
	{
		map icons/icona_grenade.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/icona_lightning
{
	nopicmip
	{
		map icons/icona_lightning.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/icona_plasma
{
	nopicmip
	{
		map icons/icona_plasma.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/icona_railgun
{
	nopicmip
	{
		map icons/icona_railgun.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/icona_bfg
{
	nopicmip
	{
		map icons/icona_bfg.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/icona_nailgun
{
	nopicmip
	{
		map icons/ammo_nailgun.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/icona_chaingun
{
	nopicmip
	{
		map icons/ammo_chaingun.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/icona_proxlauncher
{
	nopicmip
	{
		map icons/ammo_proxmine.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconr_shard
{
	nopicmip
	{
		map icons/iconr_shard.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconr_yellow
{
	nopicmip
	{
		map icons/iconr_yellow.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconr_red
{
	nopicmip
	{
		map icons/iconr_red.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconh_green
{
	nopicmip
	{
		map icons/iconh_green.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconh_yellow
{
	nopicmip
	{
		map icons/iconh_yellow.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconh_red
{
	nopicmip
	{
		map icons/iconh_red.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconh_mega
{
	nopicmip
	{
		map icons/iconh_mega.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconf_red
{
	nopicmip
	{
		map icons/iconf_red.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
icons/iconf_blu
{
	nopicmip
	{
		map icons/iconf_blu.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
gfx/2d/menuinfo
{
	nopicmip
	{
		map gfx/2d/menuinfo.tga
	}
}
gfx/2d/menuinfo2
{
	nopicmip
	{
		map gfx/2d/menuinfo2.tga
	}
}
gfx/2d/quit
{
	nopicmip
	nomipmaps
	{
		map gfx/2d/quit.tga
	}
}
gfx/2d/cursor
{
	nopicmip
	nomipmaps
	{
		map gfx/2d/cursor.tga
	}
}
sprites/balloon3
{
	{
		map sprites/balloon4.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
textures/animationTest
{
	qer_editorimage	textures/liquids/bloodwater1.tga
	{
		animMap 0.5 textures/liquids/bloodwater1.tga textures/liquids/pool.tga textures/liquids/slime7.tga
	}	
	{
		animMap 0.5 textures/liquids/pool.tga textures/liquids/slime7.tga textures/liquids/bloodwater1.tga
		blendFunc	GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen	wave sawtooth 0 1 0 0.5
	}	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
}
teleportEffect
{
	cull none
	{
		map gfx/misc/teleportEffect2.tga
		blendFunc GL_ONE GL_ONE
		rgbGen entity
		tcMod scale 1 4
		tcMod scroll 0 2
	}
}
markShadow
{
	polygonOffset
	{
		map gfx/damage/shadow.tga
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen exactVertex
	}	
}
projectionShadow
{
	polygonOffset
	deformVertexes projectionShadow
	{
		map *white
		blendFunc GL_ONE GL_ZERO
		rgbGen wave square 0 0 0 0
	}	
}
wake
{
	{
		clampmap sprites/splash.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		tcMod rotate 250
		tcMod stretch sin .9 0.1 0 0.7
		rgbGen wave sin .7 .3 .25 .5
	}	
		{
		clampmap sprites/splash.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		tcMod rotate -230
		tcMod stretch sin .9 0.05 0 0.9
		rgbGen wave sin .7 .3 .25 .4
	}	
}
viewBloodBlend
{
	sort nearest
	{
		map gfx/damage/blood_screen.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identityLighting
		alphaGen vertex
	}
}
waterBubble
{
	sort underwater
	cull none
	entityMergable
	{
		map sprites/bubble.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}
smokePuff
{
	cull none
	entityMergable
	{
		map gfx/misc/smokepuff3.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}
hasteSmokePuff
{
	cull none
	entityMergable
	{
		map gfx/misc/smokepuff3.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}
smokePuffRagePro
{
	cull none
	entityMergable
	{
		map gfx/misc/smokepuffragepro.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
shotgunSmokePuff
{
	cull none
	{
		map gfx/misc/smokepuff2b.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		alphaGen entity		
		rgbGen entity
	}
}
flareShader
{
	cull none
	{
		map gfx/misc/flare.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}
sun
{
	cull none
	{
		map gfx/misc/sun.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}
railDisc
{
	sort nearest
	cull none
	deformVertexes wave 100 sin 0 .5 0 2.4
	{
		clampmap gfx/misc/raildisc_mono2.tga 
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		 tcMod rotate -30
	}
}
railCore
{
	sort nearest
	cull none
	{
		map gfx/misc/railcorethin_mono.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		tcMod scroll -1 0
	}
}
lightningBolt
{
	sort nearest
	cull none
	{
		map gfx/misc/lightning3.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 1 0.5 0 7.1
		 tcMod scale 2 1
		tcMod scroll -5 0
	}
	{
		map gfx/misc/lightning3.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin 1 0.8 0 8.1
		tcMod scale -1.3 -1
		tcMod scroll -7.2 0
	}
}
gfx/misc/tracer
{
	cull none
	{
		map	gfx/misc/tracer2.tga
		blendFunc GL_ONE GL_ONE
	}
}
bloodMark
{
	nopicmip
	polygonOffset
	{
		clampmap gfx/damage/blood_stain.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identityLighting
		alphaGen vertex
	}
}
bloodTrail
{
	nopicmip
	entityMergable
	{
		clampmap gfx/damage/blood_spurt.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}
gfx/damage/bullet_mrk
{
	polygonOffset
	{
		map gfx/damage/bullet_mrk.tga
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen exactVertex
	}
}
gfx/damage/burn_med_mrk
{
	polygonOffset
	{
		map gfx/damage/burn_med_mrk.tga
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen exactVertex
	}
}
gfx/damage/hole_lg_mrk
{
	polygonOffset
	{
		map gfx/damage/hole_lg_mrk.tga
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
		rgbGen exactVertex
	}
}
gfx/damage/plasma_mrk
{
	polygonOffset
	{
		map gfx/damage/plasma_mrk.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}
scoreboardName
{
	nopicmip
	nomipmaps
	{
		clampmap menu/tab/name.tga
		blendFunc blend
	}
}
scoreboardScore
{
	nopicmip
	nomipmaps
	{
		clampmap menu/tab/score.tga
		blendFunc blend
	}
}
scoreboardTime
{
	nopicmip
	nomipmaps
	{
		clampmap menu/tab/time.tga
		blendFunc blend
	}
}
scoreboardPing
{
	nopicmip
	nomipmaps
	{
		clampmap menu/tab/ping.tga
		blendFunc blend
	}
}
gfx/2d/crosshaira
{
	nopicmip
	{
		map gfx/2d/crosshaira.tga
		blendFunc blend
		rgbGen exactVertex
	}
}
gfx/2d/crosshairb
{
	nopicmip
	{
		map gfx/2d/crosshairb.tga
		blendFunc blend
		rgbGen exactVertex
	}
}
gfx/2d/crosshairc
{
	nopicmip
	{
		map gfx/2d/crosshairc.tga
		blendFunc blend
		rgbGen exactVertex
	}
}
gfx/2d/crosshaird
{
	nopicmip
	{
		map gfx/2d/crosshaird.tga
		blendFunc blend
		rgbGen exactVertex
	}
}
gfx/2d/crosshaire
{
	nopicmip
	{
		map gfx/2d/crosshaire.tga
		blendFunc blend
		rgbGen exactVertex
	}
}
gfx/2d/crosshairf
{
	nopicmip
	{
		map gfx/2d/crosshairf.tga
		blendFunc blend
		rgbGen exactVertex
	}
}
gfx/2d/crosshairg
{
	nopicmip
	{
		map gfx/2d/crosshairg.tga
		blendFunc blend
		rgbGen exactVertex
	}
}
gfx/2d/crosshairh
{
	nopicmip
	{
		map gfx/2d/crosshairh.tga
		blendFunc blend
		rgbGen exactVertex
	}
}
gfx/2d/crosshairi
{
	nopicmip
	{
		map gfx/2d/crosshairi.tga
		blendFunc blend
		rgbGen exactVertex
	}
}
gfx/2d/crosshairj
{
	nopicmip
	{
		map gfx/2d/crosshairj.tga
		blendFunc blend
		rgbGen exactVertex
	}
}
gfx/2d/bigchars
{
	nopicmip
	nomipmaps
	{
		map gfx/2d/bigchars.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
gfx/2d/select
{
	nopicmip
	{
		map gfx/2d/select.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen identity
		rgbGen vertex
	}
}
gfx/2d/assault1d
{
	nopicmip
	{
		map gfx/2d/assault1d.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
gfx/2d/armor1h
{
	nopicmip
	{
		map gfx/2d/armor1h.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
gfx/2d/health
{
	nopicmip
	{
		map gfx/2d/health.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
gfx/2d/blank
{
	nopicmip
	{
		map gfx/2d/blank.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}
gfx/2d/numbers/zero_32b
{
	nopicmip
	{
		map gfx/2d/numbers/zero_32b.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
gfx/2d/numbers/one_32b
{
	nopicmip
	{
		map gfx/2d/numbers/one_32b.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
gfx/2d/numbers/two_32b
{
	nopicmip
	{
		map gfx/2d/numbers/two_32b.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
gfx/2d/numbers/three_32b
{
	nopicmip
	{
		map gfx/2d/numbers/three_32b.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
gfx/2d/numbers/four_32b
{
	nopicmip
	{
		map gfx/2d/numbers/four_32b.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
gfx/2d/numbers/five_32b
{
	nopicmip
	{
		map gfx/2d/numbers/five_32b.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
gfx/2d/numbers/six_32b
{
	nopicmip
	{
		map gfx/2d/numbers/six_32b.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
gfx/2d/numbers/seven_32b
{
	nopicmip
	{
		map gfx/2d/numbers/seven_32b.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
gfx/2d/numbers/eight_32b
{
	nopicmip
	{
		map gfx/2d/numbers/eight_32b.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
gfx/2d/numbers/nine_32b
{
	nopicmip
	{
		map gfx/2d/numbers/nine_32b.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
gfx/2d/numbers/minus_32b
{
	nopicmip
	{
		map gfx/2d/numbers/minus_32b.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}
plasmaExplosion
{
	cull disable
	{
		clampmap models/weaphits/plasmaboom.tga
		blendFunc add
		tcMod stretch triangle .6 0.1 0 8
		tcMod rotate 999
		rgbGen wave inversesawtooth 0 1 0 1.5
	}
}
railExplosion
{
	cull disable
	{
		animmap 5 models/weaphits/ring02_1.tga models/weaphits/ring02_2.tga models/weaphits/ring02_3.tga models/weaphits/ring02_4.tga gfx/colors/black.tga
		alphaGen wave inversesawtooth 0 1 0 5
		blendFunc blend
	}
	{
		animmap 5 models/weaphits/ring02_2.tga models/weaphits/ring02_3.tga models/weaphits/ring02_4.tga gfx/colors/black.tga gfx/colors/black.tga
		alphaGen wave sawtooth 0 1 0 5
		blendFunc blend
	}
}
lightningExplosion
{
	cull disable
	{
		animmap 5 models/weaphits/ring02_1.tga models/weaphits/ring02_3.tga models/weaphits/ring02_3.tga gfx/colors/black.tga
		rgbGen wave inversesawtooth 0 1 0 5
		blendFunc add
	}
	{
		animmap 5 models/weaphits/ring02_2.tga models/weaphits/ring02_3.tga gfx/colors/black.tga gfx/colors/black.tga
		rgbGen wave sawtooth 0 1 0 5
		blendFunc add
	}
}
bulletExplosion
{
	cull disable
	{
		animmap 5 models/weaphits/bullet1.tga models/weaphits/bullet2.tga models/weaphits/bullet3.tga gfx/colors/black.tga
		rgbGen wave inversesawtooth 0 1 0 5
		blendFunc add
	}
	{
		animmap 5 models/weaphits/bullet2.tga models/weaphits/bullet3.tga gfx/colors/black.tga gfx/colors/black.tga
		rgbGen wave sawtooth 0 1 0 5
		blendFunc add
	}
}
rocketExplosion
{
	cull disable
	{
		animmap 8 models/weaphits/rlboom/rlboom_1.tga models/weaphits/rlboom/rlboom_2.tga models/weaphits/rlboom/rlboom_3.tga models/weaphits/rlboom/rlboom_4.tga models/weaphits/rlboom/rlboom_5.tga models/weaphits/rlboom/rlboom_6.tga models/weaphits/rlboom/rlboom_7.tga models/weaphits/rlboom/rlboom_8.tga
		rgbGen wave inversesawtooth 0 1 0 8
		blendFunc add
	}
	{
		animmap 8 models/weaphits/rlboom/rlboom_2.tga models/weaphits/rlboom/rlboom_3.tga models/weaphits/rlboom/rlboom_4.tga models/weaphits/rlboom/rlboom_5.tga models/weaphits/rlboom/rlboom_6.tga models/weaphits/rlboom/rlboom_7.tga models/weaphits/rlboom/rlboom_8.tga gfx/colors/black.tga
		rgbGen wave sawtooth 0 1 0 8
		blendFunc add
	}
}
grenadeExplosion
{
	cull disable
	{
		animmap 5 models/weaphits/glboom/glboom_1.tga models/weaphits/glboom/glboom_2.tga models/weaphits/glboom/glboom_3.tga
		rgbGen wave inversesawtooth 0 1 0 5
		blendFunc add
	}
	{
		animmap 5 models/weaphits/glboom/glboom_2.tga models/weaphits/glboom/glboom_3.tga gfx/colors/black.tga
		rgbGen wave sawtooth 0 1 0 5
		blendFunc add
	}
}
bfgExplosion
{
	cull disable
	{
		animmap 5 models/weaphits/bfgboom/bfgboom_1.tga models/weaphits/bfgboom/bfgboom_2.tga models/weaphits/bfgboom/bfgboom_3.tga
		rgbGen wave inversesawtooth 0 1 0 5
		blendFunc add
	}
	{
		animmap 5 models/weaphits/bfgboom/bfgboom_2.tga models/weaphits/bfgboom/bfgboom_3.tga gfx/colors/black.tga
		rgbGen wave sawtooth 0 1 0 5
		blendFunc add
	}
}
bloodExplosion
{
	cull disable
	{
		animmap 5 models/weaphits/blood201.tga models/weaphits/blood202.tga models/weaphits/blood203.tga models/weaphits/blood204.tga models/weaphits/blood205.tga
		blendFunc blend
	}
}