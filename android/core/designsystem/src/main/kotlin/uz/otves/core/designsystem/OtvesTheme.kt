package uz.otves.core.designsystem

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.ReadOnlyComposable
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp

/** Spacing and touch-target sizes; builders work in gloves, so targets are larger than usual. */
public object OtvesDimens {
    public val gutter: Dp = 16.dp
    public val minTarget: Dp = 48.dp
    public val primaryTarget: Dp = 56.dp
}

public object OtvesTheme {
    public val colors: OtvesColors
        @Composable @ReadOnlyComposable
        get() = LocalOtvesColors.current
}

@Composable
public fun OtvesTheme(content: @Composable () -> Unit) {
    val colors = LightOtvesColors
    val scheme =
        lightColorScheme(
            primary = colors.ink,
            onPrimary = colors.surface,
            background = colors.canvas,
            onBackground = colors.ink,
            surface = colors.surface,
            onSurface = colors.ink,
            onSurfaceVariant = colors.inkSecondary,
            outline = colors.border,
            outlineVariant = colors.divider,
            error = colors.warning,
        )
    CompositionLocalProvider(LocalOtvesColors provides colors) {
        MaterialTheme(colorScheme = scheme, content = content)
    }
}
