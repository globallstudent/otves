package uz.otves.data.update

import com.google.common.truth.Truth.assertThat
import com.sun.net.httpserver.HttpServer
import kotlinx.coroutines.flow.toList
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.assertThrows
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File
import java.io.IOException
import java.net.InetSocketAddress
import java.security.MessageDigest

class ApkDownloadTest {
    @get:Rule
    val folder = TemporaryFolder()

    private val content = ByteArray(300_000) { (it % 251).toByte() }
    private val hash = MessageDigest.getInstance("SHA-256").digest(content).toHexString()

    private lateinit var server: HttpServer
    private lateinit var directory: File

    // What the server was asked for, and what it does with a Range header.
    private val ranges = mutableListOf<String?>()
    private var honoursRanges = true
    private var served = content

    @Before
    fun setUp() {
        directory = folder.newFolder("update")
        server = HttpServer.create(InetSocketAddress("127.0.0.1", 0), 0)
        server.createContext("/otves.apk") { exchange ->
            val range = exchange.requestHeaders.getFirst("Range")
            ranges += range
            val from = range?.removePrefix("bytes=")?.removeSuffix("-")?.toInt()
            when {
                from == null || !honoursRanges -> {
                    exchange.sendResponseHeaders(200, served.size.toLong())
                    exchange.responseBody.use { it.write(served) }
                }

                from >= served.size -> {
                    exchange.sendResponseHeaders(416, -1)
                    exchange.close()
                }

                else -> {
                    exchange.sendResponseHeaders(206, (served.size - from).toLong())
                    exchange.responseBody.use { it.write(served, from, served.size - from) }
                }
            }
        }
        server.start()
    }

    @After
    fun tearDown() = server.stop(0)

    private fun release(sha256: String = hash) =
        Release(
            versionCode = 101,
            versionName = "0.1.1",
            apkUrl = "http://127.0.0.1:${server.address.port}/otves.apk",
            apkSha256 = sha256,
            apkSize = content.size.toLong(),
            notes = emptyMap(),
        )

    private fun download(free: Long = Long.MAX_VALUE) = ApkDownload(directory, freeSpace = { free })

    private val apk get() = File(directory, "otves-101.apk")
    private val part get() = File(directory, "otves-101.apk.part")

    @Test
    fun theWholeFileIsFetchedAndChecked() =
        runTest {
            val steps = download().fetch(release()).toList()

            assertThat(steps.first()).isEqualTo(Download.Progress(0, content.size.toLong()))
            assertThat(steps.last()).isEqualTo(Download.Ready(apk))
            assertThat(apk.readBytes()).isEqualTo(content)
            assertThat(part.exists()).isFalse()
        }

    @Test
    fun aPartialFileIsContinued() =
        runTest {
            part.writeBytes(content.copyOf(120_000))

            val steps = download().fetch(release()).toList()

            assertThat(ranges).containsExactly("bytes=120000-")
            assertThat(steps.first()).isEqualTo(Download.Progress(120_000, content.size.toLong()))
            assertThat(steps.last()).isEqualTo(Download.Ready(apk))
            assertThat(apk.readBytes()).isEqualTo(content)
        }

    @Test
    fun aServerThatIgnoresRangesSendsTheFileAgain() =
        runTest {
            honoursRanges = false
            part.writeBytes(content.copyOf(120_000))

            val steps = download().fetch(release()).toList()

            assertThat(steps.first()).isEqualTo(Download.Progress(0, content.size.toLong()))
            assertThat(apk.readBytes()).isEqualTo(content)
        }

    @Test
    fun aPartThatIsAlreadyWholeIsNotFetchedAgain() =
        runTest {
            part.writeBytes(content)

            val steps = download().fetch(release()).toList()

            assertThat(steps).containsExactly(Download.Ready(apk))
        }

    @Test
    fun aFinishedFileNeedsNoServer() =
        runTest {
            apk.writeBytes(content)
            server.stop(0)

            assertThat(download().fetch(release()).toList()).containsExactly(Download.Ready(apk))
        }

    @Test
    fun aFileThatDoesNotMatchIsDeleted() =
        runTest {
            val steps = download().fetch(release(sha256 = "00".repeat(32))).toList()

            assertThat(steps.last()).isEqualTo(Download.Corrupt)
            assertThat(directory.listFiles()).isEmpty()
        }

    @Test
    fun aChangedFinishedFileIsFetchedAgain() =
        runTest {
            apk.writeBytes(ByteArray(content.size))

            val steps = download().fetch(release()).toList()

            assertThat(steps.last()).isEqualTo(Download.Ready(apk))
            assertThat(apk.readBytes()).isEqualTo(content)
        }

    @Test
    fun filesOfOtherVersionsAreRemoved() =
        runTest {
            val old = File(directory, "otves-100.apk").apply { writeBytes(ByteArray(10)) }

            download().fetch(release()).toList()

            assertThat(old.exists()).isFalse()
        }

    @Test
    fun aFullPhoneIsToldBeforeFetching() =
        runTest {
            val steps = download(free = content.size.toLong()).fetch(release()).toList()

            assertThat(steps).containsExactly(Download.NoSpace)
            assertThat(ranges).isEmpty()
        }

    @Test
    fun aBrokenConnectionKeepsWhatCame() =
        runTest {
            // The server closes after two thirds of what it promised.
            served = content.copyOf(200_000)
            server.removeContext("/otves.apk")
            server.createContext("/otves.apk") { exchange ->
                exchange.sendResponseHeaders(200, content.size.toLong())
                exchange.responseBody.write(served)
                exchange.responseBody.flush()
                exchange.close()
            }

            assertThrows(IOException::class.java) {
                kotlinx.coroutines.runBlocking { download().fetch(release()).toList() }
            }
            assertThat(part.length()).isGreaterThan(0L)
            assertThat(apk.exists()).isFalse()
        }

    @Test
    fun aMissingFileOnTheServerFails() =
        runTest {
            val gone = release().copy(apkUrl = "http://127.0.0.1:${server.address.port}/nothing.apk")

            assertThrows(IOException::class.java) {
                kotlinx.coroutines.runBlocking { download().fetch(gone).toList() }
            }
        }
}
