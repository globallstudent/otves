#pragma once

#include <optional>
#include <span>
#include <vector>

#include <otves/core/build.h>
#include <otves/core/uuid.h>
#include <otves/core/wall.h>
#include <otves/takeoff/masonry.h>
#include <otves/takeoff/wall_quantities.h>

// Walls of several layers (docs/adr/0014): each masonry layer is measured as a wall of its own,
// insulation by the face it covers.
namespace otves::takeoff {

// A wall with what it is built of, from its outer face in. `wall` is the wall as geometry reads
// it: its thickness is the sum of the layers.
struct BuiltWall {
  ElementId id;
  Wall wall;
  std::vector<BuiltLayer> layers;
};

struct Layers {
  // A measured wall for every masonry layer, in the order of the walls and from the outer face
  // in; `id` is the wall's.
  std::vector<WallQuantities> masonry;
  // The unit each of them is laid from, absent until chosen.
  std::vector<std::optional<MasonryUnit>> units;
  std::vector<Insulation> insulation;  // one per thickness, sorted
};

// Splits measured walls into their layers. A wall that is one masonry layer is measured as it
// was. Otherwise a layer is as long as its strip of the wall's joined outline, so the outer
// layer of a corner is longer than the inner one; openings and lintels cut every layer alike.
// A gap takes nothing.
// Precondition: `measurement` is measure_levels of these walls, and every wall passes validate().
[[nodiscard]] Layers measure_layers(std::span<const BuiltWall> walls,
                                    const Measurement& measurement);

}  // namespace otves::takeoff
