package uz.otves.data.project

import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import uz.otves.core.model.ElementId
import uz.otves.engine.CommandResult
import uz.otves.engine.Conditions
import uz.otves.engine.Engine
import uz.otves.engine.EngineCommand
import uz.otves.engine.EngineError
import uz.otves.engine.EngineResponse
import uz.otves.engine.Issue
import uz.otves.engine.Level
import uz.otves.engine.Opening
import uz.otves.engine.Takeoff
import uz.otves.engine.Wall
import uz.otves.engine.WallLayer
import uz.otves.engine.WallOutline
import uz.otves.engine.WallType

/**
 * The open job as the engine holds it: every element by its id, where the walls lie and what is
 * to fix. It changes only when a [JobSession] hears from the engine.
 */
public interface JobModel {
    public val levels: Map<ElementId, Level>
    public val walls: Map<ElementId, Wall>
    public val openings: Map<ElementId, Opening>
    public val wallTypes: Map<ElementId, WallType>
    public val wallLayers: Map<ElementId, WallLayer>

    /** What the job is estimated under; null until a condition is set. */
    public val conditions: Conditions?

    /** Every wall's joined outline, storey by storey. */
    public val outlines: List<WallOutline>
    public val issues: List<Issue>

    /** Whether a step of the builder's own can be taken back. */
    public val canUndo: Boolean
    public val canRedo: Boolean

    /** The storeys from the bottom up; ones at one elevation in the order they were made. */
    public fun levelsFromBottom(): List<ElementId>
}

/** What the engine said to one step. */
public data class Outcome(
    /** One per command, in order. */
    public val results: List<CommandResult>,
    /** One per question asked after the commands, in order. */
    public val answers: List<CommandResult>,
    /** The take-off when the step changed it, else null. */
    public val takeoff: Takeoff?,
) {
    /** What the first command made. */
    public val created: ElementId?
        get() = (results.firstOrNull() as? CommandResult.Done)?.created

    /** Why the engine refused the first command it refused. */
    public val refused: EngineError?
        get() = results.filterIsInstance<CommandResult.Failed>().firstOrNull()?.error
}

/**
 * One open job: the engine that holds its model and the file that keeps it (docs/adr/0007). Every
 * command goes through here, so each response is written to the file before the next command
 * runs, and [model] is what the engine last said. Steps run one at a time, inside [inTurn].
 * Closing the session closes the job's file.
 */
public class JobSession(
    private val engine: Engine,
    private val store: JobStore,
) : AutoCloseable {
    private val turn = Mutex()
    private val mirror = Mirror()

    public val model: JobModel
        get() = mirror

    /** The job's name, as the user gave it; known once the job is open. */
    public var name: String = ""
        private set

    /**
     * Runs [step] when the steps before it are done. Each sees the model the ones before it left,
     * and the file keeps the engine's order.
     */
    public suspend fun <T> inTurn(step: suspend () -> T): T = turn.withLock { step() }

    /**
     * Opens the saved job in the engine under [content]. A job with no storey yet gets [ground];
     * undo never takes that one away.
     */
    public suspend fun open(
        content: EngineCommand.LoadContent,
        ground: Level,
    ): Outcome {
        inOrder()
        val saved = store.load()
        name = saved.name
        val commands =
            buildList {
                add(content)
                add(EngineCommand.OpenModel(saved.elements, saved.clock))
                if (saved.elements.values.none { it is Level }) add(EngineCommand.AddLevel(ground))
            }
        val response = engine.send(commands)
        store.record(response)
        mirror.apply(response, steps = 0)
        return Outcome(response.results, answers = emptyList(), response.takeoff)
    }

    /**
     * Runs [commands] and then asks [questions] in the same request, so the answers show what the
     * commands did. The response is written to the job's file.
     */
    public suspend fun execute(
        commands: List<EngineCommand>,
        questions: List<EngineCommand> = emptyList(),
    ): Outcome {
        inOrder()
        val response = engine.send(commands + questions)
        store.record(response)
        // Each group in the log is one step: undo takes one back, anything else adds one.
        mirror.apply(response, steps = response.log.sumOf { if (it.isUndo) -1 else 1 })
        return Outcome(response.results.take(commands.size), response.results.drop(commands.size), response.takeoff)
    }

    /** Asks the engine [questions] that change nothing, so nothing is written. */
    public suspend fun ask(questions: List<EngineCommand>): List<CommandResult> {
        inOrder()
        return engine.send(questions).results
    }

    override fun close(): Unit = store.close()

    private fun inOrder() = check(turn.isLocked) { "The engine is spoken to inside inTurn" }

    private class Mirror : JobModel {
        override val levels = mutableMapOf<ElementId, Level>()
        override val walls = mutableMapOf<ElementId, Wall>()
        override val openings = mutableMapOf<ElementId, Opening>()
        override val wallTypes = mutableMapOf<ElementId, WallType>()
        override val wallLayers = mutableMapOf<ElementId, WallLayer>()
        private var conditionsId: ElementId? = null
        override var conditions: Conditions? = null
        override var outlines: List<WallOutline> = emptyList()
        override var issues: List<Issue> = emptyList()
        override var canUndo = false
        override var canRedo = false

        // The steps the builder made since the job was opened. Undo stops at the first of them, so
        // it never removes the ground floor of a new job.
        private var userSteps = 0

        fun apply(
            response: EngineResponse,
            steps: Int,
        ) {
            for (change in response.changes) {
                when (val element = change.element) {
                    is Level -> {
                        levels[change.id] = element
                    }

                    is Wall -> {
                        walls[change.id] = element
                    }

                    is Opening -> {
                        openings[change.id] = element
                    }

                    is WallType -> {
                        wallTypes[change.id] = element
                    }

                    is WallLayer -> {
                        wallLayers[change.id] = element
                    }

                    is Conditions -> {
                        conditionsId = change.id
                        conditions = element
                    }

                    null -> {
                        remove(change.id)
                    }
                }
            }
            outlines = response.outlines ?: outlines
            issues = response.issues ?: issues
            userSteps += steps
            canUndo = response.canUndo && userSteps > 0
            canRedo = response.canRedo
        }

        private fun remove(id: ElementId) {
            if (conditionsId == id) {
                conditionsId = null
                conditions = null
            }
            levels.remove(id) ?: walls.remove(id) ?: openings.remove(id) ?: wallTypes.remove(id) ?: wallLayers.remove(id)
        }

        // Ids grow with time, so they order the storeys made at one elevation.
        override fun levelsFromBottom(): List<ElementId> =
            levels.entries
                .sortedWith(compareBy<Map.Entry<ElementId, Level>> { it.value.elevation }.thenBy { it.key.uuid.toString() })
                .map { it.key }
    }
}
