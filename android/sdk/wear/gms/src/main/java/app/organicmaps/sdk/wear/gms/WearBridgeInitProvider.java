package app.organicmaps.sdk.wear.gms;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.wear.WearBridge;

/**
 * Registers the Google Wear Data Layer publisher at process startup.
 *
 * <p>Declared in this module's manifest, which is merged into the app only for flavors that pull in
 * {@code :sdk:wear:gms} via {@code googleImplementation} -- so the publisher is installed on Google
 * builds and absent elsewhere. Runs before {@code Application.onCreate()}, so the publisher is
 * registered before that method's initial publish and before any later navigation-state change --
 * no startup-ordering race.
 */
public final class WearBridgeInitProvider extends ContentProvider
{
  @Override
  public boolean onCreate()
  {
    final Context context = getContext();
    if (context != null)
      WearBridge.register(new GmsWearNavigationPublisher(context));
    return true;
  }

  // This provider exists only for its onCreate() side effect; it serves no data.
  @Nullable
  @Override
  public Cursor query(@NonNull Uri uri, @Nullable String[] projection, @Nullable String selection,
                      @Nullable String[] selectionArgs, @Nullable String sortOrder)
  {
    return null;
  }

  @Nullable
  @Override
  public String getType(@NonNull Uri uri)
  {
    return null;
  }

  @Nullable
  @Override
  public Uri insert(@NonNull Uri uri, @Nullable ContentValues values)
  {
    return null;
  }

  @Override
  public int delete(@NonNull Uri uri, @Nullable String selection, @Nullable String[] selectionArgs)
  {
    return 0;
  }

  @Override
  public int update(@NonNull Uri uri, @Nullable ContentValues values, @Nullable String selection,
                    @Nullable String[] selectionArgs)
  {
    return 0;
  }
}
