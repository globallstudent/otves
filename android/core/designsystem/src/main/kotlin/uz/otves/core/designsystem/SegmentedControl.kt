package uz.otves.core.designsystem

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.selection.selectableGroup
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp

/** One choice out of a few, as a white thumb on a field-coloured track. */
@Composable
public fun SegmentedControl(
    options: List<String>,
    selected: Int,
    onSelect: (Int) -> Unit,
    modifier: Modifier = Modifier,
) {
    val colors = OtvesTheme.colors
    Row(
        modifier =
            modifier
                .height(OtvesDimens.minTarget)
                .clip(TrackShape)
                .background(colors.field)
                .padding(TrackInset)
                .selectableGroup(),
    ) {
        options.forEachIndexed { index, label ->
            val chosen = index == selected
            Box(
                contentAlignment = Alignment.Center,
                modifier =
                    Modifier
                        .weight(1f)
                        .fillMaxHeight()
                        .shadow(if (chosen) ThumbElevation else 0.dp, ThumbShape)
                        .clip(ThumbShape)
                        .background(if (chosen) colors.surface else Color.Transparent)
                        .selectable(selected = chosen, role = Role.Tab, onClick = { onSelect(index) }),
            ) {
                Text(
                    text = label,
                    style = MaterialTheme.typography.labelLarge,
                    color = if (chosen) colors.ink else colors.inkSecondary,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                    modifier = Modifier.padding(horizontal = 8.dp),
                )
            }
        }
    }
}

private val TrackInset = 3.dp
private val TrackShape = RoundedCornerShape(10.dp)
private val ThumbShape = RoundedCornerShape(8.dp)
private val ThumbElevation = 1.dp

@Preview(widthDp = 360)
@Composable
private fun SegmentedControlPreview() {
    OtvesTheme {
        SegmentedControl(options = listOf("Стена", "Окно", "Дверь"), selected = 1, onSelect = {})
    }
}
