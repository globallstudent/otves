package uz.otves.feature.editor

import com.google.common.truth.Truth.assertThat
import org.junit.Test

class PlanSheetTest {
    // The scales of content/catalog/sheet.json.
    private val scales = listOf(50, 75, 100, 200, 400, 500)

    @Test
    fun aMetreAtOneToAHundredIsTenMillimetresOfPaper() {
        // 10 mm at 72 points to the inch.
        assertThat(pointsPerMetre(100)).isWithin(1e-3f).of(28.346f)
        assertThat(pointsPerMetre(50)).isWithin(1e-3f).of(56.693f)
    }

    @Test
    fun aPlanIsDrawnAtTheLargestScaleItFitsAt() {
        // A house 8.3 m wide in a box that leaves it 480 points: 57.8 points a metre fit, so 1:50.
        assertThat(namedScale(scales, 480f / 8.3f)).isEqualTo(50)
        // 12.8 m in the same box: 37.5 points a metre; 1:75 takes 37.8, so the plan is 1:100.
        assertThat(namedScale(scales, 480f / 12.8f)).isEqualTo(100)
        // A scale that fits to the point is taken.
        assertThat(namedScale(scales, pointsPerMetre(75))).isEqualTo(75)
    }

    @Test
    fun aPlanTooLargeForEveryScaleHasNoneToName() {
        assertThat(namedScale(scales, pointsPerMetre(500) - 0.1f)).isNull()
        assertThat(namedScale(emptyList(), 100f)).isNull()
    }
}
