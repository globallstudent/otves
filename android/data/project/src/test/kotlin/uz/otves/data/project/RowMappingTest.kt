package uz.otves.data.project

import com.google.common.truth.Truth.assertThat
import org.junit.Test
import uz.otves.core.model.ElementId
import uz.otves.core.model.mm
import uz.otves.engine.Level
import uz.otves.engine.LocationLine
import uz.otves.engine.Opening
import uz.otves.engine.OpeningKind
import uz.otves.engine.PointMm
import uz.otves.engine.Wall
import kotlin.random.Random
import kotlin.uuid.Uuid
import uz.otves.schema.LocationLine as LocationLineCode
import uz.otves.schema.OpeningKind as OpeningKindCode

class RowMappingTest {
    private val level = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000001"))
    private val wall = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000002"))
    private val opening = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000003"))

    @Test
    fun storedCodesAreTheSchemaCodes() {
        assertThat(listOf(CENTER, LEFT_FACE, RIGHT_FACE))
            .containsExactly(LocationLineCode.Center, LocationLineCode.LeftFace, LocationLineCode.RightFace)
            .inOrder()
        assertThat(listOf(WINDOW, DOOR, EMPTY))
            .containsExactly(OpeningKindCode.Window, OpeningKindCode.Door, OpeningKindCode.Empty)
            .inOrder()
    }

    @Test
    fun elementsComeBackFromTheirRows() {
        val storey = Level(elevation = (-450).mm, height = 3000.mm)
        val masonry = Wall(level, PointMm(0.mm, 0.mm), PointMm(6400.mm, (-20).mm), 380.mm, 2800.mm, LocationLine.RIGHT_FACE)
        val door = Opening(wall, 1200.mm, 900.mm, 2100.mm, 0.mm, OpeningKind.DOOR)

        assertThat(storey.row(level).element()).isEqualTo(level to storey)
        assertThat(masonry.row(wall).element()).isEqualTo(wall to masonry)
        assertThat(door.row(opening).element()).isEqualTo(opening to door)
        assertThat(masonry.row(wall).levelId).isEqualTo("019a5f00-0000-7000-8000-000000000001")
    }

    @Test(expected = CorruptJobException::class)
    fun unknownCodeIsACorruptJob() {
        Opening(wall, 0.mm, 900.mm, 2100.mm, 0.mm).row(opening).copy(kind = 7).element()
    }

    @Test
    fun jobIdsAreVersion7AndSortByTime() {
        val earlier = uuidV7(1_762_387_200_000, Random(1))
        val later = uuidV7(1_762_387_200_001, Random(2))

        assertThat(earlier.toString()[14]).isEqualTo('7')
        assertThat(earlier.toString()[19]).isAnyOf('8', '9', 'a', 'b')
        assertThat(earlier.toString()).isLessThan(later.toString())
        assertThat(earlier.toString()).startsWith("019a5676-8800-7")
    }
}
