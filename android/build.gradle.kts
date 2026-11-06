plugins {
    alias(libs.plugins.android.application) apply false
    alias(libs.plugins.android.library) apply false
    alias(libs.plugins.kotlin.jvm) apply false
    alias(libs.plugins.compose.compiler) apply false
    alias(libs.plugins.spotless)
}

spotless {
    val ktlintVersion = libs.versions.ktlint.get()
    // One .editorconfig for the whole repository, shared with the IDE.
    val editorConfig = rootProject.file("../.editorconfig")
    val ktlintOverrides = mapOf("ktlint_function_naming_ignore_when_annotated_with" to "Composable")
    kotlin {
        target("**/*.kt")
        targetExclude("**/build/**")
        ktlint(ktlintVersion).setEditorConfigPath(editorConfig).editorConfigOverride(ktlintOverrides)
    }
    kotlinGradle {
        target("**/*.gradle.kts")
        targetExclude("**/build/**")
        ktlint(ktlintVersion).setEditorConfigPath(editorConfig)
    }
}
