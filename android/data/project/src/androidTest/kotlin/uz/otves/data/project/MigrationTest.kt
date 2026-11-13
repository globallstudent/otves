package uz.otves.data.project

import androidx.room.testing.MigrationTestHelper
import androidx.sqlite.driver.bundled.BundledSQLiteDriver
import androidx.sqlite.execSQL
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.google.common.truth.Truth.assertThat
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class MigrationTest {
    private val instrumentation = InstrumentationRegistry.getInstrumentation()
    private val file = instrumentation.targetContext.getDatabasePath("migration-test.job")

    @get:Rule
    val helper = MigrationTestHelper(instrumentation, file, BundledSQLiteDriver(), ProjectDatabase::class)

    // docs/adr/0008: a version 1 job keeps its walls and starts with nothing chosen.
    @Test
    fun version1JobsOpenWithNoMaterialAndNoReinforcement() {
        helper.createDatabase(1).use { connection ->
            connection.execSQL("INSERT INTO level (id, elevation, height) VALUES ('level', 0, 3000)")
            connection.execSQL(
                "INSERT INTO wall (id, level_id, start_x, start_y, end_x, end_y, thickness, height, location_line) " +
                    "VALUES ('wall', 'level', 0, 0, 6400, 0, 380, 2800, 0)",
            )
        }

        helper.runMigrationsAndValidate(5).use { connection ->
            // docs/adr/0010: no footing until the builder gives one.
            connection.prepare("SELECT footing_width, footing_depth FROM level").use { level ->
                assertThat(level.step()).isTrue()
                assertThat((0..1).map(level::getLong)).containsExactly(0L, 0L).inOrder()
            }
            // docs/adr/0009: no conditions yet, so the norms' defaults apply.
            connection.prepare("SELECT COUNT(*) FROM conditions").use { conditions ->
                assertThat(conditions.step()).isTrue()
                assertThat(conditions.getLong(0)).isEqualTo(0L)
            }
            // docs/adr/0012: a wall drawn before sketches has a length that was given.
            connection.prepare("SELECT thickness, unit_length, unit_width, unit_height, exact FROM wall").use { wall ->
                assertThat(wall.step()).isTrue()
                assertThat((0..4).map(wall::getLong)).containsExactly(380L, 0L, 0L, 0L, 1L).inOrder()
            }
            connection.prepare("SELECT ring_beam, junction_meshes FROM level").use { level ->
                assertThat(level.step()).isTrue()
                assertThat((0..1).map(level::getLong)).containsExactly(0L, 0L).inOrder()
            }
        }
    }
}
