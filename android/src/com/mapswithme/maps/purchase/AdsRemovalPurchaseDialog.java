package com.mapswithme.maps.purchase;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.billingclient.api.SkuDetails;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.Collections;
import java.util.List;

public class AdsRemovalPurchaseDialog extends BaseMwmDialogFragment
{
  private final static Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private final static String TAG = AdsRemovalPurchaseDialog.class.getSimpleName();
  private final static String EXTRA_CURRENT_STATE = "extra_current_state";
  @Nullable
  private List<SkuDetails> mDetails;
  @NonNull
  private State mState = State.NONE;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PurchaseController<AdsRemovalPurchaseCallback> mController;
  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    LOGGER.d(TAG, "onCreate savedInstanceState = " + savedInstanceState);
  }

  @Override
  public void onAttach(Context context)
  {
    super.onAttach(context);
    LOGGER.d(TAG, "onAttach");
    mController = ((AdsRemovalPurchaseControllerProvider) context).getAdsRemovalPurchaseController();
    mController.addCallback(new PurchaseCallback());
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    LOGGER.d(TAG, "onCreateView savedInstanceState = " + savedInstanceState + "this " + this);
    View view = inflater.inflate(R.layout.fragment_ads_removal_purchase_dialog, container, false);
    return view;
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    LOGGER.d(TAG, "onViewCreated savedInstanceState = " + savedInstanceState);
    if (savedInstanceState != null)
    {
      State savedState = State.values()[savedInstanceState.getInt(EXTRA_CURRENT_STATE)];
      activateState(savedState);
    }
    else
    {
      activateState(State.LOADING);
      mController.queryPurchaseDetails();
    }
  }

  private void activateState(@NonNull State state)
  {
    mState = state;
    mState.activate(getViewOrThrow());
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);
    LOGGER.d(TAG, "onSaveInstanceState");
    outState.putInt(EXTRA_CURRENT_STATE, mState.ordinal());
  }

  @Override
  public void onDetach()
  {
    LOGGER.d(TAG, "onDetach");
    super.onDetach();
    mController.removeCallback();
  }

  public enum State
  {
    NONE
        {
          @Override
          void activate(@NonNull View view)
          {
            throw new UnsupportedOperationException("This state can't be used!");
          }
        },
    LOADING
        {
          @Override
          void activate(@NonNull View view)
          {
            UiUtils.show(view, R.id.progress_layout);
          }
        },
    PRICE_SELECTION
        {
          @Override
          void activate(@NonNull View view)
          {
            UiUtils.hide(view, R.id.progress_layout);
          }
        },
    EXPLANATION
        {
          @Override
          void activate(@NonNull View view)
          {

          }
        },
    ERROR
        {
          @Override
          void activate(@NonNull View view)
          {

          }
        };

    abstract void activate(@NonNull View view);
  }

  private class PurchaseCallback implements AdsRemovalPurchaseCallback
  {
    @Override
    public void onProductDetailsLoaded(@NonNull List<SkuDetails> details)
    {
      mDetails = Collections.unmodifiableList(details);
      activateState(State.PRICE_SELECTION);
    }

    @Override
    public void onFailure()
    {
      // Coming soon.
    }
  }
}
