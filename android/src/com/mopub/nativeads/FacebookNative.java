package com.mopub.nativeads;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;

import com.facebook.ads.Ad;
import com.facebook.ads.AdError;
import com.facebook.ads.AdListener;
import com.facebook.ads.MediaView;
import com.facebook.ads.NativeAd;
import com.facebook.ads.NativeAd.Rating;
import com.mopub.common.Preconditions;
import com.mopub.common.logging.MoPubLog;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static com.mopub.nativeads.NativeImageHelper.preCacheImages;

/**
 * FacebookAdRenderer is also necessary in order to show video ads.
 * Video ads will only be shown if VIDEO_ENABLED is set to true or a server configuration
 * "video_enabled" flag is set to true. The server configuration will override the local
 * configuration.
 * Please reference the Supported Mediation Partner page at http://bit.ly/2mqsuFH for the
 * latest version and ad format certifications.
 */
public class FacebookNative extends CustomEventNative {
  private static final String PLACEMENT_ID_KEY = "placement_id";
  private static final String VIDEO_ENABLED_KEY = "video_enabled";

  /**
   * Sets whether or not Facebook native video ads will be shown. This value is overridden with
   * server extras.
   */
  private static boolean VIDEO_ENABLED = false;

  /**
   * Sets whether or not there is a video renderer available. This class will check for the
   * default Facebook video renderer. This value can be overridden with {@link
   * FacebookNative#setVideoRendererAvailable} if there already is a custom Facebook video
   * renderer.
   */
  private static Boolean sIsVideoRendererAvailable = null;

  // CustomEventNative implementation
  @Override
  protected void loadNativeAd(final Context context,
                              final CustomEventNativeListener customEventNativeListener,
                              final Map<String, Object> localExtras,
                              final Map<String, String> serverExtras) {

    final String placementId;
    if (extrasAreValid(serverExtras)) {
      placementId = serverExtras.get(PLACEMENT_ID_KEY);
    } else {
      customEventNativeListener.onNativeAdFailed(NativeErrorCode.NATIVE_ADAPTER_CONFIGURATION_ERROR);
      return;
    }

    final String videoEnabledString = serverExtras.get(VIDEO_ENABLED_KEY);
    boolean videoEnabledFromServer = Boolean.parseBoolean(videoEnabledString);

    if (sIsVideoRendererAvailable == null) {
      try {
        Class.forName("com.mopub.nativeads.FacebookAdRenderer");
        sIsVideoRendererAvailable = true;
      } catch (ClassNotFoundException e) {
        sIsVideoRendererAvailable = false;
      }
    }

    if (shouldUseVideoEnabledNativeAd(sIsVideoRendererAvailable, videoEnabledString,
                                      videoEnabledFromServer)) {
      final FacebookVideoEnabledNativeAd facebookVideoEnabledNativeAd =
          new FacebookVideoEnabledNativeAd(context,
                                           new NativeAd(context, placementId), customEventNativeListener);
      facebookVideoEnabledNativeAd.loadAd();
    } else {
      final FacebookStaticNativeAd facebookStaticNativeAd = new FacebookStaticNativeAd(
          context, new NativeAd(context, placementId), customEventNativeListener);
      facebookStaticNativeAd.loadAd();
    }
  }

  /**
   * Sets whether Facebook native video ads may be shown. This value is overridden by the value of
   * the "video_enabled" key that may be sent from the MoPub ad server.
   * com.mopub.nativeads.FacebookAdRenderer must also be used to display video-enabled ads.
   *
   * @param videoEnabled True if you want to enable Facebook native video.
   */
  public static void setVideoEnabled(final boolean videoEnabled) {
    VIDEO_ENABLED = videoEnabled;
  }

  /**
   * Sets whether a renderer is available that supports Facebook video ads.
   * <p/>
   * If you use a custom renderer class that is not com.mopub.nativeads.FacebookAdRenderer to show
   * video-enabled native ads, you should set this to true.
   *
   * @param videoRendererAvailable Whether or not there is a renderer available for video-enabled
   *                               Facebook native ads.
   */
  public static void setVideoRendererAvailable(final boolean videoRendererAvailable) {
    sIsVideoRendererAvailable = videoRendererAvailable;
  }

  static boolean shouldUseVideoEnabledNativeAd(final boolean isVideoRendererAvailable,
                                               final String videoEnabledString, final boolean videoEnabledFromServer) {
    if (!isVideoRendererAvailable) {
      return false;
    }
    return (videoEnabledString != null && videoEnabledFromServer) ||
           (videoEnabledString == null && VIDEO_ENABLED);
  }

  static Boolean isVideoRendererAvailable() {
    return sIsVideoRendererAvailable;
  }

  private boolean extrasAreValid(final Map<String, String> serverExtras) {
    final String placementId = serverExtras.get(PLACEMENT_ID_KEY);
    return (placementId != null && placementId.length() > 0);
  }

  private static void registerChildViewsForInteraction(final View view, final NativeAd nativeAd) {
    if (nativeAd == null) {
      return;
    }

    if (view instanceof ViewGroup && ((ViewGroup) view).getChildCount() > 0) {
      final ViewGroup vg = (ViewGroup) view;
      final List<View> clickableViews = new ArrayList<>();
      for (int i = 0; i < vg.getChildCount(); i++) {
        clickableViews.add(vg.getChildAt(i));
      }
      nativeAd.registerViewForInteraction(view, clickableViews);
    } else {
      nativeAd.registerViewForInteraction(view);
    }
  }

  static class FacebookStaticNativeAd extends StaticNativeAd implements AdListener {
    private static final String SOCIAL_CONTEXT_FOR_AD = "socialContextForAd";

    private final Context mContext;
    private final NativeAd mNativeAd;
    private final CustomEventNativeListener mCustomEventNativeListener;

    FacebookStaticNativeAd(final Context context,
                           final NativeAd nativeAd,
                           final CustomEventNativeListener customEventNativeListener) {
      mContext = context.getApplicationContext();
      mNativeAd = nativeAd;
      mCustomEventNativeListener = customEventNativeListener;
    }

    void loadAd() {
      mNativeAd.setAdListener(this);
      mNativeAd.loadAd();
    }

    // AdListener
    @Override
    public void onAdLoaded(final Ad ad) {
      // This identity check is from Facebook's Native API sample code:
      // https://developers.facebook.com/docs/audience-network/android/native-api
      if (!mNativeAd.equals(ad) || !mNativeAd.isAdLoaded()) {
        mCustomEventNativeListener.onNativeAdFailed(NativeErrorCode.NETWORK_INVALID_STATE);
        return;
      }

      setTitle(mNativeAd.getAdTitle());
      setText(mNativeAd.getAdBody());

      final NativeAd.Image coverImage = mNativeAd.getAdCoverImage();
      setMainImageUrl(coverImage == null ? null : coverImage.getUrl());

      final NativeAd.Image icon = mNativeAd.getAdIcon();
      setIconImageUrl(icon == null ? null : icon.getUrl());

      setCallToAction(mNativeAd.getAdCallToAction());
      setStarRating(getDoubleRating(mNativeAd.getAdStarRating()));

      addExtra(SOCIAL_CONTEXT_FOR_AD, mNativeAd.getAdSocialContext());

      final NativeAd.Image adChoicesIconImage = mNativeAd.getAdChoicesIcon();
      setPrivacyInformationIconImageUrl(adChoicesIconImage == null ? null : adChoicesIconImage
          .getUrl());
      setPrivacyInformationIconClickThroughUrl(mNativeAd.getAdChoicesLinkUrl());

      final List<String> imageUrls = new ArrayList<String>();
      final String mainImageUrl = getMainImageUrl();
      if (mainImageUrl != null) {
        imageUrls.add(getMainImageUrl());
      }
      final String iconUrl = getIconImageUrl();
      if (iconUrl != null) {
        imageUrls.add(getIconImageUrl());
      }
      final String privacyInformationIconImageUrl = getPrivacyInformationIconImageUrl();
      if (privacyInformationIconImageUrl != null) {
        imageUrls.add(privacyInformationIconImageUrl);
      }

      preCacheImages(mContext, imageUrls, new NativeImageHelper.ImageListener() {
        @Override
        public void onImagesCached() {
          mCustomEventNativeListener.onNativeAdLoaded(FacebookStaticNativeAd.this);
        }

        @Override
        public void onImagesFailedToCache(NativeErrorCode errorCode) {
          mCustomEventNativeListener.onNativeAdFailed(errorCode);
        }
      });
    }

    @Override
    public void onError(final Ad ad, final AdError adError) {
      if (adError == null) {
        mCustomEventNativeListener.onNativeAdFailed(NativeErrorCode.UNSPECIFIED);
      } else if (adError.getErrorCode() == AdError.NO_FILL.getErrorCode()) {
        mCustomEventNativeListener.onNativeAdFailed(NativeErrorCode.NETWORK_NO_FILL);
      } else if (adError.getErrorCode() == AdError.INTERNAL_ERROR.getErrorCode()) {
        mCustomEventNativeListener.onNativeAdFailed(NativeErrorCode.NETWORK_INVALID_STATE);
      } else {
        mCustomEventNativeListener.onNativeAdFailed(NativeErrorCode.UNSPECIFIED);
      }
    }

    @Override
    public void onAdClicked(final Ad ad) {
      notifyAdClicked();
    }

    @Override
    public void onLoggingImpression(final Ad ad) {
      notifyAdImpressed();
    }

    // BaseForwardingNativeAd
    @Override
    public void prepare(final View view) {
      registerChildViewsForInteraction(view, mNativeAd);
    }

    @Override
    public void clear(final View view) {
      mNativeAd.unregisterView();
    }

    @Override
    public void destroy() {
      mNativeAd.destroy();
    }

    private Double getDoubleRating(final Rating rating) {
      if (rating == null) {
        return null;
      }

      return MAX_STAR_RATING * rating.getValue() / rating.getScale();
    }
  }


  static class FacebookVideoEnabledNativeAd extends BaseNativeAd implements AdListener {
    private static final String SOCIAL_CONTEXT_FOR_AD = "socialContextForAd";

    static final double MIN_STAR_RATING = 0;
    static final double MAX_STAR_RATING = 5;

    private final Context mContext;
    private final NativeAd mNativeAd;
    private final CustomEventNativeListener mCustomEventNativeListener;

    private Double mStarRating;

    private final Map<String, Object> mExtras;

    FacebookVideoEnabledNativeAd(final Context context,
                                 final NativeAd nativeAd,
                                 final CustomEventNativeListener customEventNativeListener) {
      mContext = context.getApplicationContext();
      mNativeAd = nativeAd;
      mCustomEventNativeListener = customEventNativeListener;
      mExtras = new HashMap<String, Object>();
    }

    void loadAd() {
      mNativeAd.setAdListener(this);
      mNativeAd.loadAd();
    }

    /**
     * Returns the String corresponding to the ad's title.
     */
    final public String getTitle() {
      return mNativeAd.getAdTitle();
    }

    /**
     * Returns the String corresponding to the ad's body text. May be null.
     */
    final public String getText() {
      return mNativeAd.getAdBody();
    }

    /**
     * Returns the String url corresponding to the ad's main image. May be null.
     */
    final public String getMainImageUrl() {
      final NativeAd.Image coverImage = mNativeAd.getAdCoverImage();
      return coverImage == null ? null : coverImage.getUrl();
    }

    /**
     * Returns the String url corresponding to the ad's icon image. May be null.
     */
    final public String getIconImageUrl() {
      final NativeAd.Image icon = mNativeAd.getAdIcon();
      return icon == null ? null : icon.getUrl();
    }

    /**
     * Returns the Call To Action String (i.e. "Download" or "Learn More") associated with this ad.
     */
    final public String getCallToAction() {
      return mNativeAd.getAdCallToAction();
    }

    /**
     * For app install ads, this returns the associated star rating (on a 5 star scale) for the
     * advertised app. Note that this method may return null if the star rating was either never set
     * or invalid.
     */
    final public Double getStarRating() {
      return mStarRating;
    }

    /**
     * Returns the Privacy Information click through url.
     *
     * @return String representing the Privacy Information Icon click through url, or {@code null}
     * if not set.
     */
    final public String getPrivacyInformationIconClickThroughUrl() {
      return mNativeAd.getAdChoicesLinkUrl();
    }

    /**
     * Returns the Privacy Information image url.
     *
     * @return String representing the Privacy Information Icon click through url, or {@code
     * null} if not set.
     */
    final public String getPrivacyInformationIconImageUrl() {
      return mNativeAd.getAdChoicesIcon() == null ? null : mNativeAd.getAdChoicesIcon().getUrl();
    }

    // AdListener
    @Override
    public void onAdLoaded(final Ad ad) {
      // This identity check is from Facebook's Native API sample code:
      // https://developers.facebook.com/docs/audience-network/android/native-api
      if (!mNativeAd.equals(ad) || !mNativeAd.isAdLoaded()) {
        mCustomEventNativeListener.onNativeAdFailed(NativeErrorCode.NETWORK_INVALID_STATE);
        return;
      }

      setStarRating(getDoubleRating(mNativeAd.getAdStarRating()));

      addExtra(SOCIAL_CONTEXT_FOR_AD, mNativeAd.getAdSocialContext());

      final List<String> imageUrls = new ArrayList<String>();
      final String mainImageUrl = getMainImageUrl();
      if (mainImageUrl != null) {
        imageUrls.add(mainImageUrl);
      }
      final String iconImageUrl = getIconImageUrl();
      if (iconImageUrl != null) {
        imageUrls.add(iconImageUrl);
      }
      final String privacyInformationIconImageUrl = getPrivacyInformationIconImageUrl();
      if (privacyInformationIconImageUrl != null) {
        imageUrls.add(privacyInformationIconImageUrl);
      }

      preCacheImages(mContext, imageUrls, new NativeImageHelper.ImageListener() {
        @Override
        public void onImagesCached() {
          mCustomEventNativeListener.onNativeAdLoaded(FacebookVideoEnabledNativeAd.this);
        }

        @Override
        public void onImagesFailedToCache(NativeErrorCode errorCode) {
          mCustomEventNativeListener.onNativeAdFailed(errorCode);
        }
      });
    }

    @Override
    public void onError(final Ad ad, final AdError adError) {
      if (adError == null) {
        mCustomEventNativeListener.onNativeAdFailed(NativeErrorCode.UNSPECIFIED);
      } else if (adError.getErrorCode() == AdError.NO_FILL.getErrorCode()) {
        mCustomEventNativeListener.onNativeAdFailed(NativeErrorCode.NETWORK_NO_FILL);
      } else if (adError.getErrorCode() == AdError.INTERNAL_ERROR.getErrorCode()) {
        mCustomEventNativeListener.onNativeAdFailed(NativeErrorCode.NETWORK_INVALID_STATE);
      } else {
        mCustomEventNativeListener.onNativeAdFailed(NativeErrorCode.UNSPECIFIED);
      }
    }

    @Override
    public void onAdClicked(final Ad ad) {
      notifyAdClicked();
    }

    @Override
    public void onLoggingImpression(final Ad ad) {
      notifyAdImpressed();
    }

    // BaseForwardingNativeAd
    @Override
    public void prepare(final View view) {
      registerChildViewsForInteraction(view, mNativeAd);
    }

    @Override
    public void clear(final View view) {
      mNativeAd.unregisterView();
    }

    @Override
    public void destroy() {
      mNativeAd.destroy();
    }

    /**
     * Given a particular String key, return the associated Object value from the ad's extras map.
     * See {@link StaticNativeAd#getExtras()} for more information.
     */
    final public Object getExtra(final String key) {
      if (!Preconditions.NoThrow.checkNotNull(key, "getExtra key is not allowed to be null")) {
        return null;
      }
      return mExtras.get(key);
    }

    /**
     * Returns a copy of the extras map, reflecting additional ad content not reflected in any
     * of the above hardcoded setters. This is particularly useful for passing down custom fields
     * with MoPub's direct-sold native ads or from mediated networks that pass back additional
     * fields.
     */
    final public Map<String, Object> getExtras() {
      return new HashMap<String, Object>(mExtras);
    }

    final public void addExtra(final String key, final Object value) {
      if (!Preconditions.NoThrow.checkNotNull(key, "addExtra key is not allowed to be null")) {
        return;
      }
      mExtras.put(key, value);
    }

    /**
     * Attaches the native ad to the MediaView, if it exists.
     *
     * @param mediaView The View that holds the main media.
     */
    public void updateMediaView(final MediaView mediaView) {
      if (mediaView != null) {
        mediaView.setNativeAd(mNativeAd);
      }
    }

    private void setStarRating(final Double starRating) {
      if (starRating == null) {
        mStarRating = null;
      } else if (starRating >= MIN_STAR_RATING && starRating <= MAX_STAR_RATING) {
        mStarRating = starRating;
      } else {
        MoPubLog.d("Ignoring attempt to set invalid star rating (" + starRating + "). Must be "
                   + "between " + MIN_STAR_RATING + " and " + MAX_STAR_RATING + ".");
      }
    }

    private Double getDoubleRating(final Rating rating) {
      if (rating == null) {
        return null;
      }

      return MAX_STAR_RATING * rating.getValue() / rating.getScale();
    }
  }
}
