#include <variant>

#include <otves/core/command.h>
#include <otves/core/op.h>
#include <otves/core/overloaded.h>

namespace otves {

CommandKind kind_of(const Command& command) {
  return std::visit(
      Overloaded{
          [](const AddLevel&) { return CommandKind::kAddLevel; },
          [](const SetLevelElevation&) { return CommandKind::kSetLevelElevation; },
          [](const SetLevelHeight&) { return CommandKind::kSetLevelHeight; },
          [](const RemoveLevel&) { return CommandKind::kRemoveLevel; },
          [](const AddWall&) { return CommandKind::kAddWall; },
          [](const MoveWallEnd&) { return CommandKind::kMoveWallEnd; },
          [](const SetWallThickness&) { return CommandKind::kSetWallThickness; },
          [](const SetWallHeight&) { return CommandKind::kSetWallHeight; },
          [](const SetWallLocationLine&) { return CommandKind::kSetWallLocationLine; },
          [](const RemoveWall&) { return CommandKind::kRemoveWall; },
          [](const AddOpening&) { return CommandKind::kAddOpening; },
          [](const MoveOpening&) { return CommandKind::kMoveOpening; },
          [](const ResizeOpening&) { return CommandKind::kResizeOpening; },
          [](const SetOpeningSill&) { return CommandKind::kSetOpeningSill; },
          [](const SetOpeningKind&) { return CommandKind::kSetOpeningKind; },
          [](const RemoveOpening&) { return CommandKind::kRemoveOpening; },
          [](const SetWallUnit&) { return CommandKind::kSetWallUnit; },
          [](const SetLevelRingBeam&) { return CommandKind::kSetLevelRingBeam; },
          [](const SetLevelJunctionMeshes&) { return CommandKind::kSetLevelJunctionMeshes; },
          [](const SetCondition&) { return CommandKind::kSetCondition; },
      },
      command);
}

}  // namespace otves
