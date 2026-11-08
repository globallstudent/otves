plugins {
    alias(libs.plugins.otves.android.library)
    alias(libs.plugins.ksp)
    alias(libs.plugins.room)
}

android {
    namespace = "uz.otves.data.project"

    defaultConfig {
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }
}

kotlin {
    explicitApi()
}

room {
    schemaDirectory("$projectDir/schemas")
}

dependencies {
    api(projects.engineBridge)
    implementation(libs.room.runtime)
    implementation(libs.sqlite.bundled)
    implementation(libs.kotlinx.coroutines.android)
    ksp(libs.room.compiler)
    testImplementation(libs.junit4)
    testImplementation(libs.truth)
    androidTestImplementation(libs.androidx.test.runner)
    androidTestImplementation(libs.androidx.test.ext.junit)
    androidTestImplementation(libs.truth)
    androidTestImplementation(libs.kotlinx.coroutines.test)
}
