@file:JvmName("ExportMenuItems")

package app.organicmaps.util.bottomsheet

import app.organicmaps.R
import app.organicmaps.sdk.bookmarks.data.FileType
import java.util.function.Consumer

fun create(onSelected: Consumer<FileType>): ArrayList<MenuBottomSheetItem> = arrayListOf(
    MenuBottomSheetItem(R.string.export_file, R.drawable.ic_file_kmz) { onSelected.accept(FileType.Kml) },
    MenuBottomSheetItem(R.string.export_file_gpx, R.drawable.ic_file_gpx) { onSelected.accept(FileType.Gpx) },
    MenuBottomSheetItem(R.string.export_file_geojson, R.drawable.ic_file_geojson) {
        onSelected.accept(FileType.GeoJson)
    },
)
