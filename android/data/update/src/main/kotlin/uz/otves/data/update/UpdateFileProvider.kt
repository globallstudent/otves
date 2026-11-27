package uz.otves.data.update

import androidx.core.content.FileProvider

// Its own class, so its manifest entry does not merge with another module's FileProvider.
internal class UpdateFileProvider : FileProvider()
