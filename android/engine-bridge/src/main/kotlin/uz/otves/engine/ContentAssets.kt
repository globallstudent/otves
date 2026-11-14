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

/**
 * A wall make-up the catalog offers (content/catalog/wall_types.json): what the wall is for and
 * its layers from the outer face in. The job keeps the sizes, not the [id].
 */
public data class WallTypeTemplate(
    public val id: String,
    public val role: WallRole,
    public val layers: List<LayerSpec>,
)

/**
 * The sheet plans are printed on (content/catalog/sheet.json), in millimetres: its size, how far
 * its frame lies from the edge that is bound and from the other edges, and how high its text is.
 */
public data class SheetFormat(
    public val width: Double,
    public val height: Double,
    public val binding: Double,
    public val edge: Double,
    public val textHeight: Double,
)

/** What the app offers the builder to choose from (content/catalog); sizes come from standards. */
public data class Catalog(
    public val units: List<MasonryUnitOption> = emptyList(),
    /** Ring beam heights, the norm's minimum first. */
    public val ringBeamHeights: List<Mm> = emptyList(),
    /** The values offered for each estimating condition, with the norm default among them. */
    public val conditions: Map<Condition, ConditionChoices> = emptyMap(),
    /** Wall make-ups to start a wall type from. */
    public val wallTypes: List<WallTypeTemplate> = emptyList(),
    /** The sheet plans are printed on; none until the catalog is read. */
    public val sheet: SheetFormat? = null,
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
    private const val WALL_TYPES = "catalog/wall_types.json"
    private const val SHEET = "catalog/sheet.json"

    public fun load(assets: AssetManager): EngineCommand.LoadContent =
        EngineCommand.LoadContent(norms = read(assets, "norms"), rules = read(assets, "rules"))

    public fun catalog(assets: AssetManager): Catalog {
        val units = masonryUnits(text(assets, UNITS))
        return Catalog(
            units = units,
            ringBeamHeights = lengths(JSONObject(text(assets, RING_BEAM)).getJSONArray("heights")),
            conditions = conditions(text(assets, CONDITIONS)),
            wallTypes = wallTypes(text(assets, WALL_TYPES), units),
            sheet = sheetFormat(text(assets, SHEET)),
        )
    }

    internal fun sheetFormat(json: String): SheetFormat {
        val sheet = JSONObject(json)
        val frame = sheet.getJSONObject("frame")
        return SheetFormat(
            width = sheet.getDouble("width"),
            height = sheet.getDouble("height"),
            binding = frame.getDouble("binding"),
            edge = frame.getDouble("edge"),
            textHeight = sheet.getDouble("text_height"),
        )
    }

    /**
     * The wall make-ups to offer, in catalog order, with each masonry layer's unit looked up in
     * [units]. A make-up with an unknown role, function or unit is left out.
     */
    internal fun wallTypes(
        json: String,
        units: List<MasonryUnitOption>,
    ): List<WallTypeTemplate> {
        val types = JSONObject(json).getJSONArray("types")
        return (0 until types.length()).mapNotNull { index ->
            val type = types.getJSONObject(index)
            val role = WallRole.entries.find { it.name.equals(type.getString("role"), ignoreCase = true) }
            val layers = type.getJSONArray("layers")
            val specs = (0 until layers.length()).map { layerSpec(layers.getJSONObject(it), units) }
            if (role == null || specs.isEmpty() ||
                specs.any { it == null }
            ) {
                null
            } else {
                WallTypeTemplate(type.getString("id"), role, specs.filterNotNull())
            }
        }
    }

    private fun layerSpec(
        layer: JSONObject,
        units: List<MasonryUnitOption>,
    ): LayerSpec? {
        val function = LayerFunction.entries.find { it.name.equals(layer.getString("function"), ignoreCase = true) } ?: return null
        val unit = if (layer.has("unit")) units.find { it.id == layer.getString("unit") }?.size ?: return null else null
        return LayerSpec(function, Mm(layer.getLong("thickness")), unit)
    }

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
