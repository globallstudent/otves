package uz.otves.engine

import android.content.res.AssetManager
import org.json.JSONArray
import org.json.JSONObject
import uz.otves.core.model.Mm

/**
 * A masonry unit the catalog offers (content/catalog/masonry_units.json): what it is, its mark in
 * the standard, its size as laid and the wall thicknesses it builds with the norm's joints.
 */
public data class MasonryUnitOption(
    public val id: String,
    public val kind: MasonryUnitKind,
    public val mark: String,
    public val size: UnitSize,
    public val thicknesses: List<Mm>,
)

public enum class MasonryUnitKind { BRICK, STONE, BLOCK }

/** What the app offers the builder to choose from (content/catalog); sizes come from standards. */
public data class Catalog(
    public val units: List<MasonryUnitOption> = emptyList(),
    /** Ring beam heights, the norm's minimum first. */
    public val ringBeamHeights: List<Mm> = emptyList(),
    /** The values offered for each estimating condition, with the norm default among them. */
    public val conditions: Map<Condition, ConditionChoices> = emptyMap(),
)

/** What a condition may be set to; [default] is what the norms use while it is unset. */
public data class ConditionChoices(
    public val default: Int,
    public val values: List<Int>,
)

/** Reads the content bundle (content/) that ships in the APK's assets. */
public object ContentAssets {
    private const val EXTENSION = ".luau"
    private const val UNITS = "catalog/masonry_units.json"
    private const val RING_BEAM = "catalog/ring_beam.json"
    private const val CONDITIONS = "catalog/conditions.json"

    public fun load(assets: AssetManager): EngineCommand.LoadContent =
        EngineCommand.LoadContent(norms = read(assets, "norms"), rules = read(assets, "rules"))

    public fun catalog(assets: AssetManager): Catalog =
        Catalog(
            units = masonryUnits(text(assets, UNITS)),
            ringBeamHeights = lengths(JSONObject(text(assets, RING_BEAM)).getJSONArray("heights")),
            conditions = conditions(text(assets, CONDITIONS)),
        )

    internal fun conditions(json: String): Map<Condition, ConditionChoices> {
        val all = JSONObject(json)
        return Condition.entries
            .filter { all.has(it.name.lowercase()) }
            .associateWith { condition ->
                val choices = all.getJSONObject(condition.name.lowercase())
                val values = choices.getJSONArray("values")
                ConditionChoices(choices.getInt("default"), (0 until values.length()).map(values::getInt))
            }
    }

    private fun text(
        assets: AssetManager,
        file: String,
    ): String = assets.open(file).bufferedReader().use { it.readText() }

    private fun lengths(values: JSONArray): List<Mm> = (0 until values.length()).map { Mm(values.getLong(it)) }

    /** The masonry units to offer, in catalog order; an entry of an unknown kind is left out. */
    internal fun masonryUnits(json: String): List<MasonryUnitOption> {
        val units = JSONObject(json).getJSONArray("units")
        return (0 until units.length()).mapNotNull { index ->
            val unit = units.getJSONObject(index)
            val kind = MasonryUnitKind.entries.find { it.name.equals(unit.getString("kind"), ignoreCase = true) }
            kind?.let {
                MasonryUnitOption(
                    id = unit.getString("id"),
                    kind = it,
                    mark = unit.optString("mark"),
                    size = UnitSize(Mm(unit.getLong("length")), Mm(unit.getLong("width")), Mm(unit.getLong("height"))),
                    thicknesses = lengths(unit.getJSONArray("thicknesses")),
                )
            }
        }
    }

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
