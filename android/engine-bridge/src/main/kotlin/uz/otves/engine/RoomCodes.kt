package uz.otves.engine

import uz.otves.schema.RoomKind as RoomKindCode

// Every room kind with its code in the schema, for the encoder, the decoder and the painter.
private val Codes: List<Pair<RoomKind, Int>> =
    listOf(
        RoomKind.ROOM to RoomKindCode.Room,
        RoomKind.LIVING_ROOM to RoomKindCode.LivingRoom,
        RoomKind.KITCHEN_LIVING to RoomKindCode.KitchenLiving,
        RoomKind.KITCHEN to RoomKindCode.Kitchen,
        RoomKind.DINING to RoomKindCode.Dining,
        RoomKind.BEDROOM to RoomKindCode.Bedroom,
        RoomKind.CHILDREN to RoomKindCode.Children,
        RoomKind.STUDY to RoomKindCode.Study,
        RoomKind.HALL to RoomKindCode.Hall,
        RoomKind.CORRIDOR to RoomKindCode.Corridor,
        RoomKind.BATHROOM to RoomKindCode.Bathroom,
        RoomKind.BATH to RoomKindCode.Bath,
        RoomKind.TOILET to RoomKindCode.Toilet,
        RoomKind.BOILER to RoomKindCode.Boiler,
        RoomKind.STORAGE to RoomKindCode.Storage,
        RoomKind.WARDROBE to RoomKindCode.Wardrobe,
        RoomKind.VERANDA to RoomKindCode.Veranda,
        RoomKind.GARAGE to RoomKindCode.Garage,
        RoomKind.TAMBOUR to RoomKindCode.Tambour,
    )

/** The kind's code in the schema, as the job file and a drawing's room names carry it. */
public val RoomKind.code: Int
    get() = Codes.first { it.first == this }.second

/** The kind a schema code names; null for one from a newer engine than this app knows. */
public fun roomKindOf(code: Long): RoomKind? = Codes.firstOrNull { it.second.toLong() == code }?.first
