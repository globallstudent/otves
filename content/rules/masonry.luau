--!strict
-- Masonry materials for the job: the units the walls are laid from, one line per unit size
-- ("masonry_unit.250x120x65", millimetres as laid), and mortar with waste. The mortar is mixed on
-- site, so it comes with what is bought for it: cement by weight and in whole bags
-- ("mortar.cement.bag50"), and loose sand. An item is made of the items named after it and a dot,
-- so the app prices those instead.
-- Settings: unit_waste and mortar_waste (fractions), cement ("PC400", "PC500"),
-- mortar_grade ("M50" to "M200"). Missing settings fall back to norms.practice.

-- Fractions of a piece this small come from floating-point rounding, not from the plan.
local ROUNDING = 1e-6

local function setting(settings, name: string, default)
	local value = settings[name]
	if value == nil then
		return default
	end
	return value
end

local function fraction(settings, name: string, default: number): number
	local value = setting(settings, name, default)
	if type(value) ~= "number" or value < 0 or value >= 1 then
		error(`{name} must be a fraction from 0 to 1`)
	end
	return value
end

return {
	run = function(input, norms, settings)
		local practice = norms.practice
		local unit_waste = fraction(settings, "unit_waste", practice.waste.masonry_unit)
		local mortar_waste = fraction(settings, "mortar_waste", practice.waste.mortar)
		local cement = setting(settings, "cement", practice.defaults.cement)
		local grade = setting(settings, "mortar_grade", practice.defaults.mortar_grade)

		local by_grade = norms.mortar.cement_kg_per_m3[cement]
		local cement_per_m3 = if by_grade then by_grade[grade] else nil
		local by_grade_sand = norms.mortar.cement_kg_per_m3_sand[cement]
		local cement_per_m3_sand = if by_grade_sand then by_grade_sand[grade] else nil
		if cement_per_m3 == nil or cement_per_m3_sand == nil then
			error(`no cement norm for {cement} in mortar {grade}`)
		end

		local lines = {}
		for _, of_unit in input.masonry.by_unit do
			table.insert(lines, {
				item = `masonry_unit.{of_unit.length}x{of_unit.width}x{of_unit.height}`,
				unit = "pcs",
				amount = math.max(0, math.ceil(of_unit.units * (1 + unit_waste) - ROUNDING)),
			})
		end
		if #lines == 0 then
			return {} -- no wall has a unit yet
		end
		local mortar = input.masonry.mortar * (1 + mortar_waste)
		local cement_kg = mortar * cement_per_m3
		local bag = practice.cement_bag_kg
		table.insert(lines, { item = "mortar", unit = "m3", amount = mortar })
		table.insert(lines, { item = "mortar.cement", unit = "kg", amount = cement_kg })
		table.insert(lines, {
			item = `mortar.cement.bag{bag}`,
			unit = "pcs",
			amount = math.max(0, math.ceil(cement_kg / bag - ROUNDING)),
		})
		table.insert(lines, { item = "mortar.sand", unit = "m3", amount = mortar * cement_per_m3 / cement_per_m3_sand })
		return lines
	end,
}
