--!strict
-- Normative placement losses of concrete mix, as fractions of the geometric volume.
return {
	source = {
		standard = "Сборник типовых норм потерь материальных ресурсов (к РДС 82-202-96)",
		clause = "табл. 1, п. 42",
		jurisdiction = "RU",
		status = "recommended",
	},
	concrete = {
		foundation = 0.018,
		slab = 0.018,
		beam = 0.028,
		column = 0.028,
		ribbed_slab = 0.035,
	},
}
