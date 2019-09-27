package com.mapswithme.util.statistics;

import android.graphics.Rect;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.ads.MwmNativeAd;
import com.mapswithme.maps.base.Savable;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.taxi.TaxiType;
import com.mapswithme.maps.widget.placepage.PlacePageView;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.util.UiUtils;

import java.util.List;

import static com.mapswithme.util.statistics.Statistics.EventName.PP_BANNER_SHOW;
import static com.mapswithme.util.statistics.Statistics.PP_BANNER_STATE_DETAILS;
import static com.mapswithme.util.statistics.Statistics.PP_BANNER_STATE_PREVIEW;

public class PlacePageTracker implements Savable<Bundle>
{
  private static final float VISIBILITY_RATIO_TAXI = 0.6f;
  private static final String EXTRA_TAXI_TRACKED = "extra_taxi_tracked";
  private static final String EXTRA_SPONSORED_TRACKED = "extra_sponsored_tracked";
  private static final String EXTRA_BANNER_DETAILS_TRACKED = "extra_banner_details_tracked";
  private static final String EXTRA_BANNER_PREVIEW_TRACKED = "extra_banner_preview_tracked";
  private static final String EXTRA_PP_DETAILS_OPENED_TRACKED = "extra_pp_details_opened_tracked";
  @NonNull
  private final PlacePageView mPlacePageView;
  @NonNull
  private final View mBottomButtons;
  @NonNull
  private final View mTaxi;
  @Nullable
  private MapObject mMapObject;
  private boolean mTaxiTracked;
  private boolean mSponsoredTracked;
  private boolean mBannerDetailsTracked;
  private boolean mBannerPreviewTracked;
  private boolean mPpDetailsOpenedTracked;

  public PlacePageTracker(@NonNull PlacePageView placePageView, @NonNull View bottomButtons)
  {
    mPlacePageView = placePageView;
    mBottomButtons = bottomButtons;
    mTaxi = mPlacePageView.findViewById(R.id.ll__place_page_taxi);
  }

  public void setMapObject(@Nullable MapObject mapObject)
  {
    mMapObject = mapObject;
  }

  public void onMove()
  {
    trackTaxiVisibility();
  }

  public void onHidden()
  {
    mMapObject = null;
    mTaxiTracked = false;
    mSponsoredTracked = false;
    mBannerDetailsTracked = false;
    mBannerPreviewTracked = false;
    mPpDetailsOpenedTracked = false;
  }

  public void onBannerDetails(@Nullable MwmNativeAd ad)
  {
    if (mBannerDetailsTracked)
      return;

    Statistics.INSTANCE.trackPPBanner(PP_BANNER_SHOW, ad, PP_BANNER_STATE_DETAILS);
    mBannerDetailsTracked = true;
  }

  public void onBannerPreview(@Nullable MwmNativeAd ad)
  {
    if (mBannerPreviewTracked)
      return;

    Statistics.INSTANCE.trackPPBanner(PP_BANNER_SHOW, ad, PP_BANNER_STATE_PREVIEW);
    mBannerPreviewTracked = true;
  }

  public void onDetails()
  {
    if (!mSponsoredTracked)
    {
      Sponsored sponsored = mPlacePageView.getSponsored();
      if (sponsored != null)
      {
        Statistics.INSTANCE.trackSponsoredOpenEvent(sponsored);
        mSponsoredTracked = true;
      }
    }

    if (!mPpDetailsOpenedTracked)
    {
      Statistics.INSTANCE.trackEvent(Statistics.EventName.PP_DETAILS_OPEN);
      mPpDetailsOpenedTracked = true;
    }
  }

  private void trackTaxiVisibility()
  {
    if (!mTaxiTracked && isViewOnScreen(mTaxi, VISIBILITY_RATIO_TAXI) && mMapObject != null)
    {
      List<TaxiType> taxiTypes = mMapObject.getReachableByTaxiTypes();
      if (taxiTypes != null && !taxiTypes.isEmpty())
      {
        String providerName = taxiTypes.get(0).getProviderName();
        Statistics.INSTANCE.trackTaxiEvent(Statistics.EventName.ROUTING_TAXI_SHOW,
                                           providerName);
        mTaxiTracked = true;
      }
    }
  }

  /**
   *
   * @param visibilityRatio Describes what the portion of view should be visible before
   *                        the view is considered visible on the screen. It can be from 0 to 1.
   */
  @SuppressWarnings("SameParameterValue")
  private boolean isViewOnScreen(@NonNull View view, float visibilityRatio) {

    if (UiUtils.isInvisible(mPlacePageView))
      return false;

    Rect localRect = new Rect();
    boolean isVisible = view.getGlobalVisibleRect(localRect);
    if (isVisible)
    {
      int visibleHeight = localRect.height() - (localRect.bottom  - mBottomButtons.getTop());
      if ((float)visibleHeight / view.getHeight() >= visibilityRatio)
        return true;
    }
    return false;
  }

  @Override
  public void onSave(@NonNull Bundle outState)
  {
    outState.putBoolean(EXTRA_SPONSORED_TRACKED, mSponsoredTracked);
    outState.putBoolean(EXTRA_TAXI_TRACKED, mTaxiTracked);
    outState.putBoolean(EXTRA_BANNER_DETAILS_TRACKED, mBannerDetailsTracked);
    outState.putBoolean(EXTRA_BANNER_PREVIEW_TRACKED, mBannerPreviewTracked);
    outState.putBoolean(EXTRA_PP_DETAILS_OPENED_TRACKED, mPpDetailsOpenedTracked);
  }

  @Override
  public void onRestore(@NonNull Bundle inState)
  {
    mSponsoredTracked = inState.getBoolean(EXTRA_SPONSORED_TRACKED);
    mTaxiTracked = inState.getBoolean(EXTRA_TAXI_TRACKED);
    mBannerDetailsTracked = inState.getBoolean(EXTRA_BANNER_DETAILS_TRACKED);
    mBannerPreviewTracked = inState.getBoolean(EXTRA_BANNER_PREVIEW_TRACKED);
    mPpDetailsOpenedTracked = inState.getBoolean(EXTRA_PP_DETAILS_OPENED_TRACKED);
  }
}
