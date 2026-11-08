package uz.otves.core.model

import kotlin.uuid.Uuid

/** The id of a model element: a UUIDv7 the engine creates, unique across devices. */
@JvmInline
public value class ElementId(
    public val uuid: Uuid,
)
