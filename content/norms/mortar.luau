--!strict
-- Cement for 1 m³ of cement-lime masonry mortar, kg, by cement grade and mortar grade.
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
}
