package uz.otves.engine

import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm

/** A user intent the engine turns into model changes (engine/core/command.h). */
public sealed interface EngineCommand {
    public data class AddLevel(
        public val level: Level,
    ) : EngineCommand

    public data class SetLevelElevation(
        public val level: ElementId,
        public val elevation: Mm,
    ) : EngineCommand

    public data class SetLevelHeight(
        public val level: ElementId,
        public val height: Mm,
    ) : EngineCommand

    /** Also removes the walls on the level and their openings. */
    public data class RemoveLevel(
        public val level: ElementId,
    ) : EngineCommand

    public data class AddWall(
        public val wall: Wall,
    ) : EngineCommand

    public data class MoveWallEnd(
        public val wall: ElementId,
        public val end: WallEnd,
        public val to: PointMm,
    ) : EngineCommand

    public data class SetWallThickness(
        public val wall: ElementId,
        public val thickness: Mm,
    ) : EngineCommand

    public data class SetWallHeight(
        public val wall: ElementId,
        public val height: Mm,
    ) : EngineCommand

    public data class SetWallLocationLine(
        public val wall: ElementId,
        public val locationLine: LocationLine,
    ) : EngineCommand

    /** Also removes the wall's openings. */
    public data class RemoveWall(
        public val wall: ElementId,
    ) : EngineCommand

    public data class AddOpening(
        public val opening: Opening,
    ) : EngineCommand

    public data class MoveOpening(
        public val opening: ElementId,
        public val offset: Mm,
    ) : EngineCommand

    public data class ResizeOpening(
        public val opening: ElementId,
        public val width: Mm,
        public val height: Mm,
    ) : EngineCommand

    public data class SetOpeningSill(
        public val opening: ElementId,
        public val sill: Mm,
    ) : EngineCommand

    public data class SetOpeningKind(
        public val opening: ElementId,
        public val kind: OpeningKind,
    ) : EngineCommand

    public data class RemoveOpening(
        public val opening: ElementId,
    ) : EngineCommand

    /** Commands until [EndGesture] fold into one undo step, as a drag does. */
    public data object BeginGesture : EngineCommand

    public data object EndGesture : EngineCommand

    public data object Undo : EngineCommand

    public data object Redo : EngineCommand

    /** Replaces the take-off content; a bundle that does not compile keeps the previous one. */
    public data class LoadContent(
        public val norms: List<ContentScript>,
        public val rules: List<ContentScript>,
    ) : EngineCommand
}
