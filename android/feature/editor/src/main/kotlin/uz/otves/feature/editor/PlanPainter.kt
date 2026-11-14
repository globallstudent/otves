package uz.otves.feature.editor

import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.graphics.Typeface
import android.text.TextPaint
import androidx.core.graphics.withClip
import androidx.core.graphics.withRotation
import uz.otves.core.model.ElementId
import uz.otves.core.model.Mm
import uz.otves.engine.Drawing
import uz.otves.engine.DrawnArc
import uz.otves.engine.DrawnPolygon
import uz.otves.engine.DrawnText
import uz.otves.engine.FillKind
import uz.otves.engine.PointM
import uz.otves.engine.Space
import uz.otves.engine.StrokeKind
import uz.otves.engine.TextKind
import kotlin.math.abs
import kotlin.math.atan2
import kotlin.math.ceil
import kotlin.math.hypot
import kotlin.math.roundToLong

/** The colours a plan is painted in, as ARGB. */
internal data class PlanPalette(
    val wall: Int,
    /** A wall with no material yet: drawn, but not built from anything. */
    val unchosen: Int,
    /** What an opening leaves of its wall: the paper under the plan. */
    val gap: Int,
    /** The floor of a space the walls close. */
    val floor: Int,
    val edge: Int,
    /** The lines of a layer of insulation, and the line where two layers meet. */
    val layer: Int,
    val below: Int,
    /** A dimension line outside the plan, its ticks and the lines that lead to it. */
    val dimension: Int,
    val text: Int,
    /** A room's name, the text that says most. */
    val name: Int,
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

/** A size the painter wrote, and where: the text and the box it fills on the canvas. */
internal data class PaintedSize(
    val size: DrawnText,
    val bounds: RectF,
)

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
    /** How far apart the lines of a hatch lie. */
    private val hatch: Float,
    textSize: Float,
    /** The mark before a length nobody has given: «≈». */
    private val about: String,
    /** What the texts of rooms say. */
    private val rooms: RoomLabels,
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
    private val name =
        TextPaint(Paint.ANTI_ALIAS_FLAG).apply {
            this.textSize = textSize
            color = palette.name
            typeface = Typeface.create(Typeface.DEFAULT, NAME_WEIGHT, false)
        }
    private val chain =
        TextPaint(Paint.ANTI_ALIAS_FLAG).apply {
            this.textSize = textSize
            color = palette.selection
            typeface = Typeface.create(Typeface.DEFAULT, LABEL_WEIGHT, false)
        }

    // The sizes written by the last paint that a typed value can change.
    private val handles = mutableListOf<PaintedSize>()
    private val path = Path()
    private val oval = RectF()
    private val box = RectF()

    /** The size of the label text on a plan drawn at [scale] pixels a metre, to ask a drawing with. */
    fun labelSize(scale: Float): LabelSize =
        LabelSize(
            height = Mm((text.fontSpacing / scale * MM_PER_METRE).roundToLong()),
            digit = Mm((text.measureText("0") / scale * MM_PER_METRE).roundToLong()),
        )

    /**
     * Paints [drawing] in the order of its lists. The space that is [marked] is tinted over the
     * floors and under the walls. The wall that is [selected] is marked before the openings' gaps
     * are painted, so they stay cut out of it; a selected opening and the ones in [faulty] are
     * outlined over everything.
     */
    fun paint(
        canvas: Canvas,
        drawing: Drawing,
        frame: PlanFrame,
        selected: ElementId? = null,
        faulty: Set<ElementId> = emptySet(),
        marked: Space? = null,
    ) {
        handles.clear()
        val (gaps, solids) = drawing.polygons.partition { it.fill == FillKind.GAP }
        val (floors, bodies) = solids.partition { it.fill == FillKind.FLOOR }
        floors.forEach { polygon(canvas, it, frame) }
        if (marked != null) tint(canvas, marked, frame)
        bodies.forEach { polygon(canvas, it, frame) }
        // A wall of several layers is marked once, by its outline.
        bodies.filter { it.element != null && it.element == selected && it.stroke == StrokeKind.WALL_EDGE }.forEach { polygon ->
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

    /**
     * The size written nearest to a touch at ([x], [y]) on the canvas of the last paint, among
     * those a typed value can change. A touch counts within [reach] of a number's box.
     */
    fun sizeAt(
        x: Float,
        y: Float,
        reach: Float,
    ): DrawnText? =
        handles
            .filter { x in it.bounds.left - reach..it.bounds.right + reach && y in it.bounds.top - reach..it.bounds.bottom + reach }
            .minByOrNull { hypot(it.bounds.centerX() - x, it.bounds.centerY() - y) }
            ?.size

    private fun polygon(
        canvas: Canvas,
        polygon: DrawnPolygon,
        frame: PlanFrame,
    ) {
        trace(polygon.points, frame)
        val inside =
            when (polygon.fill) {
                FillKind.WALL -> palette.wall

                // Hatched layers are drawn on paper, as a wall with no material is.
                FillKind.UNCHOSEN, FillKind.FACING, FillKind.INSULATION -> palette.unchosen

                FillKind.GAP -> palette.gap

                FillKind.FLOOR -> palette.floor

                FillKind.NONE, FillKind.UNKNOWN -> null
            }
        if (inside != null) {
            fill.color = inside
            canvas.drawPath(path, fill)
        }
        // Facing masonry is hatched at 45°, as brickwork is on drawings. Insulation is hatched both
        // ways in the lighter line, so it reads whichever way its wall runs.
        when (polygon.fill) {
            FillKind.FACING -> hatch(canvas, frame, palette.edge, crossed = false)
            FillKind.INSULATION -> hatch(canvas, frame, palette.layer, crossed = true)
            else -> Unit
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
            StrokeKind.LAYER -> palette.layer
            StrokeKind.DIMENSION -> palette.dimension
            StrokeKind.CHAIN -> palette.selection
            StrokeKind.NONE, StrokeKind.UNKNOWN -> null
        }

    // Slanted lines across the traced path, [hatch] apart. They are counted from the plan's origin,
    // so a hatch stays on its wall when the plan moves, and runs on from one wall into the next.
    private fun hatch(
        canvas: Canvas,
        frame: PlanFrame,
        colour: Int,
        crossed: Boolean,
    ) {
        path.computeBounds(box, true)
        line.color = colour
        val step = hatch * SQRT_2
        canvas.withClip(path) {
            // Every point of one line has the same x + y.
            var sum = first(box.left + box.top, frame.originX + frame.originY, step)
            while (sum <= box.right + box.bottom) {
                drawLine(sum - box.bottom, box.bottom, sum - box.top, box.top, line)
                sum += step
            }
            if (!crossed) return@withClip
            // And every point of one line the other way the same x - y.
            var difference = first(box.left - box.bottom, frame.originX - frame.originY, step)
            while (difference <= box.right - box.top) {
                drawLine(difference + box.top, box.top, difference + box.bottom, box.bottom, line)
                difference += step
            }
        }
    }

    // The first value at or after [from] that lies a whole number of steps from [origin].
    private fun first(
        from: Float,
        origin: Float,
        step: Float,
    ): Float = origin + ceil((from - origin) / step) * step

    // The floor of the selected space in a wash of the selection colour, with what stands free
    // inside it left out.
    private fun tint(
        canvas: Canvas,
        space: Space,
        frame: PlanFrame,
    ) {
        path.rewind()
        path.fillType = Path.FillType.EVEN_ODD
        (listOf(space.outline) + space.islands).forEach { ring ->
            ring.forEachIndexed { index, point ->
                if (index == 0) path.moveTo(frame.x(point), frame.y(point)) else path.lineTo(frame.x(point), frame.y(point))
            }
            path.close()
        }
        fill.color = palette.selection and FLOOR_TINT
        canvas.drawPath(path, fill)
        path.fillType = Path.FillType.WINDING
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
        val written =
            when (label.kind) {
                TextKind.LENGTH -> label.value.toString()
                TextKind.ABOUT_LENGTH -> "$about ${label.value}"
                TextKind.ROOM_NAME -> rooms.name(label.value) ?: return
                TextKind.ROOM_AREA -> rooms.area(label.value)
                TextKind.OVERALL, TextKind.CLEAR, TextKind.PIER, TextKind.OPENING_WIDTH -> grouped(label.value)
                TextKind.UNKNOWN -> return
            }
        val pen =
            when (label.kind) {
                TextKind.ROOM_NAME -> name

                // The sizes of the selected space are in its colour.
                TextKind.CLEAR, TextKind.PIER, TextKind.OPENING_WIDTH -> chain

                else -> text
            }
        val width = pen.measureText(written)
        // The engine says how much room a text has; one that is wider is left out.
        if (label.fit > 0.0 && width > label.fit * frame.scale) return
        val x = frame.x(label.at)
        val y = frame.y(label.at)
        val turn = -Math.toDegrees(atan2(label.along.y, label.along.x)).toFloat()
        canvas.withRotation(turn, x, y) {
            drawText(written, x - width / 2, y - (pen.ascent() + pen.descent()) / 2, pen)
        }
        if (label.isHandle) handles += PaintedSize(label, box(x, y, width, pen.fontSpacing, label.along))
    }

    // The upright box round a text [width] by [height] with its middle at ([x], [y]), written
    // along [along] on the plan.
    private fun box(
        x: Float,
        y: Float,
        width: Float,
        height: Float,
        along: PointM,
    ): RectF {
        val across = abs(along.x).toFloat()
        val up = abs(along.y).toFloat()
        val halfWide = (width * across + height * up) / 2
        val halfHigh = (width * up + height * across) / 2
        return RectF(x - halfWide, y - halfHigh, x + halfWide, y + halfHigh)
    }

    // The direction from [centre] to [point] on the plan, in degrees counter-clockwise from +x.
    private fun degrees(
        centre: PointM,
        point: PointM,
    ): Float = Math.toDegrees(atan2(point.y - centre.y, point.x - centre.x)).toFloat()

    private companion object {
        const val MM_PER_METRE = 1000.0
        const val FULL_TURN = 360f
        const val SQRT_2 = 1.4142135f

        // Medium, as the theme's label text.
        const val LABEL_WEIGHT = 500

        // Semi-bold: a room's name stands out from the numbers round it.
        const val NAME_WEIGHT = 600

        // An eighth of the selection colour over the floor it marks.
        const val FLOOR_TINT = 0x20FFFFFF

        // A quarter of the selection colour over the wall it marks.
        const val SELECTION_FILL = 0x40FFFFFF
    }
}
