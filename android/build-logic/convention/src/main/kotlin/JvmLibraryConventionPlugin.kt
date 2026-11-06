import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.api.plugins.JavaPluginExtension
import org.gradle.kotlin.dsl.configure
import org.jetbrains.kotlin.gradle.dsl.KotlinJvmProjectExtension

/** Pure Kotlin module: no Android types, fast JVM unit tests. */
class JvmLibraryConventionPlugin : Plugin<Project> {
    override fun apply(target: Project) {
        with(target) {
            pluginManager.apply("org.jetbrains.kotlin.jvm")

            extensions.configure<JavaPluginExtension> {
                sourceCompatibility = OtvesBuild.javaVersion
                targetCompatibility = OtvesBuild.javaVersion
            }
            extensions.configure<KotlinJvmProjectExtension> {
                compilerOptions.applyOtvesDefaults()
            }
        }
    }
}
