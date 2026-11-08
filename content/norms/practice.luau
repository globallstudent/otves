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
		rebar = 0.07, -- a lap of about 70 d (СП 63, A500 in B15; docs/research) on each 11.7 m bar
	},
	defaults = {
		cement = "PC400", -- «Цемент» default in docs/research
		mortar_grade = "M100",
		seismic_intensity = 9, -- the upper value of the 8–9 points decree 727 set for the largest cities
	},
	-- Precast bar lintel 2ПБ (ГОСТ 948-2016, table 1): 120 × 140 mm, laid side by side across the
	-- wall with 10 mm joints. Millimetres.
	lintel = {
		height = 140,
		width = 120,
		joint = 10,
	},
	ring_beam = {
		concrete_class = 15, -- B15, a common ready-mix; the КМК minimum is B12.5
		rebar = "a500c",
		-- Thinner walls are partitions (КМК 3.9.7), which get bed-joint bars instead of a ring beam,
		-- until walls carry a load-bearing flag. Millimetres.
		min_wall_thickness = 250,
	},
}
