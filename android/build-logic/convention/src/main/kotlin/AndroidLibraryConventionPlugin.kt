import com.android.build.api.dsl.LibraryExtension
import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.kotlin.dsl.configure
import org.jetbrains.kotlin.gradle.dsl.KotlinAndroidProjectExtension

class AndroidLibraryConventionPlugin : Plugin<Project> {
    override fun apply(target: Project) {
        with(target) {
            pluginManager.apply("com.android.library")

            extensions.configure<LibraryExtension> {
                compileSdk = OtvesBuild.COMPILE_SDK
                defaultConfig {
                    minSdk = OtvesBuild.MIN_SDK
                }
                compileOptions {
                    sourceCompatibility = OtvesBuild.javaVersion
                    targetCompatibility = OtvesBuild.javaVersion
                }
                lint {
                    warningsAsErrors = true
                    abortOnError = true
                }
            }
            extensions.configure<KotlinAndroidProjectExtension> {
                compilerOptions.applyOtvesDefaults()
            }
        }
    }
}
