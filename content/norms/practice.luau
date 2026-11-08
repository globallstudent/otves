--!strict
-- Allowances builders use in practice. No binding source was found (docs/research, «Запас, %»),
-- so every value is a default the user can change in the job settings.
return {
	source = {
		standard = "practice",
		status = "assumption",
	},
	waste = {
		masonry_unit = 0.03, -- research range 2–5 %
		mortar = 0.07, -- research range 5–10 %
	},
	defaults = {
		cement = "PC400", -- «Цемент» default in docs/research
		mortar_grade = "M100",
	},
	-- Precast bar lintel 2ПБ (ГОСТ 948-2016, table 1): 120 × 140 mm, laid side by side across the
	-- wall with 10 mm joints. Millimetres.
	lintel = {
		height = 140,
		width = 120,
		joint = 10,
	},
}
