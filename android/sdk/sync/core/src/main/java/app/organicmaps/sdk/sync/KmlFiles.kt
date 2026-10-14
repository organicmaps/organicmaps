package app.organicmaps.sdk.sync

import java.io.File

object KmlFiles {
    /** False for a category without bookmarks and tracks, like an untouched default list. */
    fun hasPlacemarks(file: File): Boolean = file.readText().contains("<Placemark")
}
