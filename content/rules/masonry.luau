--!strict
-- Masonry materials for the job: units and mortar with waste, and the cement in that mortar.
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
		if cement_per_m3 == nil then
			error(`no cement norm for {cement} in mortar {grade}`)
		end

		local units = math.max(0, math.ceil(input.masonry.units * (1 + unit_waste) - ROUNDING))
		local mortar = input.masonry.mortar * (1 + mortar_waste)
		return {
			{ item = "masonry_unit", unit = "pcs", amount = units },
			{ item = "mortar", unit = "m3", amount = mortar },
			{ item = "mortar.cement", unit = "kg", amount = mortar * cement_per_m3 },
		}
	end,
}
