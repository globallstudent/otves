plugins {
    alias(libs.plugins.otves.jvm.library)
}

kotlin {
    explicitApi()
}

dependencies {
    testImplementation(libs.junit4)
    testImplementation(libs.truth)
}
