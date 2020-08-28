package com.mapswithme.maps.purchase;

import android.text.TextUtils;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.SubscriptionButton;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

import java.util.List;

class TwoButtonsSubscriptionFragmentDelegate extends SubscriptionFragmentDelegate
{
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private SubscriptionButton mYearlyButton;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private SubscriptionButton mMonthlyButton;
  @NonNull
  private PurchaseUtils.Period mSelectedPeriod = PurchaseUtils.Period.P1Y;
  @Nullable
  private View mFreeTrialButton;
  @Nullable
  private TextView mFreeTrialMessage;

  TwoButtonsSubscriptionFragmentDelegate(@NonNull AbstractBookmarkSubscriptionFragment fragment)
  {
    super(fragment);
  }

  @Override
  @CallSuper
  public void onCreateView(@NonNull View root)
  {
    super.onCreateView(root);
    mYearlyButton = root.findViewById(R.id.annual_button);
    mYearlyButton.setOnClickListener(v -> onSubscriptionButtonClicked(PurchaseUtils.Period.P1Y));
    mMonthlyButton = root.findViewById(R.id.monthly_button);
    mMonthlyButton.setOnClickListener(v -> onSubscriptionButtonClicked(PurchaseUtils.Period.P1M));
    mFreeTrialButton = root.findViewById(R.id.free_trial_button);
    if (mFreeTrialButton != null)
      mFreeTrialButton.setOnClickListener(v -> onSubscriptionButtonClicked(PurchaseUtils.Period.P1Y));
    mFreeTrialMessage = root.findViewById(R.id.free_trial_mesage);
  }

  private void onSubscriptionButtonClicked(@NonNull PurchaseUtils.Period period)
  {
    List<ProductDetails> productDetails = getFragment().getProductDetails();
    if (productDetails == null || productDetails.isEmpty())
      return;

    mSelectedPeriod = period;
    if (mSelectedPeriod == PurchaseUtils.Period.P1Y)
      getFragment().trackYearlyProductSelected();
    else if (mSelectedPeriod == PurchaseUtils.Period.P1M)
      getFragment().trackMonthlyProductSelected();
    getFragment().pingBookmarkCatalog();
  }

  @NonNull
  @Override
  PurchaseUtils.Period getSelectedPeriod()
  {
    return mSelectedPeriod;
  }

  @Override
  void onProductDetailsLoading()
  {
    mYearlyButton.showProgress();
    mMonthlyButton.showProgress();
  }

  @Override
  void onPriceSelection()
  {
    mYearlyButton.hideProgress();
    mMonthlyButton.hideProgress();
    updatePaymentButtons();
  }

  private void updatePaymentButtons()
  {
    ProductDetails details = getFragment().getProductDetailsForPeriod(PurchaseUtils.Period.P1Y);
    if (!TextUtils.isEmpty(details.getFreeTrialPeriod()))
    {
      UiUtils.hide(mYearlyButton, mMonthlyButton);
      if (mFreeTrialButton != null && mFreeTrialMessage != null)
      {
        UiUtils.show(mFreeTrialButton, mFreeTrialMessage);
        String price = Utils.formatCurrencyString(details.getPrice(), details.getCurrencyCode());
        mFreeTrialMessage.setText(getFragment().getString(R.string.guides_trial_message, price));
      }
      return;
    }
    updateYearlyButton();
    updateMonthlyButton();
  }

  private void updateYearlyButton()
  {
    ProductDetails details = getFragment().getProductDetailsForPeriod(PurchaseUtils.Period.P1Y);
    String price = Utils.formatCurrencyString(details.getPrice(), details.getCurrencyCode());
    mYearlyButton.setPrice(price);
    mYearlyButton.setName(getFragment().getString(R.string.annual_subscription_title));
    mYearlyButton.setSale(getFragment().getString(R.string.all_pass_screen_best_value));
  }

  private void updateMonthlyButton()
  {
    ProductDetails details = getFragment().getProductDetailsForPeriod(PurchaseUtils.Period.P1M);
    String price = Utils.formatCurrencyString(details.getPrice(), details.getCurrencyCode());
    mMonthlyButton.setPrice(price);
    mMonthlyButton.setName(getFragment().getString(R.string.montly_subscription_title));
  }

  @Override
  void showButtonProgress()
  {
    if (mSelectedPeriod == PurchaseUtils.Period.P1Y)
      mYearlyButton.showProgress();
    else
      mMonthlyButton.showProgress();
  }

  @Override
  void hideButtonProgress()
  {
    if (mSelectedPeriod == PurchaseUtils.Period.P1Y)
      mYearlyButton.hideProgress();
    else
      mMonthlyButton.hideProgress();
  }
}
