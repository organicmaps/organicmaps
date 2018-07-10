package com.mopub.nativeads;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;

import com.google.ads.mediation.admob.AdMobAdapter;
import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdLoader;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.formats.NativeAdOptions;
import com.google.android.gms.ads.formats.NativeAppInstallAd;
import com.google.android.gms.ads.formats.NativeContentAd;
import com.mopub.common.MediationSettings;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;

public class GooglePlayServicesNative extends CustomEventNative {
    protected static final String TAG = "MoPubToAdMobNative";

    /**
     * The current version of the adapter.
     */
    private static final String ADAPTER_VERSION = "0.3.1";

    /**
     * Key to obtain AdMob application ID from the server extras provided by MoPub.
     */
    private static final String KEY_EXTRA_APPLICATION_ID = "appid";

    /**
     * Key to obtain AdMob ad unit ID from the extras provided by MoPub.
     */
    private static final String KEY_EXTRA_AD_UNIT_ID = "adunit";

    /**
     * Key to set and obtain the image orientation preference.
     */
    public static final String KEY_EXTRA_ORIENTATION_PREFERENCE = "orientation_preference";

    /**
     * Key to set and obtain the AdChoices icon placement preference.
     */
    public static final String KEY_EXTRA_AD_CHOICES_PLACEMENT = "ad_choices_placement";

    /**
     * Key to set and obtain the experimental swap margins flag.
     */
    public static final String KEY_EXPERIMENTAL_EXTRA_SWAP_MARGINS = "swap_margins";

    /**
     * Flag to determine whether or not the adapter has been initialized.
     */
    private static AtomicBoolean sIsInitialized = new AtomicBoolean(false);

    @Override
    protected void loadNativeAd(@NonNull final Context context,
                                @NonNull final CustomEventNativeListener customEventNativeListener,
                                @NonNull Map<String, Object> localExtras,
                                @NonNull Map<String, String> serverExtras) {

        if (!sIsInitialized.getAndSet(true)) {
            Log.i(TAG, "Adapter version - " + ADAPTER_VERSION);
            if (serverExtras.containsKey(KEY_EXTRA_APPLICATION_ID)
                && !TextUtils.isEmpty(serverExtras.get(KEY_EXTRA_APPLICATION_ID))) {
                MobileAds.initialize(context, serverExtras.get(KEY_EXTRA_APPLICATION_ID));
            } else {
                MobileAds.initialize(context);
            }
        }

        String adUnitId = serverExtras.get(KEY_EXTRA_AD_UNIT_ID);
        if (TextUtils.isEmpty(adUnitId)) {
            customEventNativeListener.onNativeAdFailed(NativeErrorCode.NETWORK_INVALID_REQUEST);
            return;
        }
        GooglePlayServicesNativeAd nativeAd =
          new GooglePlayServicesNativeAd(customEventNativeListener);
        nativeAd.loadAd(context, adUnitId, localExtras);
    }

    /**
     * The {@link GooglePlayServicesNativeAd} class is used to load and map Google native
     * ads to MoPub native ads.
     */
    static class GooglePlayServicesNativeAd extends BaseNativeAd {

        // Native ad assets.
        private String mTitle;
        private String mText;
        private String mMainImageUrl;
        private String mIconImageUrl;
        private String mCallToAction;
        private Double mStarRating;
        private String mAdvertiser;
        private String mStore;
        private String mPrice;

        /**
         * Flag to determine whether or not to swap margins from actual ad view to Google native ad
         * view.
         */
        private boolean mSwapMargins;

        /**
         * A custom event native listener used to forward Google Mobile Ads SDK events to MoPub.
         */
        private CustomEventNativeListener mCustomEventNativeListener;

        /**
         * A Google native ad of type content.
         */
        private NativeContentAd mNativeContentAd;

        /**
         * A Google native ad of type app install.
         */
        private NativeAppInstallAd mNativeAppInstallAd;

        public GooglePlayServicesNativeAd(
          CustomEventNativeListener customEventNativeListener) {
            this.mCustomEventNativeListener = customEventNativeListener;
        }

        /**
         * @return the title string associated with this native ad.
         */
        public String getTitle() {
            return mTitle;
        }

        /**
         * @return the text/body string associated with the native ad.
         */
        public String getText() {
            return mText;
        }

        /**
         * @return the main image URL associated with the native ad.
         */
        public String getMainImageUrl() {
            return mMainImageUrl;
        }

        /**
         * @return the icon image URL associated with the native ad.
         */
        public String getIconImageUrl() {
            return mIconImageUrl;
        }

        /**
         * @return the call to action string associated with the native ad.
         */
        public String getCallToAction() {
            return mCallToAction;
        }

        /**
         * @return the star rating associated with the native ad.
         */
        public Double getStarRating() {
            return mStarRating;
        }

        /**
         * @return the advertiser string associated with the native ad.
         */
        public String getAdvertiser() {
            return mAdvertiser;
        }

        /**
         * @return the store string associated with the native ad.
         */
        public String getStore() {
            return mStore;
        }

        /**
         * @return the price string associated with the native ad.
         */
        public String getPrice() {
            return mPrice;
        }

        /**
         * @param title the title to be set.
         */
        public void setTitle(String title) {
            this.mTitle = title;
        }

        /**
         * @param text the text/body to be set.
         */
        public void setText(String text) {
            this.mText = text;
        }

        /**
         * @param mainImageUrl the main image URL to be set.
         */
        public void setMainImageUrl(String mainImageUrl) {
            this.mMainImageUrl = mainImageUrl;
        }

        /**
         * @param iconImageUrl the icon image URL to be set.
         */
        public void setIconImageUrl(String iconImageUrl) {
            this.mIconImageUrl = iconImageUrl;
        }

        /**
         * @param callToAction the call to action string to be set.
         */
        public void setCallToAction(String callToAction) {
            this.mCallToAction = callToAction;
        }

        /**
         * @param starRating the star rating value to be set.
         */
        public void setStarRating(Double starRating) {
            this.mStarRating = starRating;
        }

        /**
         * @param advertiser the advertiser string to be set.
         */
        public void setAdvertiser(String advertiser) {
            this.mAdvertiser = advertiser;
        }

        /**
         * @param store the store string to be set.
         */
        public void setStore(String store) {
            this.mStore = store;
        }

        /**
         * @param price the price string to be set.
         */
        public void setPrice(String price) {
            this.mPrice = price;
        }

        /**
         * @return whether or not this ad is native content ad.
         */
        public boolean isNativeContentAd() {
            return mNativeContentAd != null;
        }

        /**
         * @return whether or not to swap margins when rendering the ad.
         */
        public boolean shouldSwapMargins() {
            return this.mSwapMargins;
        }

        /**
         * @return whether or not this ad is native app install ad.
         */
        public boolean isNativeAppInstallAd() {
            return mNativeAppInstallAd != null;
        }

        /**
         * @return {@link #mNativeContentAd}.
         */
        public NativeContentAd getContentAd() {
            return mNativeContentAd;
        }

        /**
         * @return {@link #mNativeAppInstallAd}.
         */
        public NativeAppInstallAd getAppInstallAd() {
            return mNativeAppInstallAd;
        }

        /**
         * This method will load native ads from Google for the given ad unit ID.
         *
         * @param context  required to request a Google native ad.
         * @param adUnitId Google's AdMob Ad Unit ID.
         */
        public void loadAd(final Context context, String adUnitId,
                           Map<String, Object> localExtras) {

            AdLoader.Builder builder = new AdLoader.Builder(context, adUnitId);

            // Get the experimental swap margins extra.
            if (localExtras.containsKey(KEY_EXPERIMENTAL_EXTRA_SWAP_MARGINS)) {
                Object swapMarginExtra = localExtras.get(KEY_EXPERIMENTAL_EXTRA_SWAP_MARGINS);
                if (swapMarginExtra instanceof Boolean) {
                    mSwapMargins = (boolean) swapMarginExtra;
                }
            }

            NativeAdOptions.Builder optionsBuilder = new NativeAdOptions.Builder();

            // MoPub requires the images to be pre-cached using their APIs, so we do not want
            // Google to download the image assets.
            optionsBuilder.setReturnUrlsForImageAssets(true);

            // MoPub allows for only one image, so only request for one image.
            optionsBuilder.setRequestMultipleImages(false);

            // Get the preferred image orientation from the local extras.
            if (localExtras.containsKey(KEY_EXTRA_ORIENTATION_PREFERENCE)
                && isValidOrientationExtra(localExtras.get(KEY_EXTRA_ORIENTATION_PREFERENCE))) {
                optionsBuilder.setImageOrientation(
                  (int) localExtras.get(KEY_EXTRA_ORIENTATION_PREFERENCE));
            }

            // Get the preferred AdChoices icon placement from the local extras.
            if (localExtras.containsKey(KEY_EXTRA_AD_CHOICES_PLACEMENT)
                && isValidAdChoicesPlacementExtra(
              localExtras.get(KEY_EXTRA_AD_CHOICES_PLACEMENT))) {
                optionsBuilder.setAdChoicesPlacement(
                  (int) localExtras.get(KEY_EXTRA_AD_CHOICES_PLACEMENT));
            }
            NativeAdOptions adOptions = optionsBuilder.build();

            AdLoader adLoader =
              builder.forContentAd(new NativeContentAd.OnContentAdLoadedListener() {
                  @Override
                  public void onContentAdLoaded(final NativeContentAd nativeContentAd) {
                      if (!isValidContentAd(nativeContentAd)) {
                          Log.i(TAG, "The Google native content ad is missing one or more "
                                     + "required assets, failing request.");
                          mCustomEventNativeListener.onNativeAdFailed(
                            NativeErrorCode.INVALID_RESPONSE);
                          return;
                      }

                      mNativeContentAd = nativeContentAd;
                      List<com.google.android.gms.ads.formats.NativeAd.Image> images =
                        nativeContentAd.getImages();
                      List<String> imageUrls = new ArrayList<>();
                      // Only one image should be in the the list as we turned off request
                      // for multiple images.
                      com.google.android.gms.ads.formats.NativeAd.Image mainImage =
                        images.get(0);
                      // Assuming that the URI provided is an URL.
                      imageUrls.add(mainImage.getUri().toString());

                      com.google.android.gms.ads.formats.NativeAd.Image logoImage =
                        nativeContentAd.getLogo();
                      // Assuming that the URI provided is an URL.
                      imageUrls.add(logoImage.getUri().toString());
                      preCacheImages(context, imageUrls);
                  }
              }).forAppInstallAd(new NativeAppInstallAd.OnAppInstallAdLoadedListener() {
                  @Override
                  public void onAppInstallAdLoaded(
                    final NativeAppInstallAd nativeAppInstallAd) {
                      if (!isValidAppInstallAd(nativeAppInstallAd)) {
                          Log.i(TAG, "The Google native app install ad is missing one or "
                                     + "more required assets, failing request.");
                          mCustomEventNativeListener.onNativeAdFailed(
                            NativeErrorCode.INVALID_RESPONSE);
                          return;
                      }
                      mNativeAppInstallAd = nativeAppInstallAd;
                      List<com.google.android.gms.ads.formats.NativeAd.Image> images =
                        nativeAppInstallAd.getImages();
                      List<String> imageUrls = new ArrayList<>();
                      // Only one image should be in the the list as we turned off request
                      // for multiple images.
                      com.google.android.gms.ads.formats.NativeAd.Image mainImage =
                        images.get(0);
                      // Assuming that the URI provided is an URL.
                      imageUrls.add(mainImage.getUri().toString());

                      com.google.android.gms.ads.formats.NativeAd.Image iconImage =
                        nativeAppInstallAd.getIcon();
                      // Assuming that the URI provided is an URL.
                      imageUrls.add(iconImage.getUri().toString());
                      preCacheImages(context, imageUrls);
                  }
              }).withAdListener(new AdListener() {
                  @Override
                  public void onAdClicked() {
                      super.onAdClicked();
                      GooglePlayServicesNativeAd.this.notifyAdClicked();
                  }

                  @Override
                  public void onAdImpression() {
                      super.onAdImpression();
                      GooglePlayServicesNativeAd.this.notifyAdImpressed();
                  }

                  @Override
                  public void onAdFailedToLoad(int errorCode) {
                      super.onAdFailedToLoad(errorCode);
                      switch (errorCode) {
                          case AdRequest.ERROR_CODE_INTERNAL_ERROR:
                              mCustomEventNativeListener.onNativeAdFailed(
                                NativeErrorCode.NATIVE_ADAPTER_CONFIGURATION_ERROR);
                              break;
                          case AdRequest.ERROR_CODE_INVALID_REQUEST:
                              mCustomEventNativeListener.onNativeAdFailed(
                                NativeErrorCode.NETWORK_INVALID_REQUEST);
                              break;
                          case AdRequest.ERROR_CODE_NETWORK_ERROR:
                              mCustomEventNativeListener.onNativeAdFailed(
                                NativeErrorCode.CONNECTION_ERROR);
                              break;
                          case AdRequest.ERROR_CODE_NO_FILL:
                              mCustomEventNativeListener.onNativeAdFailed(
                                NativeErrorCode.NETWORK_NO_FILL);
                              break;
                          default:
                              mCustomEventNativeListener.onNativeAdFailed(
                                NativeErrorCode.UNSPECIFIED);
                      }
                  }
              }).withNativeAdOptions(adOptions).build();
            AdRequest.Builder requestBuilder = new AdRequest.Builder();
            requestBuilder.setRequestAgent("MoPub");

            // Consent collected from the MoPub’s consent dialogue should not be used to set up
            // Google's personalization preference. Publishers should work with Google to be GDPR-compliant.
            forwardNpaIfSet(requestBuilder);

            AdRequest adRequest = requestBuilder.build();
            adLoader.loadAd(adRequest);
        }

        private void forwardNpaIfSet(AdRequest.Builder builder) {

            // Only forward the "npa" bundle if it is explicitly set. Otherwise, don't attach it with the ad request.
            if (GooglePlayServicesMediationSettings.getNpaBundle() != null &&
                !GooglePlayServicesMediationSettings.getNpaBundle().isEmpty()) {
                builder.addNetworkExtrasBundle(AdMobAdapter.class, GooglePlayServicesMediationSettings.getNpaBundle());
            }
        }

        /**
         * This method will check whether or not the provided extra value can be mapped to
         * NativeAdOptions' orientation constants.
         *
         * @param extra to be checked if it is valid.
         * @return {@code true} if the extra can be mapped to one of {@link NativeAdOptions}
         * orientation constants, {@code false} otherwise.
         */
        private boolean isValidOrientationExtra(Object extra) {
            if (extra == null || !(extra instanceof Integer)) {
                return false;
            }
            Integer preference = (Integer) extra;
            return (preference == NativeAdOptions.ORIENTATION_ANY
                    || preference == NativeAdOptions.ORIENTATION_LANDSCAPE
                    || preference == NativeAdOptions.ORIENTATION_PORTRAIT);
        }

        /**
         * Checks whether or not the provided extra value can be mapped to NativeAdOptions'
         * AdChoices icon placement constants.
         *
         * @param extra to be checked if it is valid.
         * @return {@code true} if the extra can be mapped to one of {@link NativeAdOptions}
         * AdChoices icon placement constants, {@code false} otherwise.
         */
        private boolean isValidAdChoicesPlacementExtra(Object extra) {
            if (extra == null || !(extra instanceof Integer)) {
                return false;
            }
            Integer placement = (Integer) extra;
            return (placement == NativeAdOptions.ADCHOICES_TOP_LEFT
                    || placement == NativeAdOptions.ADCHOICES_TOP_RIGHT
                    || placement == NativeAdOptions.ADCHOICES_BOTTOM_LEFT
                    || placement == NativeAdOptions.ADCHOICES_BOTTOM_RIGHT);
        }

        /**
         * This method will check whether or not the given content ad has all the required assets
         * (title, text, main image url, icon url and call to action) for it to be correctly
         * mapped to a {@link GooglePlayServicesNativeAd}.
         *
         * @param contentAd to be checked if it is valid.
         * @return {@code true} if the given native content ad has all the necessary assets to
         * create a {@link GooglePlayServicesNativeAd}, {@code false} otherwise.
         */
        private boolean isValidContentAd(NativeContentAd contentAd) {
            return (contentAd.getHeadline() != null && contentAd.getBody() != null
                    && contentAd.getImages() != null && contentAd.getImages().size() > 0
                    && contentAd.getImages().get(0) != null && contentAd.getLogo() != null
                    && contentAd.getCallToAction() != null);
        }

        /**
         * This method will check whether or not the given native app install ad has all the
         * required assets (title, text, main image url, icon url and call to action) for it to
         * be correctly mapped to a {@link GooglePlayServicesNativeAd}.
         *
         * @param appInstallAd to checked if it is valid.
         * @return {@code true} if the given native app install ad has all the necessary assets to
         * to create a {@link GooglePlayServicesNativeAd}, {@code false} otherwise.
         */
        private boolean isValidAppInstallAd(NativeAppInstallAd appInstallAd) {
            return (appInstallAd.getHeadline() != null && appInstallAd.getBody() != null
                    && appInstallAd.getImages() != null && appInstallAd.getImages().size() > 0
                    && appInstallAd.getImages().get(0) != null && appInstallAd.getIcon() != null
                    && appInstallAd.getCallToAction() != null);
        }

        @Override
        public void prepare(@NonNull View view) {
            // Adding click and impression trackers is handled by the GooglePlayServicesRenderer,
            // do nothing here.
        }

        @Override
        public void clear(@NonNull View view) {
            // Called when an ad is no longer displayed to a user.
            // GooglePlayServicesAdRenderer.removeGoogleNativeAdView(view, shouldSwapMargins());
        }

        @Override
        public void destroy() {
            // Called when the ad will never be displayed again.
            if (mNativeContentAd != null) {
                mNativeContentAd.destroy();
            }
            if (mNativeAppInstallAd != null) {
                mNativeAppInstallAd.destroy();
            }
        }

        /**
         * This method will try to cache images and send success/failure callbacks based on
         * whether or not the image caching succeeded.
         *
         * @param context   required to pre-cache images.
         * @param imageUrls the urls of images that need to be cached.
         */
        private void preCacheImages(Context context, List<String> imageUrls) {
            NativeImageHelper.preCacheImages(context, imageUrls,
                                             new NativeImageHelper.ImageListener() {
                                                 @Override
                                                 public void onImagesCached() {
                                                     if (mNativeContentAd != null) {
                                                         prepareNativeContentAd(mNativeContentAd);
                                                         mCustomEventNativeListener.onNativeAdLoaded(
                                                           GooglePlayServicesNativeAd.this);
                                                     } else if (mNativeAppInstallAd != null) {
                                                         prepareNativeAppInstallAd(mNativeAppInstallAd);
                                                         mCustomEventNativeListener.onNativeAdLoaded(
                                                           GooglePlayServicesNativeAd.this);
                                                     }
                                                 }

                                                 @Override
                                                 public void onImagesFailedToCache(NativeErrorCode errorCode) {
                                                     mCustomEventNativeListener.onNativeAdFailed(errorCode);
                                                 }
                                             });
        }

        /**
         * This method will map the Google native content ad loaded to this
         * {@link GooglePlayServicesNativeAd}.
         *
         * @param contentAd that needs to be mapped to this native ad.
         */
        private void prepareNativeContentAd(NativeContentAd contentAd) {
            List<com.google.android.gms.ads.formats.NativeAd.Image> images = contentAd.getImages();
            setMainImageUrl(images.get(0).getUri().toString());

            com.google.android.gms.ads.formats.NativeAd.Image logo = contentAd.getLogo();
            setIconImageUrl(logo.getUri().toString());

            setCallToAction(contentAd.getCallToAction().toString());

            setTitle(contentAd.getHeadline().toString());

            setText(contentAd.getBody().toString());

            setAdvertiser(contentAd.getAdvertiser().toString());
        }

        /**
         * This method will map the Google native app install ad loaded to this
         * {@link GooglePlayServicesNativeAd}.
         *
         * @param appInstallAd that needs to be mapped to this native ad.
         */
        private void prepareNativeAppInstallAd(NativeAppInstallAd appInstallAd) {
            List<com.google.android.gms.ads.formats.NativeAd.Image> images =
              appInstallAd.getImages();
            setMainImageUrl(images.get(0).getUri().toString());

            com.google.android.gms.ads.formats.NativeAd.Image icon = appInstallAd.getIcon();
            setIconImageUrl(icon.getUri().toString());

            setCallToAction(appInstallAd.getCallToAction().toString());

            setTitle(appInstallAd.getHeadline().toString());

            setText(appInstallAd.getBody().toString());

            if (appInstallAd.getStarRating() != null) {
                setStarRating(appInstallAd.getStarRating());
            }

            // Add store asset if available.
            if (appInstallAd.getStore() != null) {
                setStore(appInstallAd.getStore().toString());
            }

            // Add price asset if available.
            if (appInstallAd.getPrice() != null) {
                setPrice(appInstallAd.getPrice().toString());
            }
        }
    }

    public static final class GooglePlayServicesMediationSettings implements MediationSettings {
        private static Bundle npaBundle;

        public GooglePlayServicesMediationSettings() {
        }

        public GooglePlayServicesMediationSettings(Bundle bundle) {
            npaBundle = bundle;
        }

        public void setNpaBundle(Bundle bundle) {
            npaBundle = bundle;
        }

        /* The MoPub Android SDK queries MediationSettings from the rewarded video code
        (MoPubRewardedVideoManager.getGlobalMediationSettings). That API might not always be
        available to publishers importing the modularized SDK(s) based on select ad formats.
        This is a workaround to statically get the "npa" Bundle passed to us via the constructor. */
        private static Bundle getNpaBundle() {
            return npaBundle;
        }
    }
}