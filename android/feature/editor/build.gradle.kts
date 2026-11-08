plugins {
    alias(libs.plugins.otves.android.library)
    alias(libs.plugins.otves.android.compose)
}

android {
    namespace = "uz.otves.feature.editor"
}

dependencies {
    implementation(projects.core.designsystem)
    implementation(projects.core.model)
    implementation(projects.engineBridge)
    implementation(libs.androidx.lifecycle.viewmodel.compose)
    implementation(libs.kotlinx.coroutines.android)
    testImplementation(libs.junit4)
    testImplementation(libs.truth)
    testImplementation(libs.kotlinx.coroutines.test)
}
