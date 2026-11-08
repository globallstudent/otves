package uz.otves.engine

/** What features need from the engine; [EngineGateway] is the real one, tests use fakes. */
public interface Engine {
    /** Runs [commands] in order; one that fails changes nothing and the rest still run. */
    public suspend fun send(commands: List<EngineCommand>): EngineResponse
}
