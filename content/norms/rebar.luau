--!strict
-- Nominal mass of hot-rolled bars per metre, kg, by diameter in millimetres (steel at 7 850 kg/m³).
return {
	source = {
		standard = "ГОСТ 34028-2016",
		clause = "табл. 1",
		jurisdiction = "interstate",
		status = "mandatory",
	},
	kg_per_m = {
		[6] = 0.222,
		[8] = 0.395,
		[10] = 0.617,
		[12] = 0.888,
		[14] = 1.208,
		[16] = 1.578,
		[18] = 1.998,
		[20] = 2.466,
	},
}
