--!strict
-- Seismic detailing of masonry buildings, in millimetres.
return {
	source = {
		standard = "КМК 2.01.03-19",
		clause = "3.5.10–3.5.12, 3.5.15",
		jurisdiction = "UZ",
		status = "mandatory",
	},
	lintel = {
		bearing = 350, -- on each side of the opening
		short_bearing = 250, -- on each side of an opening up to short_span wide
		short_span = 1500,
	},
	-- At every floor and roof, on all walls, the full wall thickness.
	ring_beam = {
		min_height = 150,
		min_concrete_class = 12.5, -- B12.5
		-- Longitudinal bars by design intensity in points; 10 stands for >9 and 9*.
		bars = {
			[7] = { count = 4, diameter = 10 },
			[8] = { count = 4, diameter = 10 },
			[9] = { count = 4, diameter = 12 },
			[10] = { count = 4, diameter = 14 },
		},
	},
	-- Meshes laid in the bed joints where walls meet: at least 1 cm² of longitudinal bars, 1.5 m
	-- long. Above 9 points 3.5.1 may call for denser, stronger meshes; they are not listed here.
	junction_mesh = {
		length = 1500,
		min_bar_area = 100, -- mm²
		pitch = { -- vertical spacing by design intensity in points
			[7] = 700,
			[8] = 700,
			[9] = 500,
		},
	},
}
