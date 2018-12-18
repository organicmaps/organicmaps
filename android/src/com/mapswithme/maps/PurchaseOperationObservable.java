package com.mapswithme.maps;

import android.content.Context;
import android.support.annotation.NonNull;
import android.util.Base64;

import com.mapswithme.maps.purchase.CoreStartTransactionObserver;
import com.mapswithme.maps.purchase.CoreValidationObserver;
import com.mapswithme.maps.purchase.PurchaseUtils;
import com.mapswithme.maps.purchase.ValidationStatus;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class PurchaseOperationObservable implements Framework.PurchaseValidationListener,
                                                    Framework.StartTransactionListener
{
  private static final String TAG = PurchaseOperationObservable.class.getSimpleName();
  @NonNull
  private final Map<String, CoreValidationObserver> mValidationObservers = new HashMap<>();
  @NonNull
  private final List<CoreStartTransactionObserver> mTransactionObservers = new ArrayList<>();
  @SuppressWarnings("NullableProblems")
  @NonNull
  private Logger mLogger;

  PurchaseOperationObservable()
  {
    // Do nothing by default.
  }

  @NonNull
  public static PurchaseOperationObservable from(@NonNull Context context)
  {
    MwmApplication application = (MwmApplication) context.getApplicationContext();
    return application.getPurchaseOperationObservable();
  }

  public void initialize()
  {
    mLogger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
    mLogger.i(TAG, "Initializing purchase operation observable...");
    Framework.nativeSetPurchaseValidationListener(this);
    Framework.nativeStartPurchaseTransactionListener(this);
  }

  @Override
  public void onValidatePurchase(int code, @NonNull String serverId, @NonNull String vendorId,
                                 @NonNull String encodedPurchaseData)
  {
    byte[] tokenBytes = Base64.decode(encodedPurchaseData, Base64.DEFAULT);
    String purchaseData = new String(tokenBytes);
    String orderId = PurchaseUtils.parseOrderId(purchaseData);
    CoreValidationObserver observer = mValidationObservers.get(orderId);
    if (observer == null)
      return;

    observer.onValidatePurchase(ValidationStatus.values()[code], serverId, vendorId, purchaseData);
  }

  @Override
  public void onStartTransaction(boolean success, @NonNull String serverId, @NonNull String
      vendorId)
  {
    for(CoreStartTransactionObserver observer: mTransactionObservers)
      observer.onStartTransaction(success, serverId, vendorId);
  }

  public void addValidationObserver(@NonNull String orderId, @NonNull CoreValidationObserver observer)
  {
    mLogger.d(TAG, "Add validation observer '" + observer + "' for '" + orderId + "'");
    mValidationObservers.put(orderId, observer);
  }

  public void removeValidationObserver(@NonNull String orderId)
  {
    mLogger.d(TAG, "Remove validation observer for '" + orderId + "'");
    mValidationObservers.remove(orderId);
  }

  public void addTransactionObserver(@NonNull CoreStartTransactionObserver observer)
  {
    mLogger.d(TAG, "Add transaction observer '" + observer + "'");
    mTransactionObservers.add(observer);
  }

  public void removeTransactionObserver(@NonNull CoreStartTransactionObserver observer)
  {
    mLogger.d(TAG, "Remove transaction observer '" + observer + "'");
    mTransactionObservers.remove(observer);
  }
}
