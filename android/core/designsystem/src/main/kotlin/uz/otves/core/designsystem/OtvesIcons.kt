package uz.otves.core.designsystem

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.graphics.vector.addPathNodes
import androidx.compose.ui.unit.dp

/** Line icons on a 24 dp grid, drawn with a 2 dp round stroke; tint them at the call site. */
public object OtvesIcons {
    public val Back: ImageVector = icon("Back", "M15 18l-6-6 6-6")
    public val ChevronDown: ImageVector = icon("ChevronDown", "M6 9l6 6 6-6")
    public val Close: ImageVector = icon("Close", "M6 6l12 12M18 6L6 18")
    public val Undo: ImageVector = icon("Undo", "M9 14L4 9l5-5M4 9h10.5a5.5 5.5 0 0 1 0 11H11")
    public val Redo: ImageVector = icon("Redo", "M15 14l5-5-5-5M20 9H9.5a5.5 5.5 0 0 0 0 11H13")
    public val Select: ImageVector = icon("Select", "M5 3l6.5 17 2.3-7.2L21 10.5z")
    public val Wall: ImageVector = icon("Wall", "M3 8h18v8H3zM8 16l4-8M14 16l4-8")
    public val Window: ImageVector = icon("Window", "M4 5h16v14H4zM12 5v14M4 12h16")
    public val Door: ImageVector = icon("Door", "M3 20h18M6 20V6M6 6a14 14 0 0 1 14 14")
    public val List: ImageVector = icon("List", "M4 6h16M4 12h16M4 18h10")
    public val Backspace: ImageVector = icon("Backspace", "M9 5h11v14H9l-6-7zM13 9.5l5 5M18 9.5l-5 5")
    public val Plus: ImageVector = icon("Plus", "M12 5v14M5 12h14")

    private fun icon(
        name: String,
        path: String,
    ): ImageVector =
        ImageVector
            .Builder(name = name, defaultWidth = 24.dp, defaultHeight = 24.dp, viewportWidth = 24f, viewportHeight = 24f)
            .addPath(
                pathData = addPathNodes(path),
                stroke = SolidColor(Color.Black),
                strokeLineWidth = 2f,
                strokeLineCap = StrokeCap.Round,
                strokeLineJoin = StrokeJoin.Round,
            ).build()
}
