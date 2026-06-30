package app.organicmaps.sdk.location.gms;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.location.GmsLocationProviderRegistry;

/**
 * Registers the GMS location provider factory at process startup.
 *
 * <p>Declared in this module's manifest, which is merged into the app for every flavor that bundles a
 * GMS location module ({@code :sdk:location:gms:google} or {@code :sdk:location:gms:foss}). Runs
 * before {@code Application.onCreate()}, so the factory is registered before the first location
 * request -- no startup-ordering race. The provider is a manifest component, so R8 keeps it
 * automatically -- no @Keep required.
 */
public final class LocationProviderInitProvider extends ContentProvider
{
  @Override
  public boolean onCreate()
  {
    GmsLocationProviderRegistry.register(new GmsLocationProviderFactoryImpl());
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
