package uz.otves.data.update

import com.google.common.truth.Truth.assertThat
import kotlinx.serialization.SerializationException
import org.junit.Assert.assertThrows
import org.junit.Test

class ManifestTest {
    private val text =
        """
        {
          "schema": 1,
          "versionCode": 101,
          "versionName": "0.1.1",
          "apk": {
            "url": "https://example.org/otves-0.1.1.apk",
            "sha256": "AB12",
            "size": 9634512
          },
          "notes": {
            "ru": [{ "icon": "sheet", "title": "Листы в стандартном масштабе", "text": "План сразу ложится на лист" }],
            "uz-Latn": [{ "title": "Varaqlar standart masshtabda" }]
          },
          "addedLater": true
        }
        """.trimIndent()

    @Test
    fun aManifestBecomesARelease() {
        val release = Manifest.parse(text).toRelease()

        assertThat(release.versionCode).isEqualTo(101)
        assertThat(release.versionName).isEqualTo("0.1.1")
        assertThat(release.apkUrl).isEqualTo("https://example.org/otves-0.1.1.apk")
        assertThat(release.apkSha256).isEqualTo("ab12")
        assertThat(release.apkSize).isEqualTo(9_634_512)
        assertThat(release.notesIn("ru"))
            .containsExactly(ReleaseNote("sheet", "Листы в стандартном масштабе", "План сразу ложится на лист"))
    }

    @Test
    fun aNoteNeedsOnlyItsTitle() {
        val release = Manifest.parse(text).toRelease()

        assertThat(release.notesIn("uz-Latn")).containsExactly(ReleaseNote("", "Varaqlar standart masshtabda", ""))
    }

    @Test
    fun aLanguageWithoutNotesGetsTheRussianOnes() {
        val release = Manifest.parse(text).toRelease()

        assertThat(release.notesIn("uz-Cyrl")).isEqualTo(release.notesIn("ru"))
    }

    @Test
    fun aReleaseWithoutNotesHasNone() {
        val bare = """{"versionCode": 101, "versionName": "0.1.1", "apk": {"url": "u", "sha256": "ab", "size": 1}}"""

        assertThat(Manifest.parse(bare).toRelease().notesIn("ru")).isEmpty()
    }

    @Test
    fun textThatIsNotAManifestIsRefused() {
        assertThrows(SerializationException::class.java) { Manifest.parse("<html>Not Found</html>") }
        assertThrows(SerializationException::class.java) { Manifest.parse("""{"versionCode": 101}""") }
    }
}
