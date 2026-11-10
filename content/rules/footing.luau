--!strict
-- Strip footing (ленточный фундамент) under the load-bearing walls of the storeys that ask for
-- one (wall.footing_width and wall.footing_depth, metres): concrete with placement losses,
-- longitudinal bars at the norm's minimum and never fewer than practice's, stirrups at the pitch
-- the norm allows, and formwork on both sides. A guide to quantities, not a structural design.

-- Fractions this small come from floating-point rounding, not from the plan.
local ROUNDING = 1e-6
local PI = 3.141592653589793

local function concrete_item(class: number): string
	return "concrete.b" .. (string.gsub(tostring(class), "%.", "_"))
end

return {
	run = function(input, norms, settings)
		local practice = norms.practice.footing
		local detail = norms.reinforcement
		local bar_area = PI * practice.bar * practice.bar / 4 -- mm²
		local waste = norms.practice.waste.rebar

		local volume, sides, bars_kg, stirrups_kg = 0, 0, 0, 0
		for _, wall in input.walls do
			local bearing = wall.thickness * 1000 >= norms.practice.walls.load_bearing_min_thickness - ROUNDING
			if bearing and wall.footing_width > 0 and wall.footing_depth > 0 then
				local width, depth = wall.footing_width * 1000, wall.footing_depth * 1000 -- mm
				local cover = detail.cover_without_blinding
				local working = depth - cover
				if working <= 0 or width <= 2 * cover then
					error(`a footing must be wider than {2 * cover} mm and deeper than {cover} mm`)
				end
				volume += wall.length * wall.footing_width * wall.footing_depth
				sides += 2 * wall.length * wall.footing_depth

				local needed = math.ceil(detail.min_longitudinal * width * working / bar_area - ROUNDING)
				local bars = math.max(practice.min_bars, needed + needed % 2) -- in pairs, top and bottom
				bars_kg += wall.length * bars * norms.rebar.kg_per_m[practice.bar]

				local pitch = math.min(detail.stirrup_pitch_of_depth * working, detail.stirrup_max_pitch) / 1000
				local count = math.ceil(wall.length / pitch - ROUNDING) + 1
				local around = 2 * ((width - 2 * cover) + (depth - 2 * cover)) / 1000
				stirrups_kg += count * around * norms.rebar.kg_per_m[practice.stirrup]
			end
		end
		if volume == 0 then
			return {}
		end
		return {
			{ item = concrete_item(practice.concrete_class), unit = "m3", amount = volume * (1 + norms.losses.concrete.foundation) },
			{ item = `rebar.a500c.d{practice.bar}`, unit = "kg", amount = bars_kg * (1 + waste) },
			{ item = `rebar.a240.d{practice.stirrup}`, unit = "kg", amount = stirrups_kg * (1 + waste) },
			{ item = "formwork", unit = "m2", amount = sides },
		}
	end,
}
