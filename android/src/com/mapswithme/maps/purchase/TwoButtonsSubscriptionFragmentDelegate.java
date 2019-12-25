package com.mapswithme.maps.purchase;

import android.view.View;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.SubscriptionButton;
import com.mapswithme.util.Utils;

import java.util.List;

class TwoButtonsSubscriptionFragmentDelegate extends SubscriptionFragmentDelegate
{
  @SuppressWarnings("NullableProblems")
  @NonNull
  private SubscriptionButton mYearlyButton;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private SubscriptionButton mMonthlyButton;
  @NonNull
  private PurchaseUtils.Period mSelectedPeriod = PurchaseUtils.Period.P1Y;

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
    mYearlyButton.setOnClickListener(v -> {
      onSubscriptionButtonClicked(PurchaseUtils.Period.P1Y);
    });
    mMonthlyButton = root.findViewById(R.id.monthly_button);
    mMonthlyButton.setOnClickListener(v -> {
      onSubscriptionButtonClicked(PurchaseUtils.Period.P1M);
    });
  }

  private void onSubscriptionButtonClicked(@NonNull PurchaseUtils.Period period)
  {
    List<ProductDetails> productDetails = getFragment().getProductDetails();
    if (productDetails == null || productDetails.isEmpty())
      return;

    mSelectedPeriod = period;
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
