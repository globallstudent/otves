--!strict
-- Cement for 1 m³ of cement-lime masonry mortar and for 1 m³ of its loose sand, kg, by cement
-- grade and mortar grade. Their ratio is the sand in 1 m³ of mortar: 300 / 255 = 1.18 m³ for М100
-- on ПЦ400.
return {
	source = {
		standard = "СП 82-101-98",
		clause = "табл. 4, 5",
		jurisdiction = "RU",
		status = "recommended",
	},
	cement_kg_per_m3 = {
		PC400 = { M50 = 175, M75 = 240, M100 = 300, M150 = 400, M200 = 490 },
		PC500 = { M75 = 195, M100 = 250, M150 = 330, M200 = 410 },
	},
	cement_kg_per_m3_sand = {
		PC400 = { M50 = 140, M75 = 200, M100 = 255, M150 = 350, M200 = 450 },
		PC500 = { M75 = 160, M100 = 205, M150 = 280, M200 = 360 },
	},
}
