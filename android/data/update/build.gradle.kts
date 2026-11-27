plugins {
    alias(libs.plugins.otves.android.library)
    alias(libs.plugins.kotlin.serialization)
}

android {
    namespace = "uz.otves.data.update"
}

kotlin {
    explicitApi()
}

dependencies {
    api(libs.kotlinx.coroutines.android)
    implementation(libs.androidx.core.ktx)
    implementation(libs.kotlinx.serialization.json)
    testImplementation(libs.junit4)
    testImplementation(libs.truth)
    testImplementation(libs.kotlinx.coroutines.test)
}
