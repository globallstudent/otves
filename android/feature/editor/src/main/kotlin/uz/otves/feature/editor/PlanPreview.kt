package uz.otves.feature.editor

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import androidx.compose.ui.graphics.toArgb
import androidx.core.graphics.createBitmap
import uz.otves.core.designsystem.LightOtvesColors
import uz.otves.engine.Drawing
import uz.otves.engine.StrokeKind
import java.io.ByteArrayOutputStream
import kotlin.math.ceil

/**
 * A storey as a small picture for the jobs list: its walls, floors and openings on a clear
 * ground, with no text and no sizes. A PNG cut close to the plan, or null for a storey with no walls.
 */
internal class PlanPreview(
    rooms: RoomLabels,
) {
    private val painter =
        PlanPainter(
            palette =
                PlanPalette(
                    wall = LightOtvesColors.ink.toArgb(),
                    // At this size a wall reads as a wall only when it is dark, with a material or without.
                    unchosen = LightOtvesColors.ink.toArgb(),
                    gap = LightOtvesColors.surface.toArgb(),
                    floor = LightOtvesColors.surface.toArgb(),
                    edge = LightOtvesColors.ink.toArgb(),
                    layer = Color.TRANSPARENT,
                    below = Color.TRANSPARENT,
                    dimension = Color.TRANSPARENT,
                    text = Color.TRANSPARENT,
                    name = Color.TRANSPARENT,
                    selection = Color.TRANSPARENT,
                    warning = Color.TRANSPARENT,
                ),
            hairline = EDGE,
            bold = EDGE,
            hatch = HATCH,
            textSize = 1f,
            about = "",
            rooms = rooms,
            marks = { _, _ -> "" },
        )

    fun png(drawing: Drawing): ByteArray? {
        val own = drawing.polygons.filter { it.stroke != StrokeKind.BELOW }
        val points = own.flatMap { it.points }
        if (points.isEmpty()) return null
        val left = points.minOf { it.x }
        val top = points.maxOf { it.y }
        val wide = (points.maxOf { it.x } - left).coerceAtLeast(MIN_EXTENT)
        val tall = (top - points.minOf { it.y }).coerceAtLeast(MIN_EXTENT)
        val scale = ((LONG_SIDE - 2 * MARGIN) / maxOf(wide, tall)).toFloat()
        val bitmap = createBitmap(ceil(wide * scale).toInt() + 2 * MARGIN, ceil(tall * scale).toInt() + 2 * MARGIN)
        val frame = PlanFrame(originX = MARGIN - (left * scale).toFloat(), originY = MARGIN + (top * scale).toFloat(), scale = scale)
        val plain =
            drawing.copy(
                polygons = own,
                lines = drawing.lines.filter { it.stroke == StrokeKind.FRAME || it.stroke == StrokeKind.LEAF },
                texts = emptyList(),
            )
        painter.paint(Canvas(bitmap), plain, frame)
        val bytes = ByteArrayOutputStream()
        bitmap.compress(Bitmap.CompressFormat.PNG, PNG_QUALITY, bytes)
        bitmap.recycle()
        return bytes.toByteArray()
    }

    private companion object {
        // Pixels: the longer side of the picture, the clear rim round the plan, a wall's edge.
        const val LONG_SIDE = 640
        const val MARGIN = 8
        const val EDGE = 2f
        const val HATCH = 12f
        const val PNG_QUALITY = 100

        // Metres: a single short wall still gets a picture of a sensible scale.
        const val MIN_EXTENT = 1.0
    }
}
