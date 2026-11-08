package uz.otves.feature.editor.api

import androidx.navigation3.runtime.NavKey
import kotlinx.serialization.Serializable

/** The editor of one job; [job] is its id as canonical UUID text. */
@Serializable
data class EditorKey(
    val job: String,
) : NavKey
