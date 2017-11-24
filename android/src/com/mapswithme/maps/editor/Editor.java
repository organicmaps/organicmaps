package com.mapswithme.maps.editor;

import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Size;
import android.support.annotation.WorkerThread;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.background.AppBackgroundTracker;
import com.mapswithme.maps.background.WorkerService;
import com.mapswithme.maps.editor.data.FeatureCategory;
import com.mapswithme.maps.editor.data.Language;
import com.mapswithme.maps.editor.data.LocalizedName;
import com.mapswithme.maps.editor.data.LocalizedStreet;
import com.mapswithme.maps.editor.data.NamesDataSource;


/**
 * Edits active(selected on the map) feature, which is represented as osm::EditableFeature in the core.
 */
public final class Editor
{
  // Should correspond to core osm::FeatureStatus.
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({UNTOUCHED, DELETED, OBSOLETE, MODIFIED, CREATED})
  public @interface FeatureStatus {}

  public static final int UNTOUCHED = 0;
  public static final int DELETED = 1;
  public static final int OBSOLETE = 2;
  public static final int MODIFIED = 3;
  public static final int CREATED = 4;

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

  public static native boolean nativeShouldShowEditPlace();
  public static native boolean nativeShouldShowAddPlace();
  public static native boolean nativeShouldShowAddBusiness();
  @NonNull
  public static native int[] nativeGetEditableFields();

  public static native String nativeGetCategory();
  public static native String nativeGetOpeningHours();
  public static native void nativeSetOpeningHours(String openingHours);
  public static native String nativeGetPhone();
  public static native void nativeSetPhone(String phone);
  public static native String nativeGetWebsite();
  public static native void nativeSetWebsite(String website);
  public static native String nativeGetEmail();
  public static native void nativeSetEmail(String email);
  public static native int nativeGetStars();
  public static native void nativeSetStars(String stars);
  public static native String nativeGetOperator();
  public static native void nativeSetOperator(String operator);
  public static native String nativeGetWikipedia();
  public static native void nativeSetWikipedia(String wikipedia);
  public static native String nativeGetFlats();
  public static native void nativeSetFlats(String flats);
  public static native String nativeGetBuildingLevels();
  public static native void nativeSetBuildingLevels(String levels);
  public static native String nativeGetZipCode();
  public static native void nativeSetZipCode(String zipCode);
  public static native boolean nativeHasWifi();
  public static native boolean nativeSetHasWifi(boolean hasWifi);

  public static native boolean nativeIsAddressEditable();
  public static native boolean nativeIsNameEditable();
  public static native boolean nativeIsPointType();
  public static native boolean nativeIsBuilding();

  public static native NamesDataSource nativeGetNamesDataSource(boolean needFakes);
  public static native String nativeGetDefaultName();
  public static native void nativeEnableNamesAdvancedMode();
  public static native void nativeSetNames(@NonNull LocalizedName[] names);
  public static native LocalizedName nativeMakeLocalizedName(String langCode, String name);
  public static native Language[] nativeGetSupportedLanguages();

  public static native LocalizedStreet nativeGetStreet();
  public static native void nativeSetStreet(LocalizedStreet street);
  @NonNull
  public static native LocalizedStreet[] nativeGetNearbyStreets();

  public static native String nativeGetHouseNumber();
  public static native void nativeSetHouseNumber(String houseNumber);
  public static native boolean nativeIsHouseValid(String houseNumber);
  public static native boolean nativeIsLevelValid(String level);
  public static native boolean nativeIsFlatValid(String flat);
  public static native boolean nativeIsZipcodeValid(String zipCode);
  public static native boolean nativeIsPhoneValid(String phone);
  public static native boolean nativeIsWebsiteValid(String site);
  public static native boolean nativeIsEmailValid(String email);
  public static native boolean nativeIsNameValid(String name);


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

  @NonNull
  public static native FeatureCategory[] nativeGetAllFeatureCategories(String lang);
  @NonNull
  public static native FeatureCategory[] nativeSearchFeatureCategories(String query, String lang);

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
  public static native void nativeCreateNote(String text);
  public static native void nativePlaceDoesNotExist(String comment);
  public static native void nativeRollbackMapObject();

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

  @FeatureStatus
  public static native int nativeGetMapObjectStatus();
  public static native boolean nativeIsMapObjectUploaded();
}
