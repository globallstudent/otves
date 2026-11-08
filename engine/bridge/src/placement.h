#pragma once

#include <optional>

#include <otves/core/command.h>
#include <otves/core/error.h>
#include <otves/core/model.h>
#include <otves/core/uuid.h>

#include "convert.h"

namespace otves::bridge {

// The opening a tap places: in the wall on the level nearest to the tap within reach, centred on
// the tap and moved along the wall to fit between its ends.
[[nodiscard]] Result<AddOpening> place_opening(const Model& model,
                                               const OpeningPlacement& placement);

// The element a tap picks on its level: an opening or wall under the point, else the nearest
// opening, else the nearest wall within reach. kNotFound when the level does not exist.
[[nodiscard]] Result<std::optional<ElementId>> pick(const Model& model, const Tap& tap);

}  // namespace otves::bridge
