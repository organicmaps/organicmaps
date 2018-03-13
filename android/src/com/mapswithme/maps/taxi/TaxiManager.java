package com.mapswithme.maps.taxi;

import android.content.Context;
import android.location.Location;
import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.SponsoredLinks;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.statistics.Statistics;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

@MainThread
public class TaxiManager
{
  public static final TaxiManager INSTANCE = new TaxiManager();

  @NonNull
  private final List<TaxiInfo> mProviders = new ArrayList<>();
  @NonNull
  private final List<TaxiInfoError> mErrors = new ArrayList<>();
  @Nullable
  private TaxiListener mListener;

  private TaxiManager()
  {
  }

  // Called from JNI.
  @MainThread
  void onTaxiProvidersReceived(@NonNull TaxiInfo[] providers)
  {
    if (!UiThread.currentThreadIsUi())
      throw new AssertionError("Must be called from UI thread!");

    if (providers.length == 0)
    {
      if (mListener != null)
        mListener.onNoTaxiProviders();
      return;
    }

    mProviders.clear();
    mProviders.addAll(Arrays.asList(providers));

    if (mListener != null)
    {
      // Taxi provider list must contain only one element until we implement taxi aggregator feature.
      mListener.onTaxiProviderReceived(mProviders.get(0));
    }
  }

  // Called from JNI.
  @MainThread
  void onTaxiErrorsReceived(@NonNull TaxiInfoError[] errors)
  {
    if (!UiThread.currentThreadIsUi())
      throw new AssertionError("Must be called from UI thread!");

    if (errors.length == 0)
      throw new AssertionError("Taxi error array must be non-empty!");

    mErrors.clear();
    mErrors.addAll(Arrays.asList(errors));

    if (mListener != null)
    {
      // Taxi error list must contain only one element until we implement taxi aggregator feature.
      mListener.onTaxiErrorReceived(mErrors.get(0));
    }
  }

  @Nullable
  public static SponsoredLinks getTaxiLink(@NonNull String productId, @NonNull TaxiType type,
                                           @Nullable MapObject startPoint, @Nullable MapObject endPoint)
  {
    if (startPoint == null || endPoint == null)
      return null;

    return TaxiManager.INSTANCE.nativeGetTaxiLinks(NetworkPolicy.newInstance(true /* canUse */),
                                                   type.ordinal(), productId, startPoint.getLat(),
                                                   startPoint.getLon(), endPoint.getLat(),
                                                   endPoint.getLon());
  }

  public static void launchTaxiApp(@NonNull Context context, @NonNull SponsoredLinks links,
                                   @NonNull TaxiType type)
  {
    Utils.openPartner(context, links, type.getPackageName(), type.getOpenMode());

    boolean isTaxiInstalled = Utils.isAppInstalled(context, type.getPackageName());
    trackTaxiStatistics(type.getProviderName(), isTaxiInstalled);
  }

  private static void trackTaxiStatistics(@NonNull String taxiName, boolean isTaxiAppInstalled)
  {
    MapObject from = RoutingController.get().getStartPoint();
    MapObject to = RoutingController.get().getEndPoint();
    Location location = LocationHelper.INSTANCE.getLastKnownLocation();
    Statistics.INSTANCE.trackTaxiInRoutePlanning(from, to, location, taxiName, isTaxiAppInstalled);
  }
  public void setTaxiListener(@Nullable TaxiListener listener)
  {
    mListener = listener;
  }

  public native void nativeRequestTaxiProducts(@NonNull NetworkPolicy policy, double srcLat,
                                                      double srcLon, double dstLat, double dstLon);

  @NonNull
  public native SponsoredLinks nativeGetTaxiLinks(@NonNull NetworkPolicy policy, int type,
                                                  @NonNull String productId, double srcLon,
                                                  double srcLat, double dstLat, double dstLon);

  public enum ErrorCode
  {
    NoProducts, RemoteError, NoProviders
  }

  public interface TaxiListener
  {
    void onTaxiProviderReceived(@NonNull TaxiInfo provider);
    void onTaxiErrorReceived(@NonNull TaxiInfoError error);
    void onNoTaxiProviders();
  }
}
