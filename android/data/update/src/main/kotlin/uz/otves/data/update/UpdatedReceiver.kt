package uz.otves.data.update

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.widget.Toast

/**
 * Runs in the new version right after Android replaced the app.
 *
 * Android stops the app to replace it and does not start it again, so a line of text
 * tells the user the update is done; the downloaded file is no longer needed.
 */
internal class UpdatedReceiver : BroadcastReceiver() {
    override fun onReceive(
        context: Context,
        intent: Intent,
    ) {
        if (intent.action != Intent.ACTION_MY_PACKAGE_REPLACED) return
        AppUpdates.updateFiles(context).deleteRecursively()
        val version = context.packageManager.getPackageInfo(context.packageName, 0).versionName
        Toast.makeText(context, context.getString(R.string.update_done, version), Toast.LENGTH_LONG).show()
    }
}
