package uz.otves.feature.editor

import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.graphics.Typeface
import android.text.TextPaint
import androidx.core.graphics.withRotation
import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm
import uz.otves.engine.Drawing
import uz.otves.engine.DrawnArc
import uz.otves.engine.DrawnPolygon
import uz.otves.engine.DrawnText
import uz.otves.engine.FillKind
import uz.otves.engine.PointM
import uz.otves.engine.StrokeKind
import uz.otves.engine.TextKind
import kotlin.math.atan2
import kotlin.math.hypot
import kotlin.math.roundToLong

/** The colours a plan is painted in, as ARGB. */
internal data class PlanPalette(
    val wall: Int,
    /** A wall with no material yet: drawn, but not built from anything. */
    val unchosen: Int,
    /** What an opening leaves of its wall: the paper under the plan. */
    val gap: Int,
    val edge: Int,
    val below: Int,
    val text: Int,
    val selection: Int,
    val warning: Int,
)

/**
 * Where plan metres fall on a canvas: the pixel of the plan's origin and the pixels in a metre.
 * The plan's y runs up and the canvas's down.
 */
internal data class PlanFrame(
    val originX: Float,
    val originY: Float,
    val scale: Float,
) {
    fun x(point: PointM): Float = originX + (point.x * scale).toFloat()

    fun y(point: PointM): Float = originY - (point.y * scale).toFloat()
}

/** How big the label text is on the plan at a zoom: the height of a line and the width of a digit. */
internal data class LabelSize(
    val height: Mm,
    val digit: Mm,
)

/**
 * Paints a level's [Drawing] (docs/adr/0013) on a canvas, the screen's or a PDF page's. It maps
 * fills and strokes to paints and writes the numbers where the engine put them. It positions
 * nothing: what it does not know how to paint it leaves out.
 */
internal class PlanPainter(
    private val palette: PlanPalette,
    hairline: Float,
    /** The width of the line round what is selected or to be fixed. */
    bold: Float,
    textSize: Float,
    /** The mark before a length nobody has given: «≈». */
    private val about: String,
) {
    private val fill = Paint(Paint.ANTI_ALIAS_FLAG).apply { style = Paint.Style.FILL }
    private val line =
        Paint(Paint.ANTI_ALIAS_FLAG).apply {
            style = Paint.Style.STROKE
            strokeWidth = hairline
        }
    private val mark =
        Paint(Paint.ANTI_ALIAS_FLAG).apply {
            style = Paint.Style.STROKE
            strokeWidth = bold
        }
    private val text =
        TextPaint(Paint.ANTI_ALIAS_FLAG).apply {
            this.textSize = textSize
            color = palette.text
            typeface = Typeface.create(Typeface.DEFAULT, LABEL_WEIGHT, false)
        }
    private val path = Path()
    private val oval = RectF()

    /** The size of the label text on a plan drawn at [scale] pixels a metre, to ask a drawing with. */
    fun labelSize(scale: Float): LabelSize =
        LabelSize(
            height = Mm((text.fontSpacing / scale * MM_PER_METRE).roundToLong()),
            digit = Mm((text.measureText("0") / scale * MM_PER_METRE).roundToLong()),
        )

    /**
     * Paints [drawing] in the order of its lists. The wall that is [selected] is marked before the
     * openings' gaps are painted, so they stay cut out of it; a selected opening and the ones in
     * [faulty] are outlined over everything.
     */
    fun paint(
        canvas: Canvas,
        drawing: Drawing,
        frame: PlanFrame,
        selected: ElementId? = null,
        faulty: Set<ElementId> = emptySet(),
    ) {
        val (gaps, bodies) = drawing.polygons.partition { it.fill == FillKind.GAP }
        bodies.forEach { polygon(canvas, it, frame) }
        bodies.filter { it.element != null && it.element == selected }.forEach { polygon ->
            trace(polygon.points, frame)
            fill.color = palette.selection and SELECTION_FILL
            canvas.drawPath(path, fill)
            outline(canvas, palette.selection)
        }
        gaps.forEach { polygon(canvas, it, frame) }
        for (segment in drawing.lines) {
            line.color = colour(segment.stroke) ?: continue
            canvas.drawLine(frame.x(segment.from), frame.y(segment.from), frame.x(segment.to), frame.y(segment.to), line)
        }
        drawing.arcs.forEach { arc(canvas, it, frame) }
        drawing.texts.forEach { text(canvas, it, frame) }
        for (gap in gaps) {
            val warns = gap.element in faulty
            val chosen = gap.element != null && gap.element == selected
            if (!warns && !chosen) continue
            trace(gap.points, frame)
            if (warns) outline(canvas, palette.warning)
            if (chosen) outline(canvas, palette.selection)
        }
    }

    private fun polygon(
        canvas: Canvas,
        polygon: DrawnPolygon,
        frame: PlanFrame,
    ) {
        trace(polygon.points, frame)
        val inside =
            when (polygon.fill) {
                FillKind.WALL -> palette.wall
                FillKind.UNCHOSEN -> palette.unchosen
                FillKind.GAP -> palette.gap
                FillKind.NONE, FillKind.UNKNOWN -> null
            }
        if (inside != null) {
            fill.color = inside
            canvas.drawPath(path, fill)
        }
        colour(polygon.stroke)?.let {
            line.color = it
            canvas.drawPath(path, line)
        }
    }

    private fun colour(stroke: StrokeKind): Int? =
        when (stroke) {
            StrokeKind.WALL_EDGE, StrokeKind.FRAME, StrokeKind.LEAF -> palette.edge
            StrokeKind.BELOW -> palette.below
            StrokeKind.NONE, StrokeKind.UNKNOWN -> null
        }

    // The traced path's outline in the bold line.
    private fun outline(
        canvas: Canvas,
        colour: Int,
    ) {
        mark.color = colour
        canvas.drawPath(path, mark)
    }

    private fun trace(
        points: List<PointM>,
        frame: PlanFrame,
    ) {
        path.rewind()
        points.forEachIndexed { index, point ->
            if (index == 0) path.moveTo(frame.x(point), frame.y(point)) else path.lineTo(frame.x(point), frame.y(point))
        }
        path.close()
    }

    // Counter-clockwise on the plan is counter-clockwise on the canvas too, whose angles run
    // clockwise: both the start and the sweep change sign.
    private fun arc(
        canvas: Canvas,
        arc: DrawnArc,
        frame: PlanFrame,
    ) {
        line.color = colour(arc.stroke) ?: return
        val radius = (hypot(arc.from.x - arc.centre.x, arc.from.y - arc.centre.y) * frame.scale).toFloat()
        val start = degrees(arc.centre, arc.from)
        val sweep = (degrees(arc.centre, arc.to) - start).let { if (it <= 0) it + FULL_TURN else it }
        val x = frame.x(arc.centre)
        val y = frame.y(arc.centre)
        oval.set(x - radius, y - radius, x + radius, y + radius)
        canvas.drawArc(oval, -start, -sweep, false, line)
    }

    private fun text(
        canvas: Canvas,
        label: DrawnText,
        frame: PlanFrame,
    ) {
        val digits = label.value.value.toString()
        val written =
            when (label.kind) {
                TextKind.LENGTH -> digits
                TextKind.ABOUT_LENGTH -> "$about $digits"
                TextKind.UNKNOWN -> return
            }
        val x = frame.x(label.at)
        val y = frame.y(label.at)
        val turn = -Math.toDegrees(atan2(label.along.y, label.along.x)).toFloat()
        canvas.withRotation(turn, x, y) {
            drawText(written, x - text.measureText(written) / 2, y - (text.ascent() + text.descent()) / 2, text)
        }
    }

    // The direction from [centre] to [point] on the plan, in degrees counter-clockwise from +x.
    private fun degrees(
        centre: PointM,
        point: PointM,
    ): Float = Math.toDegrees(atan2(point.y - centre.y, point.x - centre.x)).toFloat()

    private companion object {
        const val MM_PER_METRE = 1000.0
        const val FULL_TURN = 360f

        // Medium, as the theme's label text.
        const val LABEL_WEIGHT = 500

        // A quarter of the selection colour over the wall it marks.
        const val SELECTION_FILL = 0x40FFFFFF
    }
}
