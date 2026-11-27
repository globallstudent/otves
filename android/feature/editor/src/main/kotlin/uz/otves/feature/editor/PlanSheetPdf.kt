package uz.otves.feature.editor

import android.content.Context
import android.content.Intent
import android.content.res.Resources
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Typeface
import android.graphics.pdf.PdfDocument
import android.text.TextPaint
import android.text.TextUtils
import androidx.core.content.FileProvider
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import uz.otves.engine.Drawing
import uz.otves.engine.DrawnPolygon
import uz.otves.engine.FillKind
import uz.otves.engine.ListedOpening
import uz.otves.engine.ListedRoom
import uz.otves.engine.PointM
import uz.otves.engine.SheetFormat
import uz.otves.engine.StrokeKind
import java.io.File
import java.io.OutputStream
import java.math.BigDecimal
import java.math.RoundingMode
import java.time.LocalDate
import java.time.format.DateTimeFormatter
import java.time.format.FormatStyle
import java.util.Locale
import kotlin.math.roundToInt

/**
 * The job's plans as sheets to print: one storey to a page, in a frame, with every room's sizes,
 * the marks of the openings, the room schedule, the opening schedule and what the hatches mean
 * beside it (docs/adr/0016, 0017). The sheet's size, its frame and the height of the plan's text
 * are the catalog's (content/catalog/sheet.json); a page is laid out in points. Everything on the
 * plan and in the schedules is the engine's.
 */
internal class PlanSheetPdf(
    private val resources: Resources,
    format: SheetFormat,
) {
    private val pageWidth = (format.width * POINTS_PER_MM).roundToInt()
    private val pageHeight = (format.height * POINTS_PER_MM).roundToInt()

    // The frame stands further from the left edge, where the sheet is bound.
    private val frameLeft = format.binding.toFloat() * POINTS_PER_MM
    private val frameTop = format.edge.toFloat() * POINTS_PER_MM
    private val frameRight = pageWidth - format.edge.toFloat() * POINTS_PER_MM
    private val frameBottom = pageHeight - format.edge.toFloat() * POINTS_PER_MM
    private val sideLeft = frameRight - SIDE_WIDTH
    private val planLeft = frameLeft + PAD
    private val planTop = frameTop + PAD
    private val planWidth = sideLeft - PAD - planLeft
    private val planHeight = frameBottom - PAD - planTop
    private val labelSize = format.textHeight.toFloat() * POINTS_PER_MM
    private val scales = format.scales

    private val caption = text(CAPTION_SIZE, INK, bold = true)
    private val head = text(TEXT_SIZE, SECONDARY)
    private val body = text(TEXT_SIZE, INK)
    private val strong = text(TEXT_SIZE, INK, bold = true)
    private val title = text(TITLE_SIZE, INK, bold = true)
    private val frameLine =
        Paint(Paint.ANTI_ALIAS_FLAG).apply {
            style = Paint.Style.STROKE
            strokeWidth = FRAME_WIDTH
            color = INK
        }
    private val rule =
        Paint().apply {
            strokeWidth = HAIRLINE
            color = INK
        }

    private val painter =
        PlanPainter(
            palette =
                PlanPalette(
                    wall = WALL,
                    unchosen = PAPER,
                    gap = PAPER,
                    floor = PAPER,
                    edge = INK,
                    layer = SECONDARY,
                    below = Color.TRANSPARENT,
                    dimension = INK,
                    text = INK,
                    name = INK,
                    selection = Color.TRANSPARENT,
                    warning = Color.TRANSPARENT,
                ),
            hairline = HAIRLINE,
            bold = HAIRLINE,
            hatch = HATCH_GAP,
            textSize = labelSize,
            about = resources.getString(R.string.length_about_mark),
            rooms = RoomLabels(resources),
            marks = { kind, number -> markText(resources, kind, number) },
            sheet = true,
        )

    // Where a storey's plan lies in the plan box: at the largest of the sheet's scales that
    // leaves room around the walls for the sizes written outside them, and in its middle. A plan
    // too large for every scale is drawn as large as the box allows, at no scale to name.
    private class Placed(
        val frame: PlanFrame,
        val scale: Int?,
    )

    private fun place(storey: Storey): Placed? {
        val points = storey.outlines.flatMap { it.points }
        if (points.isEmpty()) return null
        val left = points.minOf { it.x }
        val bottom = points.minOf { it.y }
        val wide = (points.maxOf { it.x } - left).coerceAtLeast(MIN_EXTENT)
        val tall = (points.maxOf { it.y } - bottom).coerceAtLeast(MIN_EXTENT)
        val fitted = minOf((planWidth - 2 * SIZE_ROOM) / wide, (planHeight - 2 * SIZE_ROOM) / tall).toFloat()
        val named = namedScale(scales, fitted)
        val scale = named?.let(::pointsPerMetre) ?: fitted
        val x = planLeft + (planWidth - (wide * scale).toFloat()) / 2
        val y = planTop + (planHeight + (tall * scale).toFloat()) / 2
        return Placed(PlanFrame(originX = x - (left * scale).toFloat(), originY = y + (bottom * scale).toFloat(), scale = scale), named)
    }

    /**
     * How big the sheet's label text is on each storey's plan, from the bottom up; null for a
     * storey with no walls. The engine draws each storey for its size.
     */
    fun labels(state: EditorUiState): List<LabelSize?> =
        state.plan.storeys.map { storey -> place(storey)?.let { painter.labelSize(it.frame.scale) } }

    /** [drawings] are the storeys' drawings, asked as sheets with [labels]. */
    fun write(
        state: EditorUiState,
        drawings: List<Drawing?>,
        date: LocalDate,
        out: OutputStream,
    ) {
        val document = PdfDocument()
        var number = 0
        state.plan.storeys.forEachIndexed { index, storey ->
            val placed = place(storey)
            val drawing = drawings.getOrNull(index)
            if (placed == null || drawing == null) return@forEachIndexed
            val page = document.startPage(PdfDocument.PageInfo.Builder(pageWidth, pageHeight, ++number).create())
            sheet(page.canvas, state, drawing, placed, storeyNumber = index + 1, date)
            document.finishPage(page)
        }
        document.writeTo(out)
        document.close()
    }

    private fun sheet(
        canvas: Canvas,
        state: EditorUiState,
        drawing: Drawing,
        placed: Placed,
        storeyNumber: Int,
        date: LocalDate,
    ) {
        canvas.drawRect(frameLeft, frameTop, frameRight, frameBottom, frameLine)
        canvas.drawLine(sideLeft, frameTop, sideLeft, frameBottom, frameLine)
        painter.paint(canvas, drawing, placed.frame)

        var y = frameTop + PAD
        y = rooms(canvas, drawing.rooms, y)
        y = openings(canvas, drawing.openings, y + SECTION_GAP)
        legend(canvas, drawing, y + SECTION_GAP)
        stamp(canvas, state, storeyNumber, placed.scale, date)
    }

    // The room schedule: every space by its name with its floor, and their sum.
    private fun rooms(
        canvas: Canvas,
        listed: List<ListedRoom>,
        top: Float,
    ): Float {
        if (listed.isEmpty()) return top - SECTION_GAP
        var y = caption(canvas, resources.getString(R.string.sheet_rooms), top)
        y = row(canvas, y, head, resources.getString(R.string.sheet_room_name), resources.getString(R.string.sheet_room_area))
        for (room in listed) {
            val name = if (room.room == null) resources.getString(R.string.room_title) else roomKindName(resources, room.kind)
            y = row(canvas, y, body, name, tenths(room.area))
        }
        return row(canvas, y, strong, resources.getString(R.string.takeoff_total), tenths(listed.sumOf { it.area }))
    }

    // The opening schedule: every mark of the job with its size and how many there are.
    private fun openings(
        canvas: Canvas,
        listed: List<ListedOpening>,
        top: Float,
    ): Float {
        if (listed.isEmpty()) return top - SECTION_GAP
        var y = caption(canvas, resources.getString(R.string.sheet_openings), top)
        y =
            row(
                canvas,
                y,
                head,
                resources.getString(R.string.opening_mark),
                resources.getString(R.string.sheet_count),
                middle = resources.getString(R.string.sheet_size),
            )
        for (opening in listed) {
            val size = resources.getString(R.string.opening_size, opening.width.value, opening.height.value)
            y = row(canvas, y, body, markText(resources, opening.kind, opening.number), opening.count.toString(), middle = size)
        }
        return y
    }

    // What the fills of this plan's walls stand for, each painted as on the plan.
    private fun legend(
        canvas: Canvas,
        drawing: Drawing,
        top: Float,
    ) {
        val fills = drawing.polygons.map { it.fill }.toSet()
        val shown =
            listOf(
                FillKind.WALL to R.string.legend_masonry,
                FillKind.FACING to R.string.legend_facing,
                FillKind.INSULATION to R.string.legend_insulation,
            ).filter { it.first in fills }
        if (shown.isEmpty()) return
        var y = caption(canvas, resources.getString(R.string.sheet_legend), top)
        for ((fill, name) in shown) {
            // A swatch is a polygon of a plan drawn a point to the metre: the painter fills it
            // as it fills a wall.
            val left = (sideLeft + PAD).toDouble()
            val right = left + SWATCH_WIDTH
            val high = SWATCH_HEIGHT.toDouble()
            val swatch =
                DrawnPolygon(
                    element = null,
                    fill = fill,
                    stroke = StrokeKind.WALL_EDGE,
                    points = listOf(PointM(left, 0.0), PointM(right, 0.0), PointM(right, high), PointM(left, high)),
                )
            painter.paint(canvas, Drawing(polygons = listOf(swatch)), PlanFrame(originX = 0f, originY = y + SWATCH_HEIGHT, scale = 1f))
            canvas.drawText(resources.getString(name), sideLeft + 2 * PAD + SWATCH_WIDTH, y + SWATCH_HEIGHT - 1.5f, body)
            y += ROW
        }
    }

    // The title block in the sheet's lower right corner: the job, the sheet and the day.
    private fun stamp(
        canvas: Canvas,
        state: EditorUiState,
        storeyNumber: Int,
        scale: Int?,
        date: LocalDate,
    ) {
        val top = frameBottom - STAMP_HEIGHT
        canvas.drawLine(sideLeft, top, frameRight, top, frameLine)
        val width = SIDE_WIDTH - 2 * PAD
        val name = state.name.ifBlank { resources.getString(R.string.editor_title) }
        canvas.drawText(fit(name, title, width), sideLeft + PAD, top + PAD + TITLE_SIZE, title)
        val plan = resources.getString(R.string.sheet_plan, storeyNumber)
        // «План 1 этажа, М 1:100»; a plan too large for a scale has none to name.
        val line = if (scale == null) plan else resources.getString(R.string.sheet_plan_scale, plan, scale)
        canvas.drawText(line, sideLeft + PAD, top + PAD + TITLE_SIZE + ROW, body)
        // The language of the strings, not of the phone: Russian text gets a Russian date.
        val locale = Locale.forLanguageTag(resources.getString(R.string.editor_date_locale))
        val day = date.format(DateTimeFormatter.ofLocalizedDate(FormatStyle.LONG).withLocale(locale))
        canvas.drawText(day, sideLeft + PAD, top + PAD + TITLE_SIZE + 2 * ROW, head)
    }

    private fun caption(
        canvas: Canvas,
        text: String,
        top: Float,
    ): Float {
        canvas.drawText(text, sideLeft + PAD, top + CAPTION_SIZE, caption)
        return top + CAPTION_SIZE + ROW_GAP
    }

    // One line of a schedule: a name at the left, a number at the right and, between them, a
    // size; ruled off below.
    private fun row(
        canvas: Canvas,
        top: Float,
        paint: TextPaint,
        left: String,
        right: String,
        middle: String? = null,
    ): Float {
        val baseline = top + ROW - ROW_GAP
        val edge = frameRight - PAD
        canvas.drawText(fit(left, paint, NAME_WIDTH), sideLeft + PAD, baseline, paint)
        if (middle != null) canvas.drawText(middle, sideLeft + PAD + MIDDLE_LEFT, baseline, paint)
        canvas.drawText(right, edge - paint.measureText(right), baseline, paint)
        canvas.drawLine(sideLeft + PAD, top + ROW, edge, top + ROW, rule)
        return top + ROW
    }

    // Square metres to a tenth, with a decimal comma.
    private fun tenths(value: Double): String =
        BigDecimal
            .valueOf(value)
            .setScale(1, RoundingMode.HALF_UP)
            .toPlainString()
            .replace('.', ',')

    private fun fit(
        text: String,
        paint: TextPaint,
        width: Float,
    ): String = TextUtils.ellipsize(text, paint, width, TextUtils.TruncateAt.END).toString()

    private fun text(
        size: Float,
        color: Int,
        bold: Boolean = false,
    ) = TextPaint(Paint.ANTI_ALIAS_FLAG).apply {
        textSize = size
        this.color = color
        typeface = if (bold) Typeface.DEFAULT_BOLD else Typeface.DEFAULT
    }

    private companion object {
        const val FRAME_WIDTH = 1.2f

        // The schedules and the title block stand in a column at the right of the frame.
        const val SIDE_WIDTH = 236f
        const val PAD = 10f
        const val STAMP_HEIGHT = 64f
        const val NAME_WIDTH = 120f
        const val MIDDLE_LEFT = 78f

        // Round the walls, for the plan's overall size and the lengths written outside them.
        const val SIZE_ROOM = 34f

        const val TITLE_SIZE = 12f
        const val CAPTION_SIZE = 10f
        const val TEXT_SIZE = 8.5f
        const val ROW_GAP = 4f
        const val ROW = TEXT_SIZE + 2 * ROW_GAP
        const val SECTION_GAP = 16f
        const val SWATCH_WIDTH = 26f
        const val SWATCH_HEIGHT = 10f
        const val HAIRLINE = 0.6f
        const val HATCH_GAP = 2.5f

        // A single short wall is not blown up to fill the sheet: at least this many metres show.
        const val MIN_EXTENT = 4.0
        const val INK = 0xFF000000.toInt()
        const val SECONDARY = 0xFF4B5563.toInt()
        const val WALL = 0xFFB5BCC6.toInt()
        const val PAPER = 0xFFFFFFFF.toInt()
    }
}

/** How many points on the sheet a metre of the plan takes at 1:[scale]. */
internal fun pointsPerMetre(scale: Int): Float = MM_PER_METRE / scale * POINTS_PER_MM

/**
 * The scale to draw a plan at: of [scales], each the n of a 1:n and the largest scale first, the
 * first at which the plan is no larger than [fitted] points a metre lets it be. Null when the
 * plan is too large for all of them.
 */
internal fun namedScale(
    scales: List<Int>,
    fitted: Float,
): Int? = scales.firstOrNull { pointsPerMetre(it) <= fitted }

// A PDF page is measured in points, 72 to the inch.
private const val POINTS_PER_MM = 72f / 25.4f
private const val MM_PER_METRE = 1000f

/**
 * Writes the plans into the cache and offers them to the apps that take a PDF. [sheets] asks the
 * engine for every storey's drawing as a sheet, with label text of the sizes the sheet uses.
 */
internal suspend fun Context.sharePlans(
    state: EditorUiState,
    sheets: suspend (List<LabelSize?>) -> List<Drawing?>,
) {
    val pdf = PlanSheetPdf(resources, state.catalog.sheet ?: return)
    val drawings = sheets(pdf.labels(state))
    val file =
        withContext(Dispatchers.IO) {
            val folder = File(cacheDir, "estimates").apply { mkdirs() }
            val name =
                getString(R.string.share_plans_file, state.name.ifBlank { getString(R.string.editor_title) })
                    .replace(Regex("""[\\/:*?"<>|]"""), " ")
                    .trim()
            File(folder, "$name.pdf").also { file ->
                file.outputStream().use { pdf.write(state, drawings, LocalDate.now(), it) }
            }
        }
    val uri = FileProvider.getUriForFile(this, "$packageName.editor.files", file)
    val send =
        Intent(Intent.ACTION_SEND)
            .setType("application/pdf")
            .putExtra(Intent.EXTRA_STREAM, uri)
            .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
    startActivity(Intent.createChooser(send, null))
}
