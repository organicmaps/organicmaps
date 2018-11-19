package com.mapswithme.maps.metrics;

import android.support.annotation.NonNull;

import com.mapswithme.maps.discovery.DiscoveryUserEvent;
import com.mapswithme.maps.tips.TipsAction;
import com.mapswithme.maps.tips.TipsApi;

public class UserActionsLogger
{
  public static void logTipClickedEvent(@NonNull TipsApi provider, @NonNull TipsAction action)
  {
    nativeTipClicked(provider.ordinal(), action.ordinal());
  }

  public static void logBookingFilterUsedEvent()
  {
    nativeBookingFilterUsed();
  }

  public static void logDiscoveryShownEvent()
  {
    nativeDiscoveryShown();
  }

  public static void logBookmarksCatalogShownEvent()
  {
    nativeBookmarksCatalogShown();
  }

  public static void logDiscoveryItemClickedEvent(@NonNull DiscoveryUserEvent event)
  {
    nativeDiscoveryItemClicked(event.ordinal());
  }

  public static void logAddToBookmarkEvent()
  {
    nativeAddToBookmark();
  }

  public static void logUgcEditorOpened()
  {
    nativeUgcEditorOpened();
  }

  public static void logUgcSaved()
  {
    nativeUgcSaved();
  }

  private static native void nativeTipClicked(int type, int event);
  private static native void nativeBookingFilterUsed();
  private static native void nativeBookmarksCatalogShown();
  private static native void nativeDiscoveryShown();
  private static native void nativeDiscoveryItemClicked(int event);
  private static native void nativeAddToBookmark();
  private static native void nativeUgcEditorOpened();
  private static native void nativeUgcSaved();
}
