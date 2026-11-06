plugins {
    alias(libs.plugins.otves.android.library)
    alias(libs.plugins.otves.android.compose)
}

android {
    namespace = "uz.otves.core.designsystem"
}

kotlin {
    explicitApi()
}

dependencies {
    api(libs.androidx.compose.material3)
    api(libs.androidx.compose.foundation)
    api(libs.androidx.compose.ui)
}
