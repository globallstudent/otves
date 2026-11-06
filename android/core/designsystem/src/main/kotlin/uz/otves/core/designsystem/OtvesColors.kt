package uz.otves.core.designsystem

import androidx.compose.runtime.Immutable
import androidx.compose.runtime.staticCompositionLocalOf
import androidx.compose.ui.graphics.Color

/**
 * Colour roles of the design system.
 *
 * [selection] is only for what is selected or edited on the plan; commands use [ink];
 * [warning] is only for errors.
 */
@Immutable
public data class OtvesColors(
    val ink: Color,
    val inkSecondary: Color,
    val inkTertiary: Color,
    val canvas: Color,
    val surface: Color,
    val border: Color,
    val divider: Color,
    val field: Color,
    val selection: Color,
    val warning: Color,
    val brass: Color,
    val brassShade: Color,
)

public val LightOtvesColors: OtvesColors =
    OtvesColors(
        ink = Color(0xFF1B2027),
        inkSecondary = Color(0xFF4B5563),
        inkTertiary = Color(0xFF8A93A0),
        canvas = Color(0xFFF7F8FA),
        surface = Color(0xFFFFFFFF),
        border = Color(0xFFE3E6EA),
        divider = Color(0xFFEDEFF2),
        field = Color(0xFFF1F3F5),
        selection = Color(0xFF1F56E0),
        warning = Color(0xFFB4410E),
        brass = Color(0xFFD9A84E),
        brassShade = Color(0xFFA9772A),
    )

internal val LocalOtvesColors = staticCompositionLocalOf { LightOtvesColors }
