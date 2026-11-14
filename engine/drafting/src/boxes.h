#pragma once

#include <otves/core/units.h>
#include <otves/drafting/plan.h>
#include <otves/geom/vec2.h>

// The room a text takes on the plan, to keep texts off each other. The engine does not know
// the words the app writes, so a name, an area and a mark are taken to be so many digits wide.
namespace otves::drafting {

// «≈» and the space after it are about as wide as two digits.
inline constexpr int kAboutDigits = 2;

// A room's name is taken to be at most this many digits wide, its area this many and an
// opening's mark this many.
inline constexpr double kNameDigits = 16.0;
inline constexpr double kAreaDigits = 7.0;
inline constexpr double kMarkDigits = 5.0;

// How wide a wall's length is written.
[[nodiscard]] double label_width(Mm length, bool exact, TextSize text);

// How wide a written text is along its direction, at most.
[[nodiscard]] double width_of(const Text& written, TextSize text);

// A text as the box it fills: its middle, its direction and half its size along and across.
struct Box {
  geom::Vec2 at;
  geom::Vec2 along;
  double half_along = 0.0;
  double half_across = 0.0;
};

// Two boxes meet unless a line along a side of one of them parts them.
[[nodiscard]] bool meet(const Box& a, const Box& b);

// The box a written text fills.
[[nodiscard]] Box box_of(const Text& written, TextSize text);

// Whether the segment passes through the box.
[[nodiscard]] bool crosses(const Box& box, geom::Vec2 from, geom::Vec2 to);

// Whether the arc passes through the box: the box reaches from inside its circle to outside it,
// and its middle lies in the arc's turn. Right for a box that is small beside the arc.
[[nodiscard]] bool crosses(const Box& box, const Arc& arc);

}  // namespace otves::drafting
