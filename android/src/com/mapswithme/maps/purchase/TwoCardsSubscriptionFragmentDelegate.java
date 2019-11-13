package com.mapswithme.maps.purchase;

import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.cardview.widget.CardView;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

public class TwoCardsSubscriptionFragmentDelegate extends SubscriptionFragmentDelegate
{
  private static final int DEF_ELEVATION = 0;

  TwoCardsSubscriptionFragmentDelegate(@NonNull AbstractBookmarkSubscriptionFragment fragment)
  {
    super(fragment);
  }

  @Override
  void onCreateView(@NonNull View root)
  {
    super.onCreateView(root);
    CardView annualPriceCard = root.findViewById(R.id.annual_price_card);
    CardView monthlyPriceCard = root.findViewById(R.id.monthly_price_card);
    AnnualCardClickListener annualCardListener = new AnnualCardClickListener(monthlyPriceCard,
                                                                                                                                       annualPriceCard);
    annualPriceCard.setOnClickListener(annualCardListener);
    MonthlyCardClickListener monthlyCardListener = new MonthlyCardClickListener(monthlyPriceCard,
                                                                                                                                          annualPriceCard);
    monthlyPriceCard.setOnClickListener(monthlyCardListener);

    View continueBtn = root.findViewById(R.id.continue_btn);
    continueBtn.setOnClickListener(v -> onContinueButtonClicked());

    annualPriceCard.setSelected(true);
    monthlyPriceCard.setSelected(false);
    annualPriceCard.setCardElevation(getFragment().getResources()
                                                  .getDimension(R.dimen.margin_base_plus_quarter));

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
    TextView priceView = getFragment().getViewOrThrow().findViewById(R.id.annual_price);
    priceView.setText(price);
    TextView savingView = getFragment().getViewOrThrow().findViewById(R.id.sale);
    String text = getFragment().getString(R.string.annual_save_component,
                                          getFragment().calculateYearlySaving());
    savingView.setText(text);
  }

  private void updateMonthlyButton()
  {
    ProductDetails details = getFragment().getProductDetailsForPeriod(PurchaseUtils.Period.P1M);
    String price = Utils.formatCurrencyString(details.getPrice(), details.getCurrencyCode());
    TextView priceView = getFragment().getViewOrThrow().findViewById(R.id.monthly_price);
    priceView.setText(price);
  }

  @NonNull
  @Override
  PurchaseUtils.Period getSelectedPeriod()
  {
    CardView annualCard = getFragment().getViewOrThrow().findViewById(R.id.annual_price_card);
    return annualCard.getCardElevation() > 0 ? PurchaseUtils.Period.P1Y : PurchaseUtils.Period.P1M;
  }

  @Override
  public void onReset()
  {
    hideAllUi();
  }

  private void hideAllUi()
  {
    UiUtils.hide(getFragment().getViewOrThrow(), R.id.root_screen_progress, R.id.content_view);
  }

  private void onContinueButtonClicked()
  {
    getFragment().pingBookmarkCatalog();
    getFragment().trackPayEvent();
  }

  @Override
  void showButtonProgress()
  {
    UiUtils.hide(getFragment().getViewOrThrow(), R.id.continue_btn);
    UiUtils.show(getFragment().getViewOrThrow(), R.id.progress);
  }

  @Override
  void hideButtonProgress()
  {
    UiUtils.hide(getFragment().getViewOrThrow(), R.id.progress);
    UiUtils.show(getFragment().getViewOrThrow(), R.id.continue_btn);
  }

  private void showRootScreenProgress()
  {
    UiUtils.show(getFragment().getViewOrThrow(), R.id.root_screen_progress);
    UiUtils.hide(getFragment().getViewOrThrow(), R.id.content_view);
  }

  private void hideRootScreenProgress()
  {
    UiUtils.hide(getFragment().getViewOrThrow(), R.id.root_screen_progress);
    UiUtils.show(getFragment().getViewOrThrow(), R.id.content_view);
  }

  @Override
  void onProductDetailsLoading()
  {
    showRootScreenProgress();
  }

  @Override
  void onPriceSelection()
  {
    hideRootScreenProgress();
    updatePaymentButtons();
  }

  private class AnnualCardClickListener implements View.OnClickListener
  {
    @NonNull
    private final CardView mMonthlyPriceCard;

    @NonNull
    private final CardView mAnnualPriceCard;

    AnnualCardClickListener(@NonNull CardView monthlyPriceCard,
                            @NonNull CardView annualPriceCard)
    {
      mMonthlyPriceCard = monthlyPriceCard;
      mAnnualPriceCard = annualPriceCard;
    }

    @Override
    public void onClick(View v)
    {
      mMonthlyPriceCard.setCardElevation(DEF_ELEVATION);
      mAnnualPriceCard.setCardElevation(getFragment().getResources()
                                                     .getDimension(R.dimen.margin_base_plus_quarter));

      if (!mAnnualPriceCard.isSelected())
        getFragment().trackYearlyProductSelected();

      mMonthlyPriceCard.setSelected(false);
      mAnnualPriceCard.setSelected(true);
    }
  }

  private class MonthlyCardClickListener implements View.OnClickListener
  {
    @NonNull
    private final CardView mMonthlyPriceCard;

    @NonNull
    private final CardView mAnnualPriceCard;

    MonthlyCardClickListener(@NonNull CardView monthlyPriceCard,
                             @NonNull CardView annualPriceCard)
    {
      mMonthlyPriceCard = monthlyPriceCard;
      mAnnualPriceCard = annualPriceCard;
    }

    @Override
    public void onClick(View v)
    {
      mMonthlyPriceCard.setCardElevation(getFragment().getResources()
                                                      .getDimension(R.dimen.margin_base_plus_quarter));
      mAnnualPriceCard.setCardElevation(DEF_ELEVATION);

      if (!mMonthlyPriceCard.isSelected())
        getFragment().trackMonthlyProductSelected();

      mMonthlyPriceCard.setSelected(true);
      mAnnualPriceCard.setSelected(false);
    }
  }
}
