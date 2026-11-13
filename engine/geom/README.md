# engine/geom

Pure geometry over wall parameters: plan outlines in metres, with corners and junctions joined. The same input gives the same output bits on every platform.

![Three wall joins: a mitred corner, a partition cut back to the face of the wall it meets, and three walls tiling a shared point](docs/joins.svg)

## Entry points

| API | Use |
|---|---|
| `join_walls(walls)` | Outlines of all walls on one level, sorted by id; junctions where walls meet (corners, T, several walls at a point), sorted by point; join issues |
| `signed_area(outline)` | Area in m², positive for a counter-clockwise outline |
| `hit_wall(walls, at, reach)` | The wall nearest to a plan point within a reach, and how far along its location line the point lies |
| `hit_opening(openings, at, reach)` | The opening nearest to a plan point within a reach |
| `opening_outline(wall, opening)` | The rectangle an opening cuts through its wall on the plan, face to face |
| `aim(walls, below, touch)` | Where a wall being drawn would end under the finger, its length, the wall ends it lined up with and whether it meets the plan there |
| `aim(walls, measured)` | Where a wall of a typed length ends, turning left, straight or right from the wall before it |
| `resize_wall(walls, wall, length, fixed)` | Every wall end that moves when a wall gets a length, so what is joined stays joined |
| `straighten(walls, below, sketch, like)` | The walls a stroke drawn with a finger means: corners found, legs laid on the axes, a loop closed, joined to the plan as `aim` joins a wall |

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

## Editing

Editing reads the walls as a graph by the definition the joins use ([ADR 0011](../../docs/adr/0011-plan-editing-in-the-engine.md)): ends at one point are a joint, and an end inside another wall's body stands against that wall.

| Question | Answer |
|---|---|
| A touch near a wall end, of this level or the one below | That end |
| A touch elsewhere | The wall turns onto the nearer axis, always with the 90° lock and otherwise within about 6° of it; each free coordinate lines up with a wall end within reach or falls on the grid |
| A touch on or beside a wall's body | The point moves onto that wall's line, so the new wall meets it along its length |
| A typed length for a wall | Its fixed end stays and its far joint moves along it. Unless one is named, the fixed end is the one joined to other walls when the other is free, so a partition grows from the wall it stands against; else the start. A wall at a moving point stretches if it runs along the move and otherwise goes whole, with the ends standing against it; a wall reaching the fixed end turns about it |
| A length that would make a joined wall vanish or turn back, or leave a wall end off the wall it stands against | Refused with `kBreaksJoin`; nothing moves |

## Sketches

A stroke is read in six steps ([ADR 0012](../../docs/adr/0012-a-sketch-becomes-walls-in-the-engine.md)). The reach is how far from the finger still counts as touched, in plan millimetres at the zoom the stroke was drawn at.

```mermaid
flowchart LR
  P["points"] --> C["1 corners<br/>further than the reach<br/>from the line between"]
  C --> L["2 lines<br/>a held leg lies at the<br/>middle value of its points"]
  L --> O["3 loop<br/>ends within two<br/>reaches of the start"]
  O --> E["4 wall ends<br/>a corner within reach<br/>is that end"]
  E --> A["5 aim<br/>each leg, the legs<br/>before it as walls"]
  A --> R["6 runs<br/>legs in one line<br/>are one wall"]
```

| Stroke | Walls |
|---|---|
| A rough loop | The room, closed on its first corner; begun in the middle of a side, that side is still one wall |
| Rounded corners | Walls where the stroke ran, not where the corners were cut |
| A line that bows | One wall |
| From a wall to a wall | A partition meeting both along their length |
| Ending near a wall end | A wall to that end; the corner before it moves so the wall stays on its axis |
| A tap, or a stroke within reach of its first point | Refused with `kZeroLength` |

## Invariants

- Outlines are counter-clockwise. Joined outlines share edges and never overlap, so their areas add up to the masonry area.
- Overlaps at shared points, at ends inside a wall and where location lines cross are reported. Walls overlapping only side by side are not checked yet.
- Ends join only when their points are equal to the millimetre; `aim` puts a touched point on them, and `resize_wall` moves every end of a joint together.
- Directions are sorted and tested for parallel on integer millimetre steps, so which walls join never depends on floating-point rounding.
- Coordinates use only correctly rounded operations (`+ − × ÷ √`) with `-ffp-contract=off`, so arm64 and x86-64 give the same bits. A golden test checks this.
- Input walls pass `validate()` and have unique ids. Input order does not change the result.
- A stroke's points are checked to lie on the plan before any arithmetic, and its reach and grid are held within the plan's size.
