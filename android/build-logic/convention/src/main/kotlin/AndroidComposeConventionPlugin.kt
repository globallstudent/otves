import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.kotlin.dsl.dependencies

/** Adds Compose to an Android module that already applies an otves.android.* plugin. */
class AndroidComposeConventionPlugin : Plugin<Project> {
    override fun apply(target: Project) {
        with(target) {
            pluginManager.apply("org.jetbrains.kotlin.plugin.compose")

            pluginManager.withPlugin("com.android.base") {
                dependencies {
                    val bom = libs.findLibrary("androidx-compose-bom").get()
                    add("implementation", platform(bom))
                    add("implementation", libs.findLibrary("androidx-compose-ui-tooling-preview").get())
                    // Pins lifecycle to the catalog version; Compose alone pulls an older one.
                    add("implementation", libs.findLibrary("androidx-lifecycle-runtime-compose").get())
                    add("debugImplementation", libs.findLibrary("androidx-compose-ui-tooling").get())
                }
            }
        }
    }
}
