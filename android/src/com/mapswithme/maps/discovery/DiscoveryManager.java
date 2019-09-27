package com.mapswithme.maps.discovery;

import android.annotation.SuppressLint;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.promo.PromoCityGallery;
import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.EnumSet;

public enum DiscoveryManager
{
  INSTANCE;
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = DiscoveryManager.class.getSimpleName();
  private static final EnumSet<ItemType> AGGREGATE_EMPTY_RESULTS = EnumSet.noneOf(ItemType.class);
  @Nullable
  private DiscoveryResultReceiver mCallback;
  private int mRequestedTypesCount;


  public void discover(@NonNull DiscoveryParams params)
  {
    LOGGER.d(TAG, "discover: " + params);
    AGGREGATE_EMPTY_RESULTS.clear();
    mRequestedTypesCount = params.getItemTypes().length;
    DiscoveryManager.nativeDiscover(params);
  }

  // Called from JNI.
  @SuppressLint("SwitchIntDef")
  @MainThread
  private void onResultReceived(final @NonNull SearchResult[] results,
                                final @DiscoveryParams.ItemType int typeIndex)
  {
    if (typeIndex >= ItemType.values().length)
    {
      throw new AssertionError("Unsupported discovery item type " +
                               "'" + typeIndex + "' for search results!");

    }
    ItemType type = ItemType.values()[typeIndex];
    notifyUiWithCheck(results, type, callback -> onResultReceivedSafely(callback, type, results));
  }

  private void onResultReceivedSafely(@NonNull DiscoveryResultReceiver callback,
                                      @NonNull ItemType type,
                                      @NonNull SearchResult[] results)
  {
    type.onResultReceived(callback, results);
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  private void onLocalExpertsReceived(@NonNull final LocalExpert[] experts)
  {
    notifyUiWithCheck(experts, ItemType.LOCAL_EXPERTS,
                      callback -> callback.onLocalExpertsReceived(experts));
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  private void onPromoCityGalleryReceived(@NonNull PromoCityGallery gallery)
  {
    notifyUiWithCheck(gallery.getItems(), ItemType.PROMO,
                      callback -> callback.onCatalogPromoResultReceived(gallery));
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  private void onError(@DiscoveryParams.ItemType int type)
  {
    LOGGER.w(TAG, "onError for type: " + type);
    if (mCallback != null)
      mCallback.onError(ItemType.values()[type]);
  }

  private <T> void notifyUiWithCheck(@NonNull T[] results, @NonNull ItemType type,
                                     @NonNull Action action)
  {
    LOGGER.d(TAG, "Results size = " + results.length + " for type: " + type);
    if (mCallback == null)
      return;

    if (isAggregateResultsEmpty(results, type))
    {
      mCallback.onNotFound();
      return;
    }

    action.run(mCallback);
  }

  private <T> boolean isAggregateResultsEmpty(@NonNull T[] results, @NonNull ItemType type)
  {
    if (results.length == 0)
      AGGREGATE_EMPTY_RESULTS.add(type);

    return mRequestedTypesCount == AGGREGATE_EMPTY_RESULTS.size();
  }

  void attach(@NonNull DiscoveryResultReceiver callback)
  {
    LOGGER.d(TAG, "attach callback: " + callback);
    mCallback = callback;
  }

  void detach()
  {
    LOGGER.d(TAG, "detach callback: " + mCallback);
    mCallback = null;
  }

  public static native void nativeDiscover(@NonNull DiscoveryParams params);

  @NonNull
  public static native String nativeGetLocalExpertsUrl();

  interface Action
  {
    void run(@NonNull DiscoveryResultReceiver callback);
  }
}
