package uz.otves.engine

import uz.otves.core.model.ElementId

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

    /** A code from a newer engine than this app knows. */
    UNKNOWN,
}

public sealed interface CommandResult {
    public data class Done(
        public val created: ElementId?,
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

public data class EngineResponse(
    /** One per command sent, in order. */
    public val results: List<CommandResult>,
    /** Sorted by id. */
    public val changes: List<ElementChange>,
    public val canUndo: Boolean,
    public val canRedo: Boolean,
)
