plugins {
    alias(libs.plugins.otves.android.library)
    alias(libs.plugins.otves.android.compose)
    alias(libs.plugins.kotlin.serialization)
}

android {
    namespace = "uz.otves.feature.projects"
}

dependencies {
    implementation(projects.core.designsystem)
    implementation(projects.data.project)
    implementation(libs.androidx.lifecycle.viewmodel.compose)
    implementation(libs.kotlinx.coroutines.android)
    api(libs.navigation3.runtime)
    implementation(libs.kotlinx.serialization.core)
    testImplementation(libs.junit4)
    testImplementation(libs.truth)
    testImplementation(libs.kotlinx.coroutines.test)
}
