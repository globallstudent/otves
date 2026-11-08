package uz.otves.engine

import android.content.res.AssetManager

/** Reads the content bundle (content/norms, content/rules) that ships in the APK's assets. */
public object ContentAssets {
    private const val EXTENSION = ".luau"

    public fun load(assets: AssetManager): EngineCommand.LoadContent =
        EngineCommand.LoadContent(norms = read(assets, "norms"), rules = read(assets, "rules"))

    // Sorted, so rules run and report in the same order on every device.
    private fun read(
        assets: AssetManager,
        folder: String,
    ): List<ContentScript> =
        assets
            .list(folder)
            .orEmpty()
            .filter { it.endsWith(EXTENSION) }
            .sorted()
            .map { file ->
                val source = assets.open("$folder/$file").bufferedReader().use { it.readText() }
                ContentScript(name = file.removeSuffix(EXTENSION), source = source)
            }
}
