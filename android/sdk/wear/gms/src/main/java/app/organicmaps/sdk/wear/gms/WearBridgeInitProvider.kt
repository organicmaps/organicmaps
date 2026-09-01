package app.organicmaps.sdk.wear.gms

import android.content.ContentProvider
import android.content.ContentValues
import android.database.Cursor
import android.net.Uri
import app.organicmaps.sdk.wear.WearBridge

/**
 * Registers the Google Wear Data Layer publisher at process startup.
 *
 * Declared in this module's manifest, which is merged into the app only for the variants that pull
 * in `:sdk:wear:gms` -- Google debug and beta -- so the publisher is absent everywhere else,
 * including Google release. Runs before `Application.onCreate()`, so the publisher is registered
 * before that method's initial publish and before any later navigation-state change -- no
 * startup-ordering race.
 */
class WearBridgeInitProvider : ContentProvider() {
    override fun onCreate(): Boolean {
        context?.let { WearBridge.register(GmsWearNavigationPublisher(it)) }
        return true
    }

    // This provider exists only for its onCreate() side effect; it serves no data.
    override fun query(
        uri: Uri,
        projection: Array<out String>?,
        selection: String?,
        selectionArgs: Array<out String>?,
        sortOrder: String?,
    ): Cursor? = null

    override fun getType(uri: Uri): String? = null

    override fun insert(uri: Uri, values: ContentValues?): Uri? = null

    override fun delete(uri: Uri, selection: String?, selectionArgs: Array<out String>?): Int = 0

    override fun update(uri: Uri, values: ContentValues?, selection: String?, selectionArgs: Array<out String>?): Int =
        0
}
