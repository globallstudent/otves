package uz.otves.app

import android.graphics.Color
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.SystemBarStyle
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.core.splashscreen.SplashScreen
import androidx.core.splashscreen.SplashScreen.Companion.installSplashScreen
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import androidx.lifecycle.viewmodel.navigation3.rememberViewModelStoreNavEntryDecorator
import androidx.navigation3.runtime.entryProvider
import androidx.navigation3.runtime.rememberNavBackStack
import androidx.navigation3.runtime.rememberSaveableStateHolderNavEntryDecorator
import androidx.navigation3.ui.NavDisplay
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeoutOrNull
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.data.update.Updates
import uz.otves.engine.ContentAssets
import uz.otves.feature.editor.EditorRoute
import uz.otves.feature.editor.api.EditorKey
import uz.otves.feature.projects.ProjectsRoute
import uz.otves.feature.projects.api.ProjectsKey
import uz.otves.feature.update.UpdateRoute
import kotlin.uuid.Uuid

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        val splash = installSplashScreen()
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        val app = application as OtvesApplication
        checkForUpdates(app.updates, splash)
        setContent {
            OtvesTheme {
                // Restored after process death; each entry keeps its view models until it leaves.
                val backStack = rememberNavBackStack(ProjectsKey)
                val release by app.updates.required.collectAsStateWithLifecycle()
                // An update never takes a job away from under the user: it waits for the jobs list.
                val update = release?.takeIf { backStack.lastOrNull() == ProjectsKey }
                DisposableEffect(update != null) {
                    // The update screen is dark at the top, the rest of the app is light.
                    val bars =
                        if (update != null) {
                            SystemBarStyle.dark(Color.TRANSPARENT)
                        } else {
                            SystemBarStyle.light(Color.TRANSPARENT, Color.TRANSPARENT)
                        }
                    enableEdgeToEdge(statusBarStyle = bars)
                    onDispose {}
                }
                if (update != null) {
                    UpdateRoute(updates = app.updates, release = update)
                    return@OtvesTheme
                }
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
                                    prices = app.prices,
                                    onBack = { backStack.removeLastOrNull() },
                                )
                            }
                        },
                )
            }
        }
    }

    // Asks the release feed each time the app comes to the front; the feed is asked again only when a check is due.
    private fun checkForUpdates(
        updates: Updates,
        splash: SplashScreen,
    ) {
        val firstAnswer = CompletableDeferred<Unit>()
        lifecycleScope.launch {
            repeatOnLifecycle(Lifecycle.State.STARTED) {
                try {
                    updates.refresh()
                } finally {
                    firstAnswer.complete(Unit)
                }
            }
        }
        if (!updates.expected) return
        // A newer version was seen before: the splash stays until the feed confirms it,
        // so the jobs list does not flash before the update screen. Without a network the app opens.
        var waiting = true
        splash.setKeepOnScreenCondition { waiting }
        lifecycleScope.launch {
            withTimeoutOrNull(SPLASH_WAIT_MS) { firstAnswer.await() }
            waiting = false
        }
    }

    private companion object {
        const val SPLASH_WAIT_MS = 3_000L
    }
}
