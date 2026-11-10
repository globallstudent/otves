plugins {
    alias(libs.plugins.otves.android.application)
    alias(libs.plugins.otves.android.compose)
}

android {
    namespace = "uz.otves.app"

    defaultConfig {
        applicationId = "uz.otves.app"
        versionCode = 1
        versionName = "0.1.0"
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }

    androidResources {
        generateLocaleConfig = true
    }
}

dependencies {
    implementation(projects.core.designsystem)
    implementation(projects.data.catalog)
    implementation(projects.data.project)
    implementation(projects.engineBridge)
    implementation(projects.feature.editor)
    implementation(projects.feature.projects)
    implementation(libs.navigation3.runtime)
    implementation(libs.navigation3.ui)
    implementation(libs.androidx.lifecycle.viewmodel.navigation3)
    implementation(libs.kotlinx.coroutines.android)
    implementation(libs.androidx.activity.compose)
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.core.splashscreen)
}
