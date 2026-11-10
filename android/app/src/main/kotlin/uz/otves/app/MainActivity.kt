package uz.otves.app

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.core.splashscreen.SplashScreen.Companion.installSplashScreen
import androidx.lifecycle.viewmodel.navigation3.rememberViewModelStoreNavEntryDecorator
import androidx.navigation3.runtime.entryProvider
import androidx.navigation3.runtime.rememberNavBackStack
import androidx.navigation3.runtime.rememberSaveableStateHolderNavEntryDecorator
import androidx.navigation3.ui.NavDisplay
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.engine.ContentAssets
import uz.otves.feature.editor.EditorRoute
import uz.otves.feature.editor.api.EditorKey
import uz.otves.feature.projects.ProjectsRoute
import uz.otves.feature.projects.api.ProjectsKey
import kotlin.uuid.Uuid

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        installSplashScreen()
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        val app = application as OtvesApplication
        setContent {
            OtvesTheme {
                // Restored after process death; each entry keeps its view models until it leaves.
                val backStack = rememberNavBackStack(ProjectsKey)
                NavDisplay(
                    backStack = backStack,
                    onBack = { backStack.removeLastOrNull() },
                    entryDecorators =
                        listOf(
                            rememberSaveableStateHolderNavEntryDecorator(),
                            rememberViewModelStoreNavEntryDecorator(),
                        ),
                    entryProvider =
                        entryProvider {
                            entry<ProjectsKey> {
                                ProjectsRoute(jobs = app.jobs, onOpen = { job -> backStack.add(EditorKey(job.toString())) })
                            }
                            entry<EditorKey> { key ->
                                EditorRoute(
                                    engine = app.engine,
                                    loadContent = { withContext(Dispatchers.IO) { ContentAssets.load(assets) } },
                                    loadCatalog = { withContext(Dispatchers.IO) { ContentAssets.catalog(assets) } },
                                    openJob = { app.jobs.open(Uuid.parse(key.job)) },
                                    onBack = { backStack.removeLastOrNull() },
                                )
                            }
                        },
                )
            }
        }
    }
}
