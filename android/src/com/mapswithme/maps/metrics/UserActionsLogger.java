package com.mapswithme.maps.metrics;

import androidx.annotation.NonNull;

import com.mapswithme.maps.discovery.DiscoveryUserEvent;
import com.mapswithme.maps.tips.TutorialAction;
import com.mapswithme.maps.tips.Tutorial;

public class UserActionsLogger
{
  public static void logTipClickedEvent(@NonNull Tutorial tutorial, @NonNull TutorialAction action)
  {
    nativeTipClicked(tutorial.ordinal(), action.ordinal());
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

  public static void logBookingBookClicked()
  {
    nativeBookingBookClicked();
  }
  public static void logBookingMoreClicked()
  {
    nativeBookingMoreClicked();
  }
  public static void logBookingReviewsClicked()
  {
    nativeBookingReviewsClicked();
  }
  public static void logBookingDetailsClicked()
  {
    nativeBookingDetailsClicked();
  }

  public static void logPromoAfterBookingShown(@NonNull String id)
  {
    nativePromoAfterBookingShown(id);
  }

  private static native void nativeTipClicked(int type, int event);
  private static native void nativeBookingFilterUsed();
  private static native void nativeBookmarksCatalogShown();
  private static native void nativeDiscoveryShown();
  private static native void nativeDiscoveryItemClicked(int event);
  private static native void nativeAddToBookmark();
  private static native void nativeUgcEditorOpened();
  private static native void nativeUgcSaved();
  private static native void nativeBookingBookClicked();
  private static native void nativeBookingMoreClicked();
  private static native void nativeBookingReviewsClicked();
  private static native void nativeBookingDetailsClicked();
  private static native void nativePromoAfterBookingShown(@NonNull String id);
}
