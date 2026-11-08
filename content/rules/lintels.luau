--!strict
-- Precast bar lintels (ПБ) over openings, counted by the length each needs. A lintel of the wall's
-- full thickness is several bars side by side; item "lintel.bar.l22" is a bar at least 2.2 m long.

-- Fractions this small come from floating-point rounding, not from the plan.
local ROUNDING = 1e-6

return {
	run = function(input, norms, settings)
		local bar = norms.practice.lintel
		local pitch = (bar.width + bar.joint) / 1000
		local counts: { [string]: number } = {}
		for _, lintel in input.lintels do
			local bars = math.ceil(lintel.thickness / pitch - ROUNDING)
			local item = `lintel.bar.l{math.ceil(lintel.length * 10 - ROUNDING)}`
			counts[item] = (counts[item] or 0) + bars
		end
		local lines = {}
		for item, count in counts do
			table.insert(lines, { item = item, unit = "pcs", amount = count })
		end
		return lines
	end,
}
