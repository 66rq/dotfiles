static const float rootcolor[] = COLOR(0x{color0.strip}ff);

static const float fullscreen_bg[] = {{
	((0x{color0.strip} >> 16) & 0xFF) / 255.0f,
	((0x{color0.strip} >> 8) & 0xFF) / 255.0f,
	(0x{color0.strip} & 0xFF) / 255.0f,
	1.0f
}};

static uint32_t colors[][3] = {{
	/*               fg          bg          border    */
	[SchemeNorm] = {{
		0x{color15.strip}ff,
		0x{color0.strip}ff,
		0x{color8.strip}ff
	}},

	[SchemeSel] = {{
		0x{color15.strip}ff,
		0x{color4.strip}ff,
		0x{color12.strip}ff
	}},

	[SchemeUrg] = {{
		0x{color15.strip}ff,
		0x{color1.strip}ff,
		0x{color9.strip}ff
	}},
}};
