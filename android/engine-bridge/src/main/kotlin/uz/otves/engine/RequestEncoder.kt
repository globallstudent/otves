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
import uz.otves.schema.Aim as AimTable
import uz.otves.schema.AimMeasured as AimMeasuredTable
import uz.otves.schema.BeginGesture as BeginGestureTable
import uz.otves.schema.Conditions as ConditionsTable
import uz.otves.schema.ConditionsField as ConditionsFieldCode
import uz.otves.schema.ElementChange as ChangeTable
import uz.otves.schema.EndGesture as EndGestureTable
import uz.otves.schema.FixedEnd as FixedEndCode
import uz.otves.schema.Level as LevelTable
import uz.otves.schema.LoadContent as LoadContentTable
import uz.otves.schema.LocationLine as LocationLineCode
import uz.otves.schema.MoveOpening as MoveOpeningTable
import uz.otves.schema.MoveWallEnd as MoveWallEndTable
import uz.otves.schema.OpenModel as OpenModelTable
import uz.otves.schema.Opening as OpeningTable
import uz.otves.schema.OpeningKind as OpeningKindCode
import uz.otves.schema.Pick as PickTable
import uz.otves.schema.PlaceOpening as PlaceOpeningTable
import uz.otves.schema.PointMm as PointTable
import uz.otves.schema.Redo as RedoTable
import uz.otves.schema.RemoveLevel as RemoveLevelTable
import uz.otves.schema.RemoveOpening as RemoveOpeningTable
import uz.otves.schema.RemoveWall as RemoveWallTable
import uz.otves.schema.ResizeOpening as ResizeOpeningTable
import uz.otves.schema.Script as ScriptTable
import uz.otves.schema.SetCondition as SetConditionTable
import uz.otves.schema.SetLevelElevation as SetLevelElevationTable
import uz.otves.schema.SetLevelFooting as SetLevelFootingTable
import uz.otves.schema.SetLevelHeight as SetLevelHeightTable
import uz.otves.schema.SetLevelJunctionMeshes as SetLevelJunctionMeshesTable
import uz.otves.schema.SetLevelRingBeam as SetLevelRingBeamTable
import uz.otves.schema.SetOpeningKind as SetOpeningKindTable
import uz.otves.schema.SetOpeningSill as SetOpeningSillTable
import uz.otves.schema.SetWallHeight as SetWallHeightTable
import uz.otves.schema.SetWallLength as SetWallLengthTable
import uz.otves.schema.SetWallLocationLine as SetWallLocationLineTable
import uz.otves.schema.SetWallThickness as SetWallThicknessTable
import uz.otves.schema.SetWallUnit as SetWallUnitTable
import uz.otves.schema.Turn as TurnCode
import uz.otves.schema.Undo as UndoTable
import uz.otves.schema.Uuid as UuidStruct
import uz.otves.schema.Wall as WallTable
import uz.otves.schema.WallEnd as WallEndCode

/** Builds schema `Request` buffers (schema/engine.fbs). Every enum maps explicitly. */
internal object RequestEncoder {
    private const val VERSION = 1L
    private const val INITIAL_SIZE = 1024

    // The schema's value for a condition left to the norm.
    private const val UNSET = -1

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
                Command.AddLevel to AddLevelTable.createAddLevel(this, level(command.level))
            }

            is EngineCommand.SetLevelRingBeam -> {
                SetLevelRingBeamTable.startSetLevelRingBeam(this)
                SetLevelRingBeamTable.addLevel(this, uuid(command.level))
                SetLevelRingBeamTable.addHeight(this, command.height.value)
                Command.SetLevelRingBeam to SetLevelRingBeamTable.endSetLevelRingBeam(this)
            }

            is EngineCommand.SetLevelFooting -> {
                SetLevelFootingTable.startSetLevelFooting(this)
                SetLevelFootingTable.addLevel(this, uuid(command.level))
                SetLevelFootingTable.addWidth(this, command.footing?.width?.value ?: 0)
                SetLevelFootingTable.addDepth(this, command.footing?.depth?.value ?: 0)
                Command.SetLevelFooting to SetLevelFootingTable.endSetLevelFooting(this)
            }

            is EngineCommand.SetLevelJunctionMeshes -> {
                SetLevelJunctionMeshesTable.startSetLevelJunctionMeshes(this)
                SetLevelJunctionMeshesTable.addLevel(this, uuid(command.level))
                SetLevelJunctionMeshesTable.addMeshes(this, command.meshes)
                Command.SetLevelJunctionMeshes to SetLevelJunctionMeshesTable.endSetLevelJunctionMeshes(this)
            }

            is EngineCommand.SetCondition -> {
                val field =
                    when (command.condition) {
                        Condition.SEISMIC_INTENSITY -> ConditionsFieldCode.SeismicIntensity
                        Condition.CEMENT_GRADE -> ConditionsFieldCode.CementGrade
                        Condition.MORTAR_GRADE -> ConditionsFieldCode.MortarGrade
                        Condition.UNIT_WASTE -> ConditionsFieldCode.UnitWaste
                        Condition.MORTAR_WASTE -> ConditionsFieldCode.MortarWaste
                    }
                Command.SetCondition to SetConditionTable.createSetCondition(this, field, (command.value ?: UNSET).toLong())
            }

            is EngineCommand.SetWallUnit -> {
                SetWallUnitTable.startSetWallUnit(this)
                SetWallUnitTable.addWall(this, uuid(command.wall))
                SetWallUnitTable.addLength(this, command.unit?.length?.value ?: 0)
                SetWallUnitTable.addWidth(this, command.unit?.width?.value ?: 0)
                SetWallUnitTable.addHeight(this, command.unit?.height?.value ?: 0)
                Command.SetWallUnit to SetWallUnitTable.endSetWallUnit(this)
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

            is EngineCommand.Pick -> {
                PickTable.startPick(this)
                PickTable.addLevel(this, uuid(command.level))
                PickTable.addAt(this, point(command.at))
                PickTable.addReach(this, command.reach.value)
                Command.Pick to PickTable.endPick(this)
            }

            is EngineCommand.Aim -> {
                AimTable.startAim(this)
                AimTable.addLevel(this, uuid(command.level))
                command.from?.let { AimTable.addFrom(this, point(it)) }
                AimTable.addAt(this, point(command.at))
                AimTable.addReach(this, command.reach.value)
                AimTable.addGrid(this, command.grid.value)
                AimTable.addSquare(this, command.square)
                Command.Aim to AimTable.endAim(this)
            }

            is EngineCommand.AimMeasured -> {
                AimMeasuredTable.startAimMeasured(this)
                AimMeasuredTable.addLevel(this, uuid(command.level))
                AimMeasuredTable.addFrom(this, point(command.from))
                command.before?.let { AimMeasuredTable.addBefore(this, point(it)) }
                AimMeasuredTable.addTurn(this, code(command.turn))
                AimMeasuredTable.addLength(this, command.length.value)
                Command.AimMeasured to AimMeasuredTable.endAimMeasured(this)
            }

            is EngineCommand.SetWallLength -> {
                SetWallLengthTable.startSetWallLength(this)
                SetWallLengthTable.addWall(this, uuid(command.wall))
                SetWallLengthTable.addLength(this, command.length.value)
                SetWallLengthTable.addFixed(
                    this,
                    when (command.fixed) {
                        null -> FixedEndCode.Joined
                        WallEnd.START -> FixedEndCode.Start
                        WallEnd.END -> FixedEndCode.End
                    },
                )
                Command.SetWallLength to SetWallLengthTable.endSetWallLength(this)
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
            is Level -> Element.Level to level(element)
            is Wall -> Element.Wall to wall(element)
            is Opening -> Element.Opening to opening(element)
            is Conditions -> Element.Conditions to conditions(element)
        }

    private fun FlatBufferBuilder.conditions(conditions: Conditions): Int =
        ConditionsTable.createConditions(
            this,
            (conditions.seismicIntensity ?: UNSET).toLong(),
            (conditions.cementGrade ?: UNSET).toLong(),
            (conditions.mortarGrade ?: UNSET).toLong(),
            (conditions.unitWaste ?: UNSET).toLong(),
            (conditions.mortarWaste ?: UNSET).toLong(),
        )

    private fun FlatBufferBuilder.level(level: Level): Int =
        LevelTable.createLevel(
            this,
            level.elevation.value,
            level.height.value,
            level.ringBeam.value,
            level.junctionMeshes,
            level.footing?.width?.value ?: 0,
            level.footing?.depth?.value ?: 0,
        )

    private fun FlatBufferBuilder.wall(wall: Wall): Int {
        WallTable.startWall(this)
        WallTable.addLevel(this, uuid(wall.level))
        WallTable.addStart(this, point(wall.start))
        WallTable.addEnd(this, point(wall.end))
        WallTable.addThickness(this, wall.thickness.value)
        WallTable.addHeight(this, wall.height.value)
        WallTable.addLocationLine(this, code(wall.locationLine))
        wall.unit?.let { unit ->
            WallTable.addUnitLength(this, unit.length.value)
            WallTable.addUnitWidth(this, unit.width.value)
            WallTable.addUnitHeight(this, unit.height.value)
        }
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

    private fun code(turn: Turn): Int =
        when (turn) {
            Turn.LEFT -> TurnCode.Left
            Turn.STRAIGHT -> TurnCode.Straight
            Turn.RIGHT -> TurnCode.Right
        }

    private fun code(end: WallEnd): Int =
        when (end) {
            WallEnd.START -> WallEndCode.Start
            WallEnd.END -> WallEndCode.End
        }
}
