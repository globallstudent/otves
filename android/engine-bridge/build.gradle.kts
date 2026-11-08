plugins {
    alias(libs.plugins.otves.android.library)
}

// vcpkg builds the engine's C++ dependencies. Without it the native build stops at this path.
val vcpkgRoot: String =
    providers.environmentVariable("VCPKG_ROOT").orNull
        ?: providers.gradleProperty("otves.vcpkg.dir").orNull
        ?: "set-VCPKG_ROOT-or-otves.vcpkg.dir"

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
