--!strict
-- Meshes in the bed joints where load-bearing walls meet (КМК 2.01.03-19, 3.5.12): one layer at
-- every pitch of height below the wall's top, where the ring beam takes over. Item "mesh.junction"
-- is one 1.5 m mesh with at least 1 cm² of longitudinal bars. Settings: seismic_intensity (points).

-- Fractions this small come from floating-point rounding, not from the plan.
local ROUNDING = 1e-6

return {
	run = function(input, norms, settings)
		local intensity = settings.seismic_intensity
		if intensity == nil then
			intensity = norms.practice.defaults.seismic_intensity
		end
		if type(intensity) ~= "number" then
			error("seismic_intensity must be a number")
		end
		if intensity < 7 then
			return {}
		end
		local pitch_mm = norms.seismic.junction_mesh.pitch[math.floor(intensity)]
		if pitch_mm == nil then
			error(`junction meshes above 9 points follow КМК 3.5.1, which is not modelled yet`)
		end
		local pitch = pitch_mm / 1000

		local min_thickness = norms.practice.walls.load_bearing_min_thickness
		local load_bearing: { [string]: boolean } = {}
		for _, wall in input.walls do
			load_bearing[wall.id] = wall.thickness * 1000 >= min_thickness - ROUNDING
		end

		local meshes = 0
		for _, junction in input.junctions do
			local walls = 0
			for _, id in junction.walls do
				if load_bearing[id] then
					walls += 1
				end
			end
			if walls >= 2 then
				meshes += math.max(0, math.ceil(junction.height / pitch - ROUNDING) - 1)
			end
		end
		if meshes == 0 then
			return {}
		end
		return { { item = "mesh.junction", unit = "pcs", amount = meshes } }
	end,
}
