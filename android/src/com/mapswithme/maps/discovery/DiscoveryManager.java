package com.mapswithme.maps.discovery;

import android.annotation.SuppressLint;
import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.maps.viator.ViatorProduct;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.EnumSet;

enum DiscoveryManager
{
  INSTANCE;
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = DiscoveryManager.class.getSimpleName();
  private static final EnumSet<ItemType> AGGREGATE_EMPTY_RESULTS = EnumSet.noneOf(ItemType.class);
  @Nullable
  private UICallback mCallback;
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
                                final @DiscoveryParams.ItemType int type)
  {
    notifyUiWithCheck(results, ItemType.values()[type], new Action()
    {
      @Override
      public void run(@NonNull UICallback callback)
      {
        switch (type)
        {
          case DiscoveryParams.ITEM_TYPE_ATTRACTIONS:
            callback.onAttractionsReceived(results);
            break;
          case DiscoveryParams.ITEM_TYPE_CAFES:
            callback.onCafesReceived(results);
            break;
          case DiscoveryParams.ITEM_TYPE_HOTELS:
            callback.onHotelsReceived(results);
            break;
          default:
            throw new AssertionError("Unsupported discovery item type " +
                                     "'" + type + "' for search results!");
        }
      }
    });
  }

  // Called from JNI.
  @MainThread
  private void onViatorProductsReceived(@NonNull final ViatorProduct[] products)
  {
    throw new UnsupportedOperationException("Viator is not supported!");
  }

  // Called from JNI.
  @MainThread
  private void onLocalExpertsReceived(@NonNull final LocalExpert[] experts)
  {
    notifyUiWithCheck(experts, ItemType.LOCAL_EXPERTS,
                      callback -> callback.onLocalExpertsReceived(experts));
  }

  // Called from JNI.
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

  void attach(@NonNull UICallback callback)
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
  public static native String nativeGetViatorUrl();

  @NonNull
  public static native String nativeGetLocalExpertsUrl();

  interface Action
  {
    void run(@NonNull UICallback callback);
  }
}
