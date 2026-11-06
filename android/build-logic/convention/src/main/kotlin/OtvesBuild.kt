import org.gradle.api.JavaVersion
import org.gradle.api.Project
import org.gradle.api.artifacts.VersionCatalog
import org.gradle.api.artifacts.VersionCatalogsExtension
import org.gradle.kotlin.dsl.getByType
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.jetbrains.kotlin.gradle.dsl.KotlinJvmCompilerOptions

/** Shared build constants. Raise the SDK levels only to the latest stable API the pinned AGP supports. */
internal object OtvesBuild {
    const val COMPILE_SDK = 37
    const val TARGET_SDK = 37
    const val MIN_SDK = 29

    // Bytecode target. The daemon runs on JDK 25; stay at 21 until AGP documents a higher target.
    val javaVersion: JavaVersion = JavaVersion.VERSION_21
    val jvmTarget: JvmTarget = JvmTarget.JVM_21
}

internal val Project.libs: VersionCatalog
    get() = extensions.getByType<VersionCatalogsExtension>().named("libs")

internal fun KotlinJvmCompilerOptions.applyOtvesDefaults() {
    jvmTarget.set(OtvesBuild.jvmTarget)
    allWarningsAsErrors.set(true)
}
