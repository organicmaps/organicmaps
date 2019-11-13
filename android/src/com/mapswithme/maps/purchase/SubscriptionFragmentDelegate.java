package com.mapswithme.maps.purchase;

import android.view.View;

import androidx.annotation.NonNull;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.SubscriptionButton;
import com.mapswithme.util.Utils;

class SubscriptionFragmentDelegate
{
  @SuppressWarnings("NullableProblems")
  @NonNull
  private SubscriptionButton mAnnualButton;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private SubscriptionButton mMonthlyButton;
  @NonNull
  private PurchaseUtils.Period mSelectedPeriod = PurchaseUtils.Period.P1Y;
  @NonNull
  private final AbstractBookmarkSubscriptionFragment mFragment;

  SubscriptionFragmentDelegate(@NonNull AbstractBookmarkSubscriptionFragment fragment)
  {
    mFragment = fragment;
  }

  void onSubscriptionCreateView(@NonNull View root)
  {
    mAnnualButton = root.findViewById(R.id.annual_button);
    mAnnualButton.setOnClickListener(v -> {
      mSelectedPeriod = PurchaseUtils.Period.P1Y;
      mFragment.pingBookmarkCatalog();
    });
    mMonthlyButton = root.findViewById(R.id.monthly_button);
    mMonthlyButton.setOnClickListener(v -> {
      mSelectedPeriod = PurchaseUtils.Period.P1M;
      mFragment.pingBookmarkCatalog();
    });
  }

  void onSubscriptionDestroyView()
  {
    // Do nothing by default.
  }

  @NonNull
  PurchaseUtils.Period getSelectedPeriod()
  {
    return mSelectedPeriod;
  }

  void onProductDetailsLoading()
  {
    mAnnualButton.showProgress();
    mMonthlyButton.showProgress();
  }

  void onPriceSelection()
  {
    mAnnualButton.hideProgress();
    mMonthlyButton.hideProgress();
    updatePaymentButtons();
  }

  void onReset()
  {
    // Do nothing by default.
  }

  private void updatePaymentButtons()
  {
    updateYearlyButton();
    updateMonthlyButton();
  }

  private void updateMonthlyButton()
  {
    ProductDetails details = mFragment.getProductDetailsForPeriod(PurchaseUtils.Period.P1Y);
    String price = Utils.formatCurrencyString(details.getPrice(), details.getCurrencyCode());
    mAnnualButton.setPrice(price);
    mAnnualButton.setName(mFragment.getString(R.string.annual_subscription_title));
    String sale = mFragment.getString(R.string.annual_save_component, mFragment.calculateYearlySaving());
    mAnnualButton.setSale(sale);
  }

  private void updateYearlyButton()
  {
    ProductDetails details = mFragment.getProductDetailsForPeriod(PurchaseUtils.Period.P1M);
    String price = Utils.formatCurrencyString(details.getPrice(), details.getCurrencyCode());
    mMonthlyButton.setPrice(price);
    mMonthlyButton.setName(mFragment.getString(R.string.montly_subscription_title));
  }

  void showButtonProgress()
  {
    if (mSelectedPeriod == PurchaseUtils.Period.P1Y)
      mAnnualButton.showProgress();
    else
      mMonthlyButton.showProgress();
  }

  void hideButtonProgress()
  {
    if (mSelectedPeriod == PurchaseUtils.Period.P1Y)
      mAnnualButton.hideProgress();
    else
      mMonthlyButton.hideProgress();
  }
}
