package com.mapswithme.maps;

import android.content.Context;
import android.util.Base64;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.base.Initializable;
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
                                                    Framework.StartTransactionListener,
                                                    Initializable<Void>
{
  private static final String TAG = PurchaseOperationObservable.class.getSimpleName();
  @NonNull
  private final Map<String, CoreValidationObserver> mValidationObservers = new HashMap<>();
  @NonNull
  private final List<CoreStartTransactionObserver> mTransactionObservers = new ArrayList<>();
  @NonNull
  private final Map<String, PendingResult> mValidationPendingResults = new HashMap<>();
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

  @Override
  public void initialize(@Nullable Void aVoid)
  {
    mLogger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
    mLogger.i(TAG, "Initializing purchase operation observable...");
    Framework.nativeSetPurchaseValidationListener(this);
    Framework.nativeStartPurchaseTransactionListener(this);
  }

  @Override
  public void destroy()
  {
    // No op.
  }

  @Override
  public void onValidatePurchase(int code, @NonNull String serverId, @NonNull String vendorId,
                                 @NonNull String encodedPurchaseData, boolean isTrial)
  {
    byte[] tokenBytes = Base64.decode(encodedPurchaseData, Base64.DEFAULT);
    String purchaseData = new String(tokenBytes);
    String orderId = PurchaseUtils.parseOrderId(purchaseData);
    CoreValidationObserver observer = mValidationObservers.get(orderId);
    ValidationStatus status = ValidationStatus.values()[code];
    if (observer == null)
    {
      PendingResult result = new PendingResult(status, serverId, vendorId, purchaseData, isTrial);
      mValidationPendingResults.put(orderId, result);
      return;
    }

    observer.onValidatePurchase(status, serverId, vendorId, purchaseData, isTrial);
  }

  @Override
  public void onStartTransaction(boolean success, @NonNull String serverId, @NonNull String
      vendorId)
  {
    for(CoreStartTransactionObserver observer: mTransactionObservers)
      observer.onStartTransaction(success, serverId, vendorId);
  }

  public void addValidationObserver(@NonNull String orderId,
                                    @NonNull CoreValidationObserver observer)
  {
    mLogger.d(TAG, "Add validation observer '" + observer + "' for '" + orderId + "'");
    mValidationObservers.put(orderId, observer);
    PendingResult result = mValidationPendingResults.remove(orderId);
    if (result != null)
    {
      mLogger.d(TAG, "Post pending validation result to '" + observer + "' for '"
                     + orderId + "'");
      observer.onValidatePurchase(result.getStatus(), result.getServerId(), result.getVendorId(),
                                  result.getPurchaseData(), result.isTrial());
    }
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

  private static class PendingResult
  {
    @NonNull
    private final ValidationStatus mStatus;
    @NonNull
    private final String mServerId;
    @NonNull
    private final String mVendorId;
    @NonNull
    private final String mPurchaseData;
    private final boolean mIsTrial;

    private PendingResult(@NonNull ValidationStatus status, @NonNull String serverId,
                          @NonNull String vendorId, @NonNull String purchaseData, boolean isTrial)
    {
      mStatus = status;
      mServerId = serverId;
      mVendorId = vendorId;
      mPurchaseData = purchaseData;
      mIsTrial = isTrial;
    }

    @NonNull
    ValidationStatus getStatus()
    {
      return mStatus;
    }

    @NonNull
    String getServerId()
    {
      return mServerId;
    }

    @NonNull
    String getVendorId()
    {
      return mVendorId;
    }

    @NonNull
    String getPurchaseData()
    {
      return mPurchaseData;
    }

    boolean isTrial()
    {
      return mIsTrial;
    }
  }
}
