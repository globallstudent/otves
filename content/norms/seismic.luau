--!strict
-- Seismic detailing of masonry buildings, in millimetres.
return {
	source = {
		standard = "КМК 2.01.03-19",
		clause = "3.5.15",
		jurisdiction = "UZ",
		status = "mandatory",
	},
	lintel = {
		bearing = 350, -- on each side of the opening
		short_bearing = 250, -- on each side of an opening up to short_span wide
		short_span = 1500,
	},
}
