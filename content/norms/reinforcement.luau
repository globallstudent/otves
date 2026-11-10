--!strict
-- Detailing of reinforced concrete, in millimetres and fractions.
return {
	source = {
		standard = "СП 63.13330.2018",
		clause = "10.3.2 (табл. 10.1), 10.3.6, 10.3.13",
		jurisdiction = "RU",
		status = "mandatory",
	},
	-- Cover of the bottom bars of a monolithic foundation cast without blinding.
	cover_without_blinding = 70,
	-- Longitudinal bars are at least this share of the section's width times its working depth.
	min_longitudinal = 0.001,
	-- Stirrups stand no further apart than this share of the working depth, nor than max_pitch.
	stirrup_pitch_of_depth = 0.5,
	stirrup_max_pitch = 300,
}
