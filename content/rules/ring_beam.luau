--!strict
-- Antiseismic ring beam (армопояс) on top of the load-bearing walls of the storeys that ask for
-- one (wall.ring_beam, its height in metres): concrete with placement losses and its longitudinal
-- bars with laps. Wall height is the masonry, so the beam sits above it. Settings:
-- seismic_intensity (points), ring_beam_concrete (class, 15 for B15).

-- Fractions this small come from floating-point rounding, not from the plan.
local ROUNDING = 1e-6

local function setting(settings, name: string, default)
	local value = settings[name]
	if value == nil then
		return default
	end
	return value
end

local function at_least(settings, name: string, default: number, minimum: number, unit: string): number
	local value = setting(settings, name, default)
	if type(value) ~= "number" or value < minimum then
		error(`{name} must be at least {minimum}{unit}`)
	end
	return value
end

-- B12.5 is "concrete.b12_5".
local function concrete_item(class: number): string
	return "concrete.b" .. (string.gsub(tostring(class), "%.", "_"))
end

return {
	run = function(input, norms, settings)
		local seismic = norms.seismic.ring_beam
		local practice = norms.practice.ring_beam
		local intensity = at_least(settings, "seismic_intensity", norms.practice.defaults.seismic_intensity, 0, " points")
		local class = at_least(settings, "ring_beam_concrete", practice.concrete_class, seismic.min_concrete_class, "")
		-- КМК 2.01.03-19 asks for ring beams from 7 points; a beam chosen below that is detailed as at 7.
		local bars = seismic.bars[math.clamp(math.floor(intensity), 7, 10)]

		local volume, length = 0, 0
		for _, wall in input.walls do
			local bearing = wall.thickness * 1000 >= norms.practice.walls.load_bearing_min_thickness - ROUNDING
			if bearing and wall.ring_beam > 0 then
				if wall.ring_beam * 1000 < seismic.min_height - ROUNDING then
					error(`a ring beam must be at least {seismic.min_height} mm high`)
				end
				volume += wall.footprint_area * wall.ring_beam
				length += wall.length
			end
		end
		if length == 0 then
			return {}
		end

		local concrete = volume * (1 + norms.losses.concrete.beam)
		local steel = length * bars.count * norms.rebar.kg_per_m[bars.diameter] * (1 + norms.practice.waste.rebar)
		return {
			{ item = concrete_item(class), unit = "m3", amount = concrete },
			{ item = `rebar.{practice.rebar}.d{bars.diameter}`, unit = "kg", amount = steel },
		}
	end,
}
