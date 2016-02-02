package com.mapswithme.maps.downloader;

import android.support.annotation.Nullable;

import java.util.List;

public final class MapManager
{
  public interface StorageCallback
  {
    void onStatusChanged(String countryId);
    void onProgress(String countryId, long localSize, long remoteSize);
  }

  private MapManager() {}


  // Determines whether the legacy (large MWMs) mode is used.
  public static native boolean nativeIsLegacyMode();

  // Returns info about updatable data. Returns null on error.
  public static native @Nullable UpdateInfo nativeGetUpdateInfo();

  // Retrieves list of country items with its status info. Use root as parent if parent is null.
  public static native void nativeListItems(@Nullable String root, List<CountryItem> result);

  // Enqueue country in downloader.
  public static native boolean nativeStartDownload(String countryId);

  // Remove downloading country from downloader.
  public static native boolean nativeCancelDownload(String countryId);

  // Registers callback about storage status changed. Returns slot ID which is should be used to unsubscribe.
  public static native int nativeSubscribe(StorageCallback listener);

  // Unregisters storage status changed callback.
  public static native void nativeUnsubscribe(int slot);
}