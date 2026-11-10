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

    /** What the wall is laid from; null clears the choice. */
    public data class SetWallUnit(
        public val wall: ElementId,
        public val unit: UnitSize?,
    ) : EngineCommand

    /** The height of the ring beam on the storey's load-bearing walls; zero removes it. */
    public data class SetLevelRingBeam(
        public val level: ElementId,
        public val height: Mm,
    ) : EngineCommand

    /** The strip footing under the storey's load-bearing walls; null removes it. */
    public data class SetLevelFooting(
        public val level: ElementId,
        public val footing: Footing?,
    ) : EngineCommand

    public data class SetLevelJunctionMeshes(
        public val level: ElementId,
        public val meshes: Boolean,
    ) : EngineCommand

    /** Sets one estimating condition of the job; null returns it to the norm default. */
    public data class SetCondition(
        public val condition: Condition,
        public val value: Int?,
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

    /**
     * An opening where the user tapped: in the wall on [level] nearest to [at] within [reach],
     * centred on the tap and moved along the wall to fit. Runs as [AddOpening]; fails with
     * [EngineError.NO_WALL] or [EngineError.DOES_NOT_FIT].
     */
    public data class PlaceOpening(
        public val level: ElementId,
        public val at: PointMm,
        public val reach: Mm,
        public val width: Mm,
        public val height: Mm,
        public val sill: Mm,
        public val kind: OpeningKind,
    ) : EngineCommand

    /**
     * Replaces the open model with a saved job's [elements] and starts an empty log with no undo
     * or redo (docs/adr/0007). [clock] is the newest stamp in the job's log, so later stamps sort
     * after it. A broken set fails with [EngineError.CORRUPT_DATA] and changes nothing.
     */
    public data class OpenModel(
        public val elements: Map<ElementId, EngineElement>,
        public val clock: Long,
    ) : EngineCommand

    /**
     * What the user tapped on [level]: an opening or wall under [at], else the nearest within
     * [reach], openings first. Changes nothing; the result's `picked` names the element.
     */
    public data class Pick(
        public val level: ElementId,
        public val at: PointMm,
        public val reach: Mm,
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
