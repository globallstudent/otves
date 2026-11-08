import org.gradle.api.DefaultTask
import org.gradle.api.GradleException
import org.gradle.api.file.ArchiveOperations
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.file.FileSystemOperations
import org.gradle.api.file.RegularFileProperty
import org.gradle.api.provider.Property
import org.gradle.api.tasks.CacheableTask
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.InputFile
import org.gradle.api.tasks.Internal
import org.gradle.api.tasks.OutputDirectory
import org.gradle.api.tasks.PathSensitive
import org.gradle.api.tasks.PathSensitivity
import org.gradle.api.tasks.TaskAction
import org.gradle.process.ExecOperations
import java.io.File
import java.net.URI
import java.net.http.HttpClient
import java.net.http.HttpRequest
import java.net.http.HttpResponse
import java.security.MessageDigest
import javax.inject.Inject

/**
 * Generates Java for a FlatBuffers schema with a pinned flatc release (docs/adr/0006). The flatc
 * version must match the `flatbuffers-java` runtime, whose generated code checks it.
 */
@CacheableTask
abstract class FlatcJavaTask
    @Inject
    constructor(
        private val exec: ExecOperations,
        private val archives: ArchiveOperations,
        private val files: FileSystemOperations,
    ) : DefaultTask() {
        @get:InputFile
        @get:PathSensitive(PathSensitivity.NONE)
        abstract val schema: RegularFileProperty

        @get:Input
        abstract val flatcVersion: Property<String>

        @get:Input
        abstract val packagePrefix: Property<String>

        @get:OutputDirectory
        abstract val outputDirectory: DirectoryProperty

        /** Where the downloaded flatc is kept between builds. */
        @get:Internal
        abstract val toolDirectory: DirectoryProperty

        @TaskAction
        fun generate() {
            val flatc = flatc()
            files.delete { delete(outputDirectory) }
            exec.exec {
                commandLine(
                    flatc.absolutePath,
                    "--java",
                    "--java-package-prefix",
                    packagePrefix.get(),
                    "-o",
                    outputDirectory.get().asFile.absolutePath,
                    schema.get().asFile.absolutePath,
                )
            }
        }

        private fun flatc(): File {
            val version = flatcVersion.get()
            val release = RELEASES[version] ?: throw GradleException("No pinned flatc archives for $version")
            val (asset, sha256) = release[hostOs()] ?: throw GradleException("No flatc $version for ${hostOs()}")
            val directory = toolDirectory.get().dir(version).asFile
            val binary = File(directory, "flatc")
            if (binary.canExecute()) {
                return binary
            }
            val archive = File(directory, asset)
            if (!archive.isFile || sha256(archive) != sha256) {
                download("https://github.com/google/flatbuffers/releases/download/v$version/$asset", archive)
            }
            val actual = sha256(archive)
            if (actual != sha256) {
                throw GradleException("$asset has SHA-256 $actual, expected $sha256")
            }
            files.copy {
                from(archives.zipTree(archive))
                into(directory)
            }
            check(binary.setExecutable(true)) { "Cannot make $binary executable" }
            return binary
        }

        private fun download(
            url: String,
            target: File,
        ) {
            target.parentFile.mkdirs()
            val client = HttpClient.newBuilder().followRedirects(HttpClient.Redirect.NORMAL).build()
            val request = HttpRequest.newBuilder(URI.create(url)).GET().build()
            // GitHub release downloads fail now and then with a 5xx; retry with a growing pause.
            repeat(DOWNLOAD_ATTEMPTS) { attempt ->
                val response = client.send(request, HttpResponse.BodyHandlers.ofFile(target.toPath()))
                if (response.statusCode() == HTTP_OK) {
                    return
                }
                logger.warn("flatc download got HTTP ${response.statusCode()}, attempt ${attempt + 1}")
                Thread.sleep(RETRY_PAUSE_MS * (attempt + 1))
            }
            throw GradleException("Could not download $url")
        }

        private fun sha256(file: File): String =
            MessageDigest
                .getInstance("SHA-256")
                .digest(file.readBytes())
                .joinToString("") { "%02x".format(it) }

        private fun hostOs(): String {
            val name = System.getProperty("os.name").lowercase()
            return when {
                name.contains("mac") -> "mac"
                name.contains("linux") && System.getProperty("os.arch") in setOf("amd64", "x86_64") -> "linux-x64"
                else -> "$name ${System.getProperty("os.arch")}"
            }
        }

        private companion object {
            const val DOWNLOAD_ATTEMPTS = 5
            const val RETRY_PAUSE_MS = 2_000L
            const val HTTP_OK = 200

            // Release assets and their SHA-256, checked when pinned. The Mac binary is universal.
            val RELEASES =
                mapOf(
                    "25.2.10" to
                        mapOf(
                            "mac" to
                                (
                                    "Mac.flatc.binary.zip" to
                                        "eabd8e8c4d8b9381b7ea4c6324c629f9200d8bb73f94b6b28ef406efc8c06c62"
                                ),
                            "linux-x64" to
                                (
                                    "Linux.flatc.binary.clang++-18.zip" to
                                        "e3f53135eeda08b30730fc3bc457a4e773d7bdebed76245b8effee345fd82673"
                                ),
                        ),
                )
        }
    }
