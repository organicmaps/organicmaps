package app.organicmaps.sdk.editor;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.WorkerThread;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.Metadata;
import app.organicmaps.sdk.editor.data.FeatureCategory;
import app.organicmaps.sdk.editor.data.Language;
import app.organicmaps.sdk.editor.data.LocalizedName;
import app.organicmaps.sdk.editor.data.LocalizedStreet;
import app.organicmaps.sdk.editor.data.NamesDataSource;
import app.organicmaps.sdk.util.Config;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Edits active(selected on the map) feature, which is represented as osm::EditableFeature in the core.
 */
public final class Editor
{
  // Should correspond to core osm::FeatureStatus.
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({UNTOUCHED, DELETED, OBSOLETE, MODIFIED, CREATED})
  public @interface FeatureStatus
  {}

  public static final int UNTOUCHED = 0;
  public static final int DELETED = 1;
  public static final int OBSOLETE = 2;
  public static final int MODIFIED = 3;
  public static final int CREATED = 4;

  private Editor() {}

  static
  {
    nativeInit();
  }

  private static native void nativeInit();

  public static final int UPLOAD_FAILED_NOT_AUTHORIZED = -1;
  // Values below must match Editor::UploadResult in C++.
  public static final int UPLOAD_RESULT_SUCCESS = 0;
  public static final int UPLOAD_RESULT_ERROR = 1;
  public static final int UPLOAD_RESULT_NOTHING_TO_UPLOAD = 2;

  /**
   * Blocks the calling thread until the upload completes.
   * @return one of UPLOAD_RESULT_* constants, or -1 if not authorized.
   */
  @WorkerThread
  public static int uploadChanges()
  {
    if (OsmOAuth.isAuthorized())
      return nativeUploadChanges(OsmOAuth.getAuthToken(), Config.getVersionName(), Config.getApplicationId());
    return UPLOAD_FAILED_NOT_AUTHORIZED;
  }

  public static native boolean nativeShouldShowEditPlace();
  public static native boolean nativeShouldShowAddBusiness();
  public static native boolean nativeShouldShowAddPlace();
  public static native boolean nativeCanEditPlace();
  @NonNull
  public static native int[] nativeGetEditableProperties();

  public static native String nativeGetCategory();

  public static native String nativeGetMetadata(int id);
  public static native boolean nativeIsMetadataValid(int id, String value);
  public static native void nativeSetMetadata(int id, String value);
  public static native String nativeGetOpeningHours();
  public static native void nativeSetOpeningHours(String openingHours);
  public static String nativeGetPhone()
  {
    return nativeGetMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER.toInt());
  }
  public static void nativeSetPhone(String phone)
  {
    nativeSetMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER.toInt(), phone);
  }
  public static native int nativeGetStars();
  public static native int nativeGetMaxEditableBuildingLevels();
  public static String nativeGetBuildingLevels()
  {
    return nativeGetMetadata(Metadata.MetadataType.FMD_BUILDING_LEVELS.toInt());
  }
  public static void nativeSetBuildingLevels(String levels)
  {
    nativeSetMetadata(Metadata.MetadataType.FMD_BUILDING_LEVELS.toInt(), levels);
  }
  public static native boolean nativeHasWifi();
  public static native void nativeSetHasWifi(boolean hasWifi);

  public static void nativeSetSwitchInput(int id, Boolean switchValue, String checkedValue, String uncheckedValue)
  {
    nativeSetMetadata(id, switchValue ? checkedValue : uncheckedValue);
  }

  public static boolean nativeGetSwitchInput(int id, String checkedValue)
  {
    String value = nativeGetMetadata(id);
    return value.equals(checkedValue);
  }

  public static native boolean nativeIsAddressEditable();
  public static native boolean nativeIsNameEditable();
  public static native boolean nativeIsPointType();
  public static native boolean nativeIsBuilding();

  public static native NamesDataSource nativeGetNamesDataSource();
  public static native void nativeSetNames(@NonNull LocalizedName[] names);
  public static native LocalizedName nativeMakeLocalizedName(String langCode, String name);
  public static native Language[] nativeGetSupportedLanguages(boolean includeServiceLangs);
  public static native LocalizedStreet nativeGetStreet();
  public static native void nativeSetStreet(LocalizedStreet street);
  @NonNull
  public static native LocalizedStreet[] nativeGetNearbyStreets();

  public static native String nativeGetHouseNumber();
  public static native void nativeSetHouseNumber(String houseNumber);
  public static native boolean nativeIsHouseValid(String houseNumber);
  public static boolean nativeIsLevelValid(String level)
  {
    return nativeIsMetadataValid(Metadata.MetadataType.FMD_BUILDING_LEVELS.toInt(), level);
  }
  public static boolean nativeIsPhoneValid(String phone)
  {
    return nativeIsMetadataValid(Metadata.MetadataType.FMD_PHONE_NUMBER.toInt(), phone);
  }
  public static native boolean nativeIsNameValid(String name);

  public static native boolean nativeHasSomethingToUpload();
  @WorkerThread
  private static native int nativeUploadChanges(String oauthToken, String appVersion, String appId);

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

  @NonNull
  public static native String[] nativeGetAllCreatableFeatureTypes(@NonNull String lang);
  @NonNull
  public static native String[] nativeSearchCreatableFeatureTypes(@NonNull String query, @NonNull String lang);
  @NonNull
  public static native String[] nativeGetRecentCategories();
  public static native void nativeAddToRecentCategories(@NonNull String type);

  /**
   * Creates new object on the map. Places it in the center of current viewport.
   * {@link Framework#nativeIsDownloadedMapAtScreenCenter()} should be called before
   * to check whether new feature can be created on the map.
   */
  public static void createMapObject(FeatureCategory category)
  {
    nativeCreateMapObject(category.getType());
  }
  public static native void nativeCreateMapObject(@NonNull String type);
  public static native void nativeCreateNote(String text);
  public static native void nativePlaceDoesNotExist(@NonNull String comment);
  public static native void nativeRollbackMapObject();
  public static native void nativeCreateStandaloneNote(double lat, double lon, String text);

  /**
   * @return all cuisines keys.
   */
  public static native String[] nativeGetCuisines();

  /**
   * @return selected cuisines keys.
   */
  public static native String[] nativeGetSelectedCuisines();
  public static native String[] nativeFilterCuisinesKeys(String substr);
  public static native String[] nativeTranslateCuisines(String[] keys);
  public static native void nativeSetSelectedCuisines(String[] keys);
  /**
   * @return properly formatted and appended cuisines string to display in UI.
   */
  public static native String nativeGetFormattedCuisine();

  public static native String nativeGetMwmName();
  public static native long nativeGetMwmVersion();

  @FeatureStatus
  public static native int nativeGetMapObjectStatus();
  public static native boolean nativeIsMapObjectUploaded();
}
