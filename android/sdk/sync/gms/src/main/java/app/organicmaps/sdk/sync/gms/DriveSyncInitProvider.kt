package app.organicmaps.sdk.sync.gms

import android.content.ContentProvider
import android.content.ContentValues
import android.database.Cursor
import android.net.Uri
import app.organicmaps.sdk.sync.CloudSyncRegistry
import com.google.android.gms.common.ConnectionResult
import com.google.android.gms.common.GoogleApiAvailability

/**
 * Registers Google Drive sync at process startup. Declared in this module's manifest, which is merged only into
 * the Google flavor, so other flavors have no provider and show no sync settings.
 */
class DriveSyncInitProvider : ContentProvider() {
    override fun onCreate(): Boolean {
        val context = context?.applicationContext ?: return true
        CloudSyncRegistry.register {
            val availability = GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(context)
            if (availability == ConnectionResult.SUCCESS) DriveSyncProvider(context) else null
        }
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
