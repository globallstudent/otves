package uz.otves.app

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.BackHandler
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.core.splashscreen.SplashScreen.Companion.installSplashScreen
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.engine.ContentAssets
import uz.otves.feature.editor.EditorRoute

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        installSplashScreen()
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        val engine = (application as OtvesApplication).engine
        setContent {
            OtvesTheme {
                // One job for now; the projects feature brings a real back stack.
                var editing by rememberSaveable { mutableStateOf(false) }
                if (editing) {
                    BackHandler { editing = false }
                    EditorRoute(
                        engine = engine,
                        loadContent = { withContext(Dispatchers.IO) { ContentAssets.load(assets) } },
                    )
                } else {
                    HomeScreen(onNewProject = { editing = true })
                }
            }
        }
    }
}
