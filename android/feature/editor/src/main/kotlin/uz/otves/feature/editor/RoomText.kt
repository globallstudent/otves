package uz.otves.feature.editor

import android.content.res.Resources
import androidx.compose.runtime.Composable
import androidx.compose.ui.platform.LocalResources
import uz.otves.engine.OpeningKind
import uz.otves.engine.RoomKind
import uz.otves.engine.TextKind
import uz.otves.engine.roomKindOf
import java.math.BigDecimal
import java.math.RoundingMode

/** A room's name in the builder's language. */
internal fun roomKindName(
    resources: Resources,
    kind: RoomKind,
): String =
    resources.getString(
        when (kind) {
            RoomKind.ROOM -> R.string.room_kind_room
            RoomKind.LIVING_ROOM -> R.string.room_kind_living_room
            RoomKind.KITCHEN_LIVING -> R.string.room_kind_kitchen_living
            RoomKind.KITCHEN -> R.string.room_kind_kitchen
            RoomKind.DINING -> R.string.room_kind_dining
            RoomKind.BEDROOM -> R.string.room_kind_bedroom
            RoomKind.CHILDREN -> R.string.room_kind_children
            RoomKind.STUDY -> R.string.room_kind_study
            RoomKind.HALL -> R.string.room_kind_hall
            RoomKind.CORRIDOR -> R.string.room_kind_corridor
            RoomKind.BATHROOM -> R.string.room_kind_bathroom
            RoomKind.BATH -> R.string.room_kind_bath
            RoomKind.TOILET -> R.string.room_kind_toilet
            RoomKind.BOILER -> R.string.room_kind_boiler
            RoomKind.STORAGE -> R.string.room_kind_storage
            RoomKind.WARDROBE -> R.string.room_kind_wardrobe
            RoomKind.VERANDA -> R.string.room_kind_veranda
            RoomKind.GARAGE -> R.string.room_kind_garage
            RoomKind.TAMBOUR -> R.string.room_kind_tambour
        },
    )

/** A floor's area to a tenth of a square metre, as plans give it: «25,1 м²». */
internal fun areaText(
    resources: Resources,
    squareMetres: Double,
): String = resources.getString(R.string.room_area_value, tenths(BigDecimal.valueOf(squareMetres)))

/** A length in metres to a tenth: «18,5 м». */
internal fun metresText(
    resources: Resources,
    metres: Double,
): String = resources.getString(R.string.room_metres_value, tenths(BigDecimal.valueOf(metres)))

// One decimal after a comma, as builders write.
private fun tenths(value: BigDecimal): String = value.setScale(1, RoundingMode.HALF_UP).toPlainString().replace('.', ',')

/**
 * What a drawing's room texts say (docs/adr/0015): the engine gives a room's kind as its code and a
 * floor's area in cm².
 */
internal class RoomLabels(
    private val resources: Resources,
) {
    /** The room's name; null for a kind from a newer engine than this app knows. */
    fun name(code: Long): String? = roomKindOf(code)?.let { roomKindName(resources, it) }

    fun area(squareCentimetres: Long): String =
        resources.getString(R.string.room_area_value, tenths(BigDecimal.valueOf(squareCentimetres).movePointLeft(CM2_DIGITS)))

    private companion object {
        // A square metre is 10 000 cm².
        const val CM2_DIGITS = 4
    }
}

/** An opening's mark as plans write it: the letters of its kind and its number, «ОК-1». */
internal fun markText(
    resources: Resources,
    kind: TextKind,
    number: Long,
): String =
    resources.getString(
        when (kind) {
            TextKind.DOOR_MARK -> R.string.mark_door
            TextKind.EMPTY_MARK -> R.string.mark_empty
            else -> R.string.mark_window
        },
        number,
    )

/** The kind of mark an opening of [kind] has. */
internal fun markKind(kind: OpeningKind): TextKind =
    when (kind) {
        OpeningKind.WINDOW -> TextKind.WINDOW_MARK
        OpeningKind.DOOR -> TextKind.DOOR_MARK
        OpeningKind.EMPTY -> TextKind.EMPTY_MARK
    }

@Composable
internal fun roomKindName(kind: RoomKind): String = roomKindName(LocalResources.current, kind)

@Composable
internal fun areaText(squareMetres: Double): String = areaText(LocalResources.current, squareMetres)

@Composable
internal fun metresText(metres: Double): String = metresText(LocalResources.current, metres)
