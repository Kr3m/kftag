freezeShader
{
	nopicmip
	deformVertexes wave 100 sin 2 0 0 0
	{
		map gfx/misc/freeze.jpg
		blendfunc add
		rgbGen const ( 0.95 1 1 )
		tcMod scroll 0.1 0.1
		tcGen environment
	}
}

freezeShader1
{
	nopicmip
	deformvertexes wave 100 sin 3 0 0 0
	{
		map textures/effects/envmap.tga
		blendfunc gl_one gl_one
		tcgen environment
	}
}

freezeMarkShader
{
	nopicmip
	polygonoffset
	{
		clampmap gfx/damage/freeze_stain.tga
		blendfunc gl_src_alpha gl_one_minus_src_alpha
		rgbgen identitylighting
		alphagen vertex
	}
}
