package com.mapswithme.maps.editor;

import android.support.annotation.NonNull;
import android.support.annotation.WorkerThread;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.background.AppBackgroundTracker;
import com.mapswithme.maps.background.WorkerService;
import com.mapswithme.maps.bookmarks.data.Metadata;


/**
 * Edits active(selected on the map) MapObjects(aka UserMark in core).
 * All the methods apply to currently active objects.
 */
public final class Editor
{
  private static AppBackgroundTracker.OnTransitionListener sOsmUploader = new AppBackgroundTracker.OnTransitionListener()
  {
    @Override
    public void onTransit(boolean foreground)
    {
      if (!foreground)
        WorkerService.startActionUploadOsmChanges();
    }
  };

  private Editor() {}

  public static void init()
  {
    MwmApplication.backgroundTracker().addListener(sOsmUploader);
  }

  public static boolean hasEditableAttributes()
  {
    return Editor.nativeGetEditableMetadata().length != 0 ||
           Editor.nativeIsAddressEditable() ||
           Editor.nativeIsNameEditable();
  }

  @WorkerThread
  public static void uploadChanges()
  {
    if (nativeHasSomethingToUpload() &&
        OsmOAuth.isAuthorized())
      nativeUploadChanges(OsmOAuth.getAuthToken(), OsmOAuth.getAuthSecret());
  }

  @NonNull
  public static native int[] nativeGetEditableMetadata();

  public static void setMetadata(Metadata.MetadataType type, String value)
  {
    nativeSetMetadata(type.toInt(), value);
  }

  public static native void nativeSetMetadata(int type, String value);

  public static native void nativeEditFeature(String street, String houseNumber);

  public static native boolean nativeIsAddressEditable();

  public static native boolean nativeIsNameEditable();

  @NonNull
  public static native String[] nativeGetNearbyStreets();

  public static native void nativeSetName(String name);

  public static native boolean nativeHasSomethingToUpload();

  @WorkerThread
  private static native void nativeUploadChanges(String token, String secret);

  /**
   * @return array [total edits count, uploaded edits count, last upload timestamp]
   */
  public static native long[] nativeGetStats();

  public static native void nativeClearLocalEdits();
}
