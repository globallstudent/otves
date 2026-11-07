#include <variant>

#include <otves/core/command.h>
#include <otves/core/op.h>
#include <otves/core/overloaded.h>

namespace otves {

CommandKind kind_of(const Command& command) {
  return std::visit(
      Overloaded{
          [](const AddWall&) { return CommandKind::kAddWall; },
          [](const MoveWallEnd&) { return CommandKind::kMoveWallEnd; },
          [](const SetWallThickness&) { return CommandKind::kSetWallThickness; },
          [](const SetWallHeight&) { return CommandKind::kSetWallHeight; },
          [](const SetWallLocationLine&) { return CommandKind::kSetWallLocationLine; },
          [](const RemoveWall&) { return CommandKind::kRemoveWall; },
      },
      command);
}

}  // namespace otves
