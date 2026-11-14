package uz.otves.data.project

import com.google.common.truth.Truth.assertThat
import org.junit.Assert.assertThrows
import org.junit.Test
import uz.otves.core.model.ElementId
import uz.otves.core.model.mm
import uz.otves.engine.Hinge
import uz.otves.engine.LayerFunction
import uz.otves.engine.Level
import uz.otves.engine.LocationLine
import uz.otves.engine.Opening
import uz.otves.engine.OpeningKind
import uz.otves.engine.PointMm
import uz.otves.engine.Room
import uz.otves.engine.RoomKind
import uz.otves.engine.Swing
import uz.otves.engine.UnitSize
import uz.otves.engine.Wall
import uz.otves.engine.WallLayer
import uz.otves.engine.WallRole
import uz.otves.engine.WallType
import kotlin.random.Random
import kotlin.uuid.Uuid
import uz.otves.schema.Hinge as HingeCode
import uz.otves.schema.LayerFunction as LayerFunctionCode
import uz.otves.schema.LocationLine as LocationLineCode
import uz.otves.schema.OpeningKind as OpeningKindCode
import uz.otves.schema.RoomKind as RoomKindCode
import uz.otves.schema.Swing as SwingCode
import uz.otves.schema.WallRole as WallRoleCode

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
        assertThat(listOf(NEAR, FAR)).containsExactly(HingeCode.Near, HingeCode.Far).inOrder()
        assertThat(listOf(LEFT, RIGHT)).containsExactly(SwingCode.Left, SwingCode.Right).inOrder()
        assertThat(listOf(EXTERIOR, BEARING, PARTITION))
            .containsExactly(WallRoleCode.Exterior, WallRoleCode.Bearing, WallRoleCode.Partition)
            .inOrder()
        assertThat(listOf(STRUCTURE, INSULATION, FACING, GAP))
            .containsExactly(LayerFunctionCode.Structure, LayerFunctionCode.Insulation, LayerFunctionCode.Facing, LayerFunctionCode.Gap)
            .inOrder()
    }

    @Test
    fun wallTypesLayersAndTypedWallsComeBackFromTheirRows() {
        val type = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000007"))
        val layer = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000008"))
        val outside = WallType(WallRole.EXTERIOR)
        val facing = WallLayer(type, position = 0, LayerFunction.FACING, 120.mm, UnitSize(250.mm, 120.mm, 65.mm))
        val wool = WallLayer(type, position = 1, LayerFunction.INSULATION, 100.mm)
        val typed = Wall(level, PointMm(0.mm, 0.mm), PointMm(6000.mm, 0.mm), 0.mm, 2800.mm, type = type, mirrored = true)

        assertThat(outside.row(type).element()).isEqualTo(type to outside)
        assertThat(facing.row(layer).element()).isEqualTo(layer to facing)
        assertThat(wool.row(layer).element()).isEqualTo(layer to wool)
        assertThat(typed.row(wall).element()).isEqualTo(wall to typed)
        assertThat(typed.row(wall).typeId).isEqualTo("019a5f00-0000-7000-8000-000000000007")
    }

    @Test
    fun aRoomComesBackFromItsRowWithItsKindAsTheSchemaCodesIt() {
        val room = ElementId(Uuid.parse("019a5f00-0000-7000-8000-000000000009"))
        val bedroom = Room(level, PointMm(2_000.mm, (-1_500).mm), RoomKind.BEDROOM)

        assertThat(bedroom.row(room).element()).isEqualTo(room to bedroom)
        assertThat(bedroom.row(room).kind).isEqualTo(RoomKindCode.Bedroom)
        assertThat(bedroom.row(room).levelId).isEqualTo("019a5f00-0000-7000-8000-000000000001")
        // A kind this app does not know is a job from a newer one.
        assertThrows(CorruptJobException::class.java) { bedroom.row(room).copy(kind = 200).element() }
    }

    @Test
    fun elementsComeBackFromTheirRows() {
        val storey = Level(elevation = (-450).mm, height = 3000.mm)
        val masonry = Wall(level, PointMm(0.mm, 0.mm), PointMm(6400.mm, (-20).mm), 380.mm, 2800.mm, LocationLine.RIGHT_FACE)
        val door = Opening(wall, 1200.mm, 900.mm, 2100.mm, 0.mm, OpeningKind.DOOR, Hinge.FAR, Swing.RIGHT)

        assertThat(storey.row(level).element()).isEqualTo(level to storey)
        assertThat(masonry.row(wall).element()).isEqualTo(wall to masonry)
        assertThat(door.row(opening).element()).isEqualTo(opening to door)
        assertThat(masonry.row(wall).levelId).isEqualTo("019a5f00-0000-7000-8000-000000000001")
    }

    @Test
    fun aSketchedWallStaysNotExact() {
        val sketched = Wall(level, PointMm(0.mm, 0.mm), PointMm(6000.mm, 0.mm), 380.mm, 2800.mm, exact = false)

        assertThat(sketched.row(wall).exact).isFalse()
        assertThat(sketched.row(wall).element()).isEqualTo(wall to sketched)
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
