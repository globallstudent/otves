# engine/geom

Pure geometry over wall parameters: plan outlines in metres, with corners and junctions joined. The same input gives the same output bits on every platform.

![Three wall joins: a mitred corner, a partition cut back to the face of the wall it meets, and three walls tiling a shared point](docs/joins.svg)

## Entry points

| API | Use |
|---|---|
| `join_walls(walls)` | Outlines of all walls on one level, sorted by id, plus join issues |
| `signed_area(outline)` | Area in m², positive for a counter-clockwise outline |

## Joins

| Case | Result |
|---|---|
| Two ends at one point | Faces meet at a mitre (угловое сопряжение) |
| Three or more ends at one point | Each outline runs through the point, so the walls tile it with no gap |
| Ends inside another wall, at most one on each side | Cut back to that wall's face (примыкание); the other wall stays whole |
| Two walls continuing in a straight line | Both ends square, so walls of different thickness step |
| Mitre longer than `kMaxMitre` (10) thicknesses | Ends square, issue `kSharpAngle` |
| Two walls along one line from one point, or a wall inside another | Issue `kOverlap` |
| Two ends inside a wall from one side | Issue `kOverlap` |
| Walls crossing away from their ends | Issue `kOverlap`; split at the crossing, they join as four ends at one point |

## Invariants

- Outlines are counter-clockwise. Joined outlines share edges and never overlap, so their areas add up to the masonry area.
- Overlaps at shared points, at ends inside a wall and where location lines cross are reported. Walls overlapping only side by side are not checked yet.
- Ends join only when their points are equal to the millimetre; the editor snaps.
- Directions are sorted and tested for parallel on integer millimetre steps, so which walls join never depends on floating-point rounding.
- Coordinates use only correctly rounded operations (`+ − × ÷ √`) with `-ffp-contract=off`, so arm64 and x86-64 give the same bits. A golden test checks this.
- Input walls pass `validate()` and have unique ids. Input order does not change the result.
