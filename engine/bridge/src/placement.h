#pragma once

#include <otves/core/command.h>
#include <otves/core/error.h>
#include <otves/core/model.h>

#include "convert.h"

namespace otves::bridge {

// The opening a tap places: in the wall on the level nearest to the tap within reach, centred on
// the tap and moved along the wall to fit between its ends.
[[nodiscard]] Result<AddOpening> place_opening(const Model& model,
                                               const OpeningPlacement& placement);

}  // namespace otves::bridge
