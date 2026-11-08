package uz.otves.engine

import com.google.flatbuffers.FlatBufferBuilder
import uz.otves.core.model.ElementId
import uz.otves.schema.Command
import uz.otves.schema.CommandEntry
import uz.otves.schema.Element
import uz.otves.schema.Request
import java.nio.ByteBuffer
import uz.otves.schema.AddLevel as AddLevelTable
import uz.otves.schema.AddOpening as AddOpeningTable
import uz.otves.schema.AddWall as AddWallTable
import uz.otves.schema.BeginGesture as BeginGestureTable
import uz.otves.schema.ElementChange as ChangeTable
import uz.otves.schema.EndGesture as EndGestureTable
import uz.otves.schema.Level as LevelTable
import uz.otves.schema.LoadContent as LoadContentTable
import uz.otves.schema.LocationLine as LocationLineCode
import uz.otves.schema.MoveOpening as MoveOpeningTable
import uz.otves.schema.MoveWallEnd as MoveWallEndTable
import uz.otves.schema.OpenModel as OpenModelTable
import uz.otves.schema.Opening as OpeningTable
import uz.otves.schema.OpeningKind as OpeningKindCode
import uz.otves.schema.PlaceOpening as PlaceOpeningTable
import uz.otves.schema.PointMm as PointTable
import uz.otves.schema.Redo as RedoTable
import uz.otves.schema.RemoveLevel as RemoveLevelTable
import uz.otves.schema.RemoveOpening as RemoveOpeningTable
import uz.otves.schema.RemoveWall as RemoveWallTable
import uz.otves.schema.ResizeOpening as ResizeOpeningTable
import uz.otves.schema.Script as ScriptTable
import uz.otves.schema.SetLevelElevation as SetLevelElevationTable
import uz.otves.schema.SetLevelHeight as SetLevelHeightTable
import uz.otves.schema.SetOpeningKind as SetOpeningKindTable
import uz.otves.schema.SetOpeningSill as SetOpeningSillTable
import uz.otves.schema.SetWallHeight as SetWallHeightTable
import uz.otves.schema.SetWallLocationLine as SetWallLocationLineTable
import uz.otves.schema.SetWallThickness as SetWallThicknessTable
import uz.otves.schema.Undo as UndoTable
import uz.otves.schema.Uuid as UuidStruct
import uz.otves.schema.Wall as WallTable
import uz.otves.schema.WallEnd as WallEndCode

/** Builds schema `Request` buffers (schema/engine.fbs). Every enum maps explicitly. */
internal object RequestEncoder {
    private const val VERSION = 1L
    private const val INITIAL_SIZE = 1024

    fun encode(commands: List<EngineCommand>): ByteBuffer {
        val builder = FlatBufferBuilder(INITIAL_SIZE)
        val entries =
            commands
                .map { command ->
                    val (type, offset) = builder.table(command)
                    CommandEntry.createCommandEntry(builder, type, offset)
                }.toIntArray()
        val request = Request.createRequest(builder, VERSION, Request.createCommandsVector(builder, entries))
        Request.finishRequestBuffer(builder, request)
        return builder.dataBuffer()
    }

    // A struct is written inline, so it is built right before the field that holds it.
    private fun FlatBufferBuilder.uuid(id: ElementId): Int = id.uuid.toLongs { high, low -> UuidStruct.createUuid(this, high, low) }

    private fun FlatBufferBuilder.point(point: PointMm): Int = PointTable.createPointMm(this, point.x.value, point.y.value)

    private fun FlatBufferBuilder.table(command: EngineCommand): Pair<Byte, Int> =
        when (command) {
            is EngineCommand.AddLevel -> {
                val level = LevelTable.createLevel(this, command.level.elevation.value, command.level.height.value)
                Command.AddLevel to AddLevelTable.createAddLevel(this, level)
            }

            is EngineCommand.SetLevelElevation -> {
                SetLevelElevationTable.startSetLevelElevation(this)
                SetLevelElevationTable.addLevel(this, uuid(command.level))
                SetLevelElevationTable.addElevation(this, command.elevation.value)
                Command.SetLevelElevation to SetLevelElevationTable.endSetLevelElevation(this)
            }

            is EngineCommand.SetLevelHeight -> {
                SetLevelHeightTable.startSetLevelHeight(this)
                SetLevelHeightTable.addLevel(this, uuid(command.level))
                SetLevelHeightTable.addHeight(this, command.height.value)
                Command.SetLevelHeight to SetLevelHeightTable.endSetLevelHeight(this)
            }

            is EngineCommand.RemoveLevel -> {
                RemoveLevelTable.startRemoveLevel(this)
                RemoveLevelTable.addLevel(this, uuid(command.level))
                Command.RemoveLevel to RemoveLevelTable.endRemoveLevel(this)
            }

            is EngineCommand.AddWall -> {
                Command.AddWall to AddWallTable.createAddWall(this, wall(command.wall))
            }

            is EngineCommand.MoveWallEnd -> {
                MoveWallEndTable.startMoveWallEnd(this)
                MoveWallEndTable.addWall(this, uuid(command.wall))
                MoveWallEndTable.addEnd(this, code(command.end))
                MoveWallEndTable.addTo(this, point(command.to))
                Command.MoveWallEnd to MoveWallEndTable.endMoveWallEnd(this)
            }

            is EngineCommand.SetWallThickness -> {
                SetWallThicknessTable.startSetWallThickness(this)
                SetWallThicknessTable.addWall(this, uuid(command.wall))
                SetWallThicknessTable.addThickness(this, command.thickness.value)
                Command.SetWallThickness to SetWallThicknessTable.endSetWallThickness(this)
            }

            is EngineCommand.SetWallHeight -> {
                SetWallHeightTable.startSetWallHeight(this)
                SetWallHeightTable.addWall(this, uuid(command.wall))
                SetWallHeightTable.addHeight(this, command.height.value)
                Command.SetWallHeight to SetWallHeightTable.endSetWallHeight(this)
            }

            is EngineCommand.SetWallLocationLine -> {
                SetWallLocationLineTable.startSetWallLocationLine(this)
                SetWallLocationLineTable.addWall(this, uuid(command.wall))
                SetWallLocationLineTable.addLocationLine(this, code(command.locationLine))
                Command.SetWallLocationLine to SetWallLocationLineTable.endSetWallLocationLine(this)
            }

            is EngineCommand.RemoveWall -> {
                RemoveWallTable.startRemoveWall(this)
                RemoveWallTable.addWall(this, uuid(command.wall))
                Command.RemoveWall to RemoveWallTable.endRemoveWall(this)
            }

            is EngineCommand.AddOpening -> {
                Command.AddOpening to AddOpeningTable.createAddOpening(this, opening(command.opening))
            }

            is EngineCommand.MoveOpening -> {
                MoveOpeningTable.startMoveOpening(this)
                MoveOpeningTable.addOpening(this, uuid(command.opening))
                MoveOpeningTable.addOffset(this, command.offset.value)
                Command.MoveOpening to MoveOpeningTable.endMoveOpening(this)
            }

            is EngineCommand.ResizeOpening -> {
                ResizeOpeningTable.startResizeOpening(this)
                ResizeOpeningTable.addOpening(this, uuid(command.opening))
                ResizeOpeningTable.addWidth(this, command.width.value)
                ResizeOpeningTable.addHeight(this, command.height.value)
                Command.ResizeOpening to ResizeOpeningTable.endResizeOpening(this)
            }

            is EngineCommand.SetOpeningSill -> {
                SetOpeningSillTable.startSetOpeningSill(this)
                SetOpeningSillTable.addOpening(this, uuid(command.opening))
                SetOpeningSillTable.addSill(this, command.sill.value)
                Command.SetOpeningSill to SetOpeningSillTable.endSetOpeningSill(this)
            }

            is EngineCommand.SetOpeningKind -> {
                SetOpeningKindTable.startSetOpeningKind(this)
                SetOpeningKindTable.addOpening(this, uuid(command.opening))
                SetOpeningKindTable.addKind(this, code(command.kind))
                Command.SetOpeningKind to SetOpeningKindTable.endSetOpeningKind(this)
            }

            is EngineCommand.RemoveOpening -> {
                RemoveOpeningTable.startRemoveOpening(this)
                RemoveOpeningTable.addOpening(this, uuid(command.opening))
                Command.RemoveOpening to RemoveOpeningTable.endRemoveOpening(this)
            }

            is EngineCommand.PlaceOpening -> {
                PlaceOpeningTable.startPlaceOpening(this)
                PlaceOpeningTable.addLevel(this, uuid(command.level))
                PlaceOpeningTable.addAt(this, point(command.at))
                PlaceOpeningTable.addReach(this, command.reach.value)
                PlaceOpeningTable.addWidth(this, command.width.value)
                PlaceOpeningTable.addHeight(this, command.height.value)
                PlaceOpeningTable.addSill(this, command.sill.value)
                PlaceOpeningTable.addKind(this, code(command.kind))
                Command.PlaceOpening to PlaceOpeningTable.endPlaceOpening(this)
            }

            is EngineCommand.OpenModel -> {
                val saved =
                    command.elements
                        .map { (id, element) ->
                            val (type, value) = element(element)
                            ChangeTable.startElementChange(this)
                            ChangeTable.addId(this, uuid(id))
                            ChangeTable.addElementType(this, type)
                            ChangeTable.addElement(this, value)
                            ChangeTable.endElementChange(this)
                        }.toIntArray()
                val elements = OpenModelTable.createElementsVector(this, saved)
                Command.OpenModel to OpenModelTable.createOpenModel(this, elements, command.clock)
            }

            EngineCommand.BeginGesture -> {
                BeginGestureTable.startBeginGesture(this)
                Command.BeginGesture to BeginGestureTable.endBeginGesture(this)
            }

            EngineCommand.EndGesture -> {
                EndGestureTable.startEndGesture(this)
                Command.EndGesture to EndGestureTable.endEndGesture(this)
            }

            EngineCommand.Undo -> {
                UndoTable.startUndo(this)
                Command.Undo to UndoTable.endUndo(this)
            }

            EngineCommand.Redo -> {
                RedoTable.startRedo(this)
                Command.Redo to RedoTable.endRedo(this)
            }

            is EngineCommand.LoadContent -> {
                val norms = LoadContentTable.createNormsVector(this, scripts(command.norms))
                val rules = LoadContentTable.createRulesVector(this, scripts(command.rules))
                Command.LoadContent to LoadContentTable.createLoadContent(this, norms, rules)
            }
        }

    private fun FlatBufferBuilder.scripts(scripts: List<ContentScript>): IntArray =
        scripts
            .map { script ->
                ScriptTable.createScript(this, createString(script.name), createString(script.source))
            }.toIntArray()

    private fun FlatBufferBuilder.element(element: EngineElement): Pair<Byte, Int> =
        when (element) {
            is Level -> Element.Level to LevelTable.createLevel(this, element.elevation.value, element.height.value)
            is Wall -> Element.Wall to wall(element)
            is Opening -> Element.Opening to opening(element)
        }

    private fun FlatBufferBuilder.wall(wall: Wall): Int {
        WallTable.startWall(this)
        WallTable.addLevel(this, uuid(wall.level))
        WallTable.addStart(this, point(wall.start))
        WallTable.addEnd(this, point(wall.end))
        WallTable.addThickness(this, wall.thickness.value)
        WallTable.addHeight(this, wall.height.value)
        WallTable.addLocationLine(this, code(wall.locationLine))
        return WallTable.endWall(this)
    }

    private fun FlatBufferBuilder.opening(opening: Opening): Int {
        OpeningTable.startOpening(this)
        OpeningTable.addWall(this, uuid(opening.wall))
        OpeningTable.addOffset(this, opening.offset.value)
        OpeningTable.addWidth(this, opening.width.value)
        OpeningTable.addHeight(this, opening.height.value)
        OpeningTable.addSill(this, opening.sill.value)
        OpeningTable.addKind(this, code(opening.kind))
        return OpeningTable.endOpening(this)
    }

    private fun code(line: LocationLine): Int =
        when (line) {
            LocationLine.CENTER -> LocationLineCode.Center
            LocationLine.LEFT_FACE -> LocationLineCode.LeftFace
            LocationLine.RIGHT_FACE -> LocationLineCode.RightFace
        }

    private fun code(kind: OpeningKind): Int =
        when (kind) {
            OpeningKind.WINDOW -> OpeningKindCode.Window
            OpeningKind.DOOR -> OpeningKindCode.Door
            OpeningKind.EMPTY -> OpeningKindCode.Empty
        }

    private fun code(end: WallEnd): Int =
        when (end) {
            WallEnd.START -> WallEndCode.Start
            WallEnd.END -> WallEndCode.End
        }
}
