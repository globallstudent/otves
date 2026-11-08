package uz.otves.engine

import uz.otves.core.model.ElementId
import uz.otves.schema.CommandKind
import kotlin.uuid.Uuid

/** Why the engine refused a command. The app shows translated text for each value. */
public enum class EngineError {
    NOT_FOUND,
    OFF_PLAN,
    ZERO_LENGTH,
    THICKNESS_NOT_POSITIVE,
    HEIGHT_NOT_POSITIVE,
    WIDTH_NOT_POSITIVE,
    OFFSET_NEGATIVE,
    SILL_NEGATIVE,
    BEARING_NEGATIVE,
    UNIT_SIZE_NOT_POSITIVE,
    JOINT_NEGATIVE,
    RULE_SYNTAX,
    RULE_FAILED,
    RULE_OUTPUT,
    GESTURE_OPEN,
    NOTHING_TO_UNDO,
    NOTHING_TO_REDO,
    CORRUPT_DATA,
    UNSUPPORTED_VERSION,
    INTERNAL,

    /** No wall near the point where an opening was placed. */
    NO_WALL,

    /** An opening wider than its wall. */
    DOES_NOT_FIT,

    /** A code from a newer engine than this app knows. */
    UNKNOWN,
}

public sealed interface CommandResult {
    public data class Done(
        public val created: ElementId?,
        /** What a [EngineCommand.Pick] found. */
        public val picked: ElementId? = null,
    ) : CommandResult

    /** [message] is English context for logs, not for the screen. */
    public data class Failed(
        public val error: EngineError,
        public val message: String,
    ) : CommandResult
}

/** An element's new value, or its deletion when [element] is null. */
public data class ElementChange(
    public val id: ElementId,
    public val element: EngineElement?,
)

/** The take-off after a change: its lines, or why the rules could not run. */
public sealed interface Takeoff {
    public data class Lines(
        public val lines: List<QuantityLine>,
    ) : Takeoff

    public data class Failed(
        public val error: EngineError,
        public val message: String,
    ) : Takeoff
}

/**
 * A group the request appended to the operation log, for the job file (docs/adr/0007). [kind] is
 * the schema's `CommandKind` code and [ops] a finished `OpGroup` buffer; both are stored as they
 * are and only the engine reads them back.
 */
public class LogEntry(
    public val id: Uuid,
    public val kind: Int,
    public val hlc: Long,
    public val device: Uuid,
    /** The group an undo reverts or a redo repeats. */
    public val origin: Uuid?,
    public val ops: ByteArray,
) {
    /** The group reverts an earlier one. */
    public val isUndo: Boolean get() = kind == CommandKind.Undo

    /** The group repeats one that was undone. */
    public val isRedo: Boolean get() = kind == CommandKind.Redo

    override fun equals(other: Any?): Boolean =
        other is LogEntry &&
            id == other.id &&
            kind == other.kind &&
            hlc == other.hlc &&
            device == other.device &&
            origin == other.origin &&
            ops.contentEquals(other.ops)

    override fun hashCode(): Int = id.hashCode()

    override fun toString(): String = "LogEntry(id=$id, kind=$kind, hlc=$hlc, ops=${ops.size} bytes)"
}

public data class EngineResponse(
    /** One per command sent, in order. */
    public val results: List<CommandResult>,
    /** Sorted by id. */
    public val changes: List<ElementChange>,
    public val canUndo: Boolean,
    public val canRedo: Boolean,
    /** Every wall's outline, or null when nothing changed since the previous response. */
    public val outlines: List<WallOutline>? = null,
    /** Null when nothing changed, or while no content is loaded. */
    public val takeoff: Takeoff? = null,
    /** Every opening's outline, sent with [outlines]. */
    public val openings: List<OpeningOutline>? = null,
    /** Groups appended to the operation log, in order. */
    public val log: List<LogEntry> = emptyList(),
)
