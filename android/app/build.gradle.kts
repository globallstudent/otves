plugins {
    alias(libs.plugins.otves.android.application)
    alias(libs.plugins.otves.android.compose)
}

// A release build takes its version from the tag (-Potves.version=0.2.0); a local build is 0.1.0.
val appVersion = providers.gradleProperty("otves.version").orElse("0.1.0").get()
val (major, minor, patch) = appVersion.split('.').map(String::toInt)
require(minor < 100 && patch < 100) { "otves.version: minor and patch must be below 100, got $appVersion" }

// The newest release's update.json (ADR 0018). A debug build has none unless one is given.
val releaseFeed = "https://github.com/globallstudent/otves/releases/latest/download/update.json"
val updateFeed = providers.gradleProperty("otves.updateFeed")

// The signing key comes from the environment, so it never lies in the repository (docs/releasing.md).
val keystore = providers.environmentVariable("OTVES_KEYSTORE").orNull

android {
    namespace = "uz.otves.app"

    defaultConfig {
        applicationId = "uz.otves.app"
        // Grows with every release, as Android needs to take one as an update.
        versionCode = major * 10_000 + minor * 100 + patch
        versionName = appVersion
    }

    signingConfigs {
        if (keystore != null) {
            create("release") {
                storeFile = file(keystore)
                storePassword = providers.environmentVariable("OTVES_KEYSTORE_PASSWORD").get()
                keyAlias = providers.environmentVariable("OTVES_KEY_ALIAS").get()
                keyPassword = providers.environmentVariable("OTVES_KEY_PASSWORD").get()
            }
        }
    }

    buildTypes {
        debug {
            buildConfigField("String", "UPDATE_FEED", "\"${updateFeed.orElse("").get()}\"")
        }
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
            signingConfig = signingConfigs.findByName("release")
            buildConfigField("String", "UPDATE_FEED", "\"${updateFeed.orElse(releaseFeed).get()}\"")
        }
    }

    buildFeatures {
        buildConfig = true
    }

    androidResources {
        generateLocaleConfig = true
    }
}

dependencies {
    implementation(projects.core.designsystem)
    implementation(projects.data.catalog)
    implementation(projects.data.project)
    implementation(projects.data.update)
    implementation(projects.engineBridge)
    implementation(projects.feature.editor)
    implementation(projects.feature.projects)
    implementation(projects.feature.update)
    implementation(libs.navigation3.runtime)
    implementation(libs.navigation3.ui)
    implementation(libs.androidx.lifecycle.viewmodel.navigation3)
    implementation(libs.kotlinx.coroutines.android)
    implementation(libs.androidx.activity.compose)
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.core.splashscreen)
}
