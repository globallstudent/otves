#include "boxes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <utility>

#include <otves/core/units.h>
#include <otves/drafting/plan.h>
#include <otves/geom/vec2.h>

namespace otves::drafting {
namespace {

// Whether the boxes lie apart along `axis`, a unit direction.
bool apart_along(geom::Vec2 axis, const Box& a, const Box& b) {
  const auto reach = [axis](const Box& box) {
    return (box.half_along * std::abs(geom::dot(box.along, axis))) +
           (box.half_across * std::abs(geom::dot(geom::perp(box.along), axis)));
  };
  return std::abs(geom::dot(b.at - a.at, axis)) > reach(a) + reach(b);
}

}  // namespace

double label_width(Mm length, bool exact, TextSize text) {
  const auto digits = static_cast<int>(std::to_string(length.value()).size());
  return (digits + (exact ? 0 : kAboutDigits)) * text.digit;
}

double width_of(const Text& written, TextSize text) {
  switch (written.kind) {
    case TextKind::kRoomName:
      return std::min(written.fit, kNameDigits * text.digit);
    case TextKind::kRoomArea:
      return kAreaDigits * text.digit;
    case TextKind::kWindowMark:
    case TextKind::kDoorMark:
    case TextKind::kEmptyMark:
      return kMarkDigits * text.digit;
    case TextKind::kAboutLength:
      return label_width(Mm{written.value}, false, text);
    case TextKind::kLength:
    case TextKind::kOverall:
    case TextKind::kClear:
    case TextKind::kPier:
    case TextKind::kOpeningWidth:
      break;
  }
  return label_width(Mm{written.value}, true, text);
}

bool meet(const Box& a, const Box& b) {
  return !apart_along(a.along, a, b) && !apart_along(geom::perp(a.along), a, b) &&
         !apart_along(b.along, a, b) && !apart_along(geom::perp(b.along), a, b);
}

Box box_of(const Text& written, TextSize text) {
  return {.at = written.at,
          .along = written.along,
          .half_along = width_of(written, text) / 2,
          .half_across = text.height / 2};
}

bool crosses(const Box& box, geom::Vec2 from, geom::Vec2 to) {
  double enters = 0.0;
  double leaves = 1.0;
  const std::array sides{std::pair{box.along, box.half_along},
                         std::pair{geom::perp(box.along), box.half_across}};
  for (const auto& [axis, half] : sides) {
    const double start = geom::dot(from - box.at, axis);
    const double run = geom::dot(to - from, axis);
    if (run == 0.0) {
      if (std::abs(start) > half) {
        return false;
      }
      continue;
    }
    const double one = (-half - start) / run;
    const double other = (half - start) / run;
    enters = std::max(enters, std::min(one, other));
    leaves = std::min(leaves, std::max(one, other));
  }
  return enters <= leaves;
}

bool crosses(const Box& box, const Arc& arc) {
  const geom::Vec2 to_centre = arc.centre - box.at;
  const double along = std::abs(geom::dot(to_centre, box.along));
  const double across = std::abs(geom::dot(to_centre, geom::perp(box.along)));
  // The nearest and the farthest point of the box from the arc's centre, squared.
  const double near_along = std::max(along - box.half_along, 0.0);
  const double near_across = std::max(across - box.half_across, 0.0);
  const double nearest = (near_along * near_along) + (near_across * near_across);
  const double farthest = ((along + box.half_along) * (along + box.half_along)) +
                          ((across + box.half_across) * (across + box.half_across));
  const geom::Vec2 radius = arc.from - arc.centre;
  const double reach = geom::dot(radius, radius);
  if (nearest > reach || farthest < reach) {
    return false;
  }
  // Counter-clockwise from `from` to `to`.
  const geom::Vec2 middle = box.at - arc.centre;
  return geom::cross(radius, middle) >= 0.0 && geom::cross(middle, arc.to - arc.centre) >= 0.0;
}

}  // namespace otves::drafting
