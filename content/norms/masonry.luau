--!strict
-- Brick and block masonry, in millimetres.
return {
	source = {
		standard = "СП 70.13330.2012",
		clause = "9.2.4",
		jurisdiction = "RU",
		status = "mandatory",
	},
	joints = {
		bed = 12, -- horizontal joints between courses
		head = 10, -- vertical joints between units in a course
	},
}
