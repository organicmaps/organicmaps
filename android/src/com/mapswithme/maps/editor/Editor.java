package com.mapswithme.maps.editor;

import android.support.annotation.NonNull;
import android.support.annotation.Size;
import android.support.annotation.WorkerThread;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.background.AppBackgroundTracker;
import com.mapswithme.maps.background.WorkerService;
import com.mapswithme.maps.bookmarks.data.Metadata;
import com.mapswithme.maps.editor.data.FeatureCategory;


/**
 * Edits active(selected on the map) feature, which is represented as osm::EditableFeature in the core.
 */
public final class Editor
{
  private static final AppBackgroundTracker.OnTransitionListener sOsmUploader = new AppBackgroundTracker.OnTransitionListener()
  {
    @Override
    public void onTransit(boolean foreground)
    {
      if (!foreground)
        WorkerService.startActionUploadOsmChanges();
    }
  };

  private Editor() {}

  static
  {
    nativeInit();
  }

  private static native void nativeInit();

  public static void init()
  {
    MwmApplication.backgroundTracker().addListener(sOsmUploader);
  }

  @WorkerThread
  public static void uploadChanges()
  {
    if (nativeHasSomethingToUpload() && OsmOAuth.isAuthorized())
      nativeUploadChanges(OsmOAuth.getAuthToken(), OsmOAuth.getAuthSecret(), BuildConfig.VERSION_NAME,
                          BuildConfig.APPLICATION_ID);
  }

  public static native boolean nativeIsFeatureEditable();
  @NonNull
  public static native int[] nativeGetEditableFields();

  public static String getMetadata(Metadata.MetadataType type)
  {
    return nativeGetMetadata(type.toInt());
  }
  public static void setMetadata(Metadata.MetadataType type, String value)
  {
    nativeSetMetadata(type.toInt(), value);
  }
  private static native String nativeGetMetadata(int type);
  private static native void nativeSetMetadata(int type, String value);

  public static native boolean nativeIsAddressEditable();

  public static native boolean nativeIsNameEditable();

  @NonNull
  public static native String[] nativeGetNearbyStreets();

  // TODO(yunikkk): add get/set name in specific language.
  // To do that correctly, UI should query available languages, their translations and codes from
  // osm::EditableFeature. And pass these codes back in setter together with edited name.
  public static native String nativeGetDefaultName();
  public static native void nativeSetDefaultName(String name);

  public static native String nativeGetStreet();
  public static native void nativeSetStreet(String street);

  public static native String nativeGetHouseNumber();
  public static native void nativeSetHouseNumber(String houseNumber);

  // TODO(AlexZ): Support 3-state: Yes, No, Unknown.
  public static native boolean nativeHasWifi();

  public static native boolean nativeHasSomethingToUpload();
  @WorkerThread
  private static native void nativeUploadChanges(String token, String secret, String appVersion, String appId);

  /**
   * @return array [total edits count, uploaded edits count, last upload timestamp in seconds]
   */
  @Size(3)
  public static native long[] nativeGetStats();
  public static native void nativeClearLocalEdits();

  /**
   * That method should be called, when user opens editor from place page, so that information in editor
   * could refresh.
   */
  public static native void nativeStartEdit();
  /**
   * @return true if feature was saved. False if some error occurred (eg. no space)
   */
  public static native boolean nativeSaveEditedFeature();

  public static native FeatureCategory[] nativeGetNewFeatureCategories();

  /**
   * Creates new object on the map. Places it in the center of current viewport.
   * {@link Framework#nativeIsDownloadedMapAtScreenCenter()} should be called before
   * to check whether new feature can be created on the map.
   */
  public static void createMapObject(FeatureCategory category)
  {
    nativeCreateMapObject(category.category);
  }
  public static native void nativeCreateMapObject(int categoryId);

  /**
   * @return all cuisines keys.
   */
  public static native String[] nativeGetCuisines();

  /**
   * @return selected cuisines keys.
   */
  public static native String[] nativeGetSelectedCuisines();
  public static native String[] nativeTranslateCuisines(String[] keys);
  public static native void nativeSetSelectedCuisines(String [] keys);
  /**
   * @return properly formatted and appended cuisines string to display in UI.
   */
  public static native String nativeGetFormattedCuisine();

  public static native String nativeGetMwmName();
  public static native long nativeGetMwmVersion();

  public static native void nativeCreateNote(double lat, double lon, String text);
}
