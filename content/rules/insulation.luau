--!strict
-- Insulation inside the walls: one line per thickness ("insulation.100", millimetres), by the
-- face it covers, net of openings and lintels. No allowance is added: docs/research has no figure
-- for offcuts, so the builder rounds up to whole packs when buying.

return {
	run = function(input, norms, settings)
		local lines = {}
		for _, layer in input.insulation do
			if layer.area > 0 then
				table.insert(lines, { item = `insulation.{layer.thickness}`, unit = "m2", amount = layer.area })
			end
		end
		return lines
	end,
}
