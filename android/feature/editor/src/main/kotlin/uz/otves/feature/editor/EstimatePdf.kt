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
import uz.otves.engine.QuantityLine
import java.io.File
import java.io.OutputStream
import java.time.LocalDate
import java.time.format.DateTimeFormatter
import java.time.format.FormatStyle
import java.util.Locale

/**
 * The job's materials as a sheet to send: the plan of every storey, then every material with
 * its amount, the builder's price and the cost, and the total. A4 in points, as many pages as the
 * lines need.
 */
internal class EstimatePdf(
    private val resources: Resources,
) {
    private val title = text(TITLE_SIZE, INK, bold = true)
    private val heading = text(TEXT_SIZE, SECONDARY)
    private val body = text(TEXT_SIZE, INK)
    private val quiet = text(TEXT_SIZE, SECONDARY)
    private val small = text(LABEL_SIZE, SECONDARY)
    private val strong = text(TOTAL_SIZE, INK, bold = true)
    private val rule = Paint().apply { color = DIVIDER }

    // A sheet shows each storey alone, so the storey below it is not drawn; a wall with no
    // material is left white, as it is on screen.
    private val painter =
        PlanPainter(
            palette =
                PlanPalette(
                    wall = WALL,
                    unchosen = PAPER,
                    gap = PAPER,
                    edge = INK,
                    below = Color.TRANSPARENT,
                    text = SECONDARY,
                    selection = Color.TRANSPARENT,
                    warning = Color.TRANSPARENT,
                ),
            hairline = HAIRLINE,
            bold = HAIRLINE,
            textSize = LABEL_SIZE,
            about = resources.getString(R.string.length_about_mark),
        )

    // Where a storey's plan lies in its box: as large as the box allows, with room around the
    // walls for the lengths written outside them.
    private class Placed(
        val left: Double,
        val bottom: Double,
        val tall: Double,
        val scale: Float,
    ) {
        val height: Float get() = (tall * scale).toFloat() + 2 * LABEL_ROOM
    }

    private fun place(storey: Storey): Placed? {
        val points = storey.outlines.flatMap { it.points }
        if (points.isEmpty()) return null
        val left = points.minOf { it.x }
        val bottom = points.minOf { it.y }
        val wide = (points.maxOf { it.x } - left).coerceAtLeast(MIN_EXTENT)
        val tall = (points.maxOf { it.y } - bottom).coerceAtLeast(MIN_EXTENT)
        val scale = minOf((CONTENT - 2 * LABEL_ROOM) / wide, (PLAN_HEIGHT - 2 * LABEL_ROOM) / tall).toFloat()
        return Placed(left, bottom, tall, scale)
    }

    /**
     * How big the sheet's label text is on each storey's plan, from the bottom up; null for a
     * storey with no walls. The engine draws each storey for its size.
     */
    fun labels(state: EditorUiState): List<LabelSize?> = state.storeys.map { storey -> place(storey)?.let { painter.labelSize(it.scale) } }

    /** [drawings] are the storeys' drawings, asked with [labels]. */
    fun write(
        state: EditorUiState,
        drawings: List<Drawing?>,
        date: LocalDate,
        out: OutputStream,
    ) {
        val document = PdfDocument()
        val sheet = Sheet(document)
        sheet.header(state, date)
        sheet.plans(state, drawings)
        sheet.table(state)
        sheet.finish()
        document.writeTo(out)
        document.close()
    }

    // The page being drawn and how far down it is filled.
    private inner class Sheet(
        private val document: PdfDocument,
    ) {
        private var number = 0
        private var page = newPage()
        private var y = MARGIN

        private val canvas: Canvas get() = page.canvas

        private fun newPage(): PdfDocument.Page =
            document.startPage(PdfDocument.PageInfo.Builder(PAGE_WIDTH, PAGE_HEIGHT, ++number).create())

        fun finish() = document.finishPage(page)

        // Starts another page when [height] more does not fit on this one.
        private fun room(height: Float) {
            if (y + height <= PAGE_HEIGHT - MARGIN) return
            document.finishPage(page)
            page = newPage()
            y = MARGIN
        }

        fun header(
            state: EditorUiState,
            date: LocalDate,
        ) {
            y += TITLE_SIZE
            canvas.drawText(fit(state.name.ifBlank { resources.getString(R.string.editor_title) }, title, CONTENT), MARGIN, y, title)
            y += LINE
            canvas.drawText(resources.getString(R.string.takeoff_title), MARGIN, y, heading)
            // The language of the strings, not of the phone: Russian text gets a Russian date.
            val locale = Locale.forLanguageTag(resources.getString(R.string.editor_date_locale))
            val day = date.format(DateTimeFormatter.ofLocalizedDate(FormatStyle.LONG).withLocale(locale))
            right(day, RIGHT, heading)
            y += LINE
        }

        // Each storey as the engine drew it, in a box of its own.
        fun plans(
            state: EditorUiState,
            drawings: List<Drawing?>,
        ) {
            state.storeys.forEachIndexed { index, storey ->
                val placed = place(storey)
                val drawing = drawings.getOrNull(index)
                if (placed != null && drawing != null) plan(placed, drawing, resources.getString(R.string.level_name, index + 1))
            }
        }

        private fun plan(
            placed: Placed,
            drawing: Drawing,
            name: String,
        ) {
            room(LINE + placed.height)
            canvas.drawText(name, MARGIN, y + TEXT_SIZE, heading)
            y += LINE
            val frame =
                PlanFrame(
                    originX = MARGIN + LABEL_ROOM - (placed.left * placed.scale).toFloat(),
                    originY = y + placed.height - LABEL_ROOM + (placed.bottom * placed.scale).toFloat(),
                    scale = placed.scale,
                )
            painter.paint(canvas, drawing, frame)
            y += placed.height + ROW_GAP
        }

        fun table(state: EditorUiState) {
            val lines = state.takeoff.filter { it.amount != 0.0 }
            val bought = bought(lines)
            canvas.drawText(resources.getString(R.string.estimate_material), MARGIN, y + TEXT_SIZE, heading)
            y += TEXT_SIZE
            right(resources.getString(R.string.estimate_amount), AMOUNT_RIGHT, heading)
            right(resources.getString(R.string.estimate_price), PRICE_RIGHT, heading)
            right(resources.getString(R.string.estimate_cost), RIGHT, heading)
            y += ROW_GAP
            line()
            for (line in lines) {
                room(ROW)
                row(line, state, priced = line in bought)
            }
            val sum = total(lines, state.prices) ?: return
            room(ROW + LINE)
            y += TOTAL_SIZE + ROW_GAP
            canvas.drawText(resources.getString(R.string.takeoff_total), MARGIN, y, strong)
            right(moneyText(resources, sum), RIGHT, strong)
            val unpriced = bought.count { it.item !in state.prices }
            if (unpriced > 0) {
                y += LINE
                canvas.drawText(resources.getString(R.string.takeoff_unpriced, unpriced), MARGIN, y, quiet)
            }
        }

        // A mix is listed for its amount only; its parts carry the prices.
        private fun row(
            line: QuantityLine,
            state: EditorUiState,
            priced: Boolean,
        ) {
            val paint = if (priced) body else quiet
            y += TEXT_SIZE + ROW_GAP
            canvas.drawText(fit(itemName(resources, line.item, state.catalog.units), paint, NAME_WIDTH), MARGIN, y, paint)
            right("${amountText(line)} ${unitName(resources, line.unit)}", AMOUNT_RIGHT, paint)
            if (priced) {
                state.prices[line.item]?.let { right(moneyText(resources, it), PRICE_RIGHT, paint) }
                cost(line, state.prices)?.let { right(moneyText(resources, it), RIGHT, paint) }
            }
            y += ROW_GAP
            line()
        }

        private fun line() = canvas.drawLine(MARGIN, y, RIGHT, y, rule)

        private fun right(
            text: String,
            edge: Float,
            paint: Paint,
        ) = canvas.drawText(text, edge - paint.measureText(text), y, paint)
    }

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
        // A4 at 72 points to the inch.
        const val PAGE_WIDTH = 595
        const val PAGE_HEIGHT = 842
        const val MARGIN = 40f
        const val RIGHT = PAGE_WIDTH - MARGIN
        const val CONTENT = RIGHT - MARGIN
        const val PLAN_HEIGHT = 220f
        const val NAME_WIDTH = 230f
        const val AMOUNT_RIGHT = 370f
        const val PRICE_RIGHT = 460f
        const val TITLE_SIZE = 18f
        const val TOTAL_SIZE = 12f
        const val TEXT_SIZE = 10f
        const val LINE = 18f
        const val ROW_GAP = 6f
        const val ROW = TEXT_SIZE + 2 * ROW_GAP
        const val HAIRLINE = 0.6f
        const val LABEL_SIZE = 7f
        const val LABEL_ROOM = 14f

        // A single short wall is not blown up to fill the box: at least this many metres show.
        const val MIN_EXTENT = 4.0
        const val INK = 0xFF1B2027.toInt()
        const val SECONDARY = 0xFF4B5563.toInt()
        const val DIVIDER = 0xFFE3E6EA.toInt()
        const val WALL = 0xFFB5BCC6.toInt()
        const val PAPER = 0xFFFFFFFF.toInt()
    }
}

/**
 * Writes the estimate into the cache and offers it to the apps that take a PDF. [sheets] asks the
 * engine for every storey's drawing with label text of the sizes the sheet uses.
 */
internal suspend fun Context.shareEstimate(
    state: EditorUiState,
    sheets: suspend (List<LabelSize?>) -> List<Drawing?>,
) {
    val pdf = EstimatePdf(resources)
    val drawings = sheets(pdf.labels(state))
    val file =
        withContext(Dispatchers.IO) {
            val folder = File(cacheDir, "estimates").apply { mkdirs() }
            val name =
                state.name
                    .ifBlank { getString(R.string.editor_title) }
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
