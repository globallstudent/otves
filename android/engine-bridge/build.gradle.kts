plugins {
    alias(libs.plugins.otves.android.library)
}

// vcpkg builds the engine's C++ dependencies. Without it the native build stops at this path.
val vcpkgRoot: String =
    providers.environmentVariable("VCPKG_ROOT").orNull
        ?: providers.gradleProperty("otves.vcpkg.dir").orNull
        ?: "set-VCPKG_ROOT-or-otves.vcpkg.dir"

// Java for schema/engine.fbs, made by the flatc release that matches flatbuffers-java.
val generateSchemaJava =
    tasks.register<FlatcJavaTask>("generateSchemaJava") {
        schema.set(layout.projectDirectory.file("../../schema/engine.fbs"))
        flatcVersion.set(libs.versions.flatbuffers)
        packagePrefix.set("uz")
        outputDirectory.set(layout.buildDirectory.dir("generated/flatc"))
        toolDirectory.set(layout.buildDirectory.dir("flatc"))
    }

androidComponents {
    onVariants { variant ->
        variant.sources.java?.addGeneratedSourceDirectory(generateSchemaJava, FlatcJavaTask::outputDirectory)
    }
}

android {
    namespace = "uz.otves.engine"
    ndkVersion = libs.versions.ndk.get()

    defaultConfig {
        ndk {
            abiFilters += "arm64-v8a"
        }
        externalNativeBuild {
            cmake {
                targets += "otves_design"
                arguments +=
                    listOf(
                        "-DCMAKE_TOOLCHAIN_FILE=$vcpkgRoot/scripts/buildsystems/vcpkg.cmake",
                        "-DVCPKG_TARGET_TRIPLET=arm64-android",
                        "-DOTVES_BUILD_TESTS=OFF",
                    )
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../../engine/CMakeLists.txt")
            version = libs.versions.cmake.get()
        }
    }

    lint {
        // The app targets arm64-v8a phones only; x86_64 ChromeOS devices are not a target.
        disable += "ChromeOsAbiSupport"
    }
}

kotlin {
    explicitApi()
}

dependencies {
    api(projects.core.model)
    implementation(libs.flatbuffers.java)
    testImplementation(libs.junit4)
    testImplementation(libs.truth)
}
