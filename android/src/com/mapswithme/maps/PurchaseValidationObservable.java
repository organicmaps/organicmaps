package com.mapswithme.maps;

import android.content.Context;
import android.support.annotation.NonNull;
import android.util.Base64;

import com.mapswithme.maps.purchase.CoreValidationObserver;
import com.mapswithme.maps.purchase.PurchaseUtils;
import com.mapswithme.maps.purchase.ValidationStatus;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.HashMap;
import java.util.Map;

public class PurchaseValidationObservable implements Framework.PurchaseValidationListener
{
  private static final String TAG = PurchaseValidationObservable.class.getSimpleName();
  @NonNull
  private final Map<String, CoreValidationObserver> mObservers = new HashMap<>();
  @SuppressWarnings("NullableProblems")
  @NonNull
  private Logger mLogger;

  PurchaseValidationObservable()
  {
    // Do nothing by default.
  }

  @NonNull
  public static PurchaseValidationObservable from(@NonNull Context context)
  {
    MwmApplication application = (MwmApplication) context.getApplicationContext();
    return application.getPurchaseValidationObservable();
  }

  public void initialize()
  {
    mLogger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
    mLogger.i(TAG, "Initializing purchase validation observable...");
    Framework.nativeSetPurchaseValidationListener(this);
  }

  @Override
  public void onValidatePurchase(int code, @NonNull String serverId, @NonNull String vendorId,
                                 @NonNull String encodedPurchaseData)
  {
    byte[] tokenBytes = Base64.decode(encodedPurchaseData, Base64.DEFAULT);
    String purchaseData = new String(tokenBytes);
    String orderId = PurchaseUtils.parseOrderId(purchaseData);
    CoreValidationObserver observer = mObservers.get(orderId);
    if (observer == null)
      return;

    observer.onValidatePurchase(ValidationStatus.values()[code], serverId, vendorId, purchaseData);
  }

  public void addObserver(@NonNull String orderId, @NonNull CoreValidationObserver observer)
  {
    mLogger.d(TAG, "Add validation observer '" + observer + "' for '" + orderId + "'");
    mObservers.put(orderId, observer);
  }

  public void removeObserver(@NonNull String orderId)
  {
    mLogger.d(TAG, "Remove validation observer for '" + orderId + "'");
    mObservers.remove(orderId);
  }
}
