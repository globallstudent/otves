import com.android.build.api.dsl.ApplicationExtension
import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.kotlin.dsl.configure
import org.jetbrains.kotlin.gradle.dsl.KotlinAndroidProjectExtension

class AndroidApplicationConventionPlugin : Plugin<Project> {
    override fun apply(target: Project) {
        with(target) {
            pluginManager.apply("com.android.application")

            extensions.configure<ApplicationExtension> {
                compileSdk = OtvesBuild.COMPILE_SDK
                defaultConfig {
                    minSdk = OtvesBuild.MIN_SDK
                    targetSdk = OtvesBuild.TARGET_SDK
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
