package com.mapswithme.maps.purchase;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.cardview.widget.CardView;
import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

public class BookmarkSubscriptionFragment extends AbstractBookmarkSubscriptionFragment
{
  private static final int DEF_ELEVATION = 0;

  @Nullable
  @Override
  View onSubscriptionCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                                @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.bookmark_subscription_fragment, container, false);
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
    annualPriceCard.setCardElevation(getResources().getDimension(R.dimen.margin_base_plus_quarter));

    return root;
  }

  @Override
  void onSubscriptionDestroyView()
  {
    // Do nothing by default.
  }

  @NonNull
  @Override
  SubscriptionType getSubscriptionType()
  {
    return SubscriptionType.BOOKMARKS;
  }

  private void onContinueButtonClicked()
  {
    pingBookmarkCatalog();
    trackPayEvent();
  }

  private void updatePaymentButtons()
  {
    updateYearlyButton();
    updateMonthlyButton();
  }

  private void updateYearlyButton()
  {
    ProductDetails details = getProductDetailsForPeriod(PurchaseUtils.Period.P1Y);
    String price = Utils.formatCurrencyString(details.getPrice(), details.getCurrencyCode());
    TextView priceView = getViewOrThrow().findViewById(R.id.annual_price);
    priceView.setText(price);
    TextView savingView = getViewOrThrow().findViewById(R.id.sale);
    String text = getString(R.string.annual_save_component, calculateYearlySaving());
    savingView.setText(text);
  }

  private void updateMonthlyButton()
  {
    ProductDetails details = getProductDetailsForPeriod(PurchaseUtils.Period.P1M);
    String price = Utils.formatCurrencyString(details.getPrice(), details.getCurrencyCode());
    TextView priceView = getViewOrThrow().findViewById(R.id.monthly_price);
    priceView.setText(price);
  }

  @NonNull
  @Override
  PurchaseUtils.Period getSelectedPeriod()
  {
    CardView annualCard = getViewOrThrow().findViewById(R.id.annual_price_card);
    return annualCard.getCardElevation() > 0 ? PurchaseUtils.Period.P1Y : PurchaseUtils.Period.P1M;
  }

  @Override
  public void onReset()
  {
    hideAllUi();
  }

  @Override
  public void onProductDetailsLoading()
  {
    super.onProductDetailsLoading();
    showRootScreenProgress();
  }

  @Override
  public void onPriceSelection()
  {
    hideRootScreenProgress();
    updatePaymentButtons();
  }

  @NonNull
  @Override
  PurchaseController<PurchaseCallback> createPurchaseController()
  {
    return PurchaseFactory.createBookmarksSubscriptionPurchaseController(requireContext());
  }

  @Override
  void showButtonProgress()
  {
    UiUtils.hide(getViewOrThrow(), R.id.continue_btn);
    UiUtils.show(getViewOrThrow(), R.id.progress);
  }

  @Override
  void hideButtonProgress()
  {
    UiUtils.hide(getViewOrThrow(), R.id.progress);
    UiUtils.show(getViewOrThrow(), R.id.continue_btn);
  }

  private void showRootScreenProgress()
  {
    UiUtils.show(getViewOrThrow(), R.id.root_screen_progress);
    UiUtils.hide(getViewOrThrow(), R.id.content_view);
  }

  private void hideRootScreenProgress()
  {
    UiUtils.hide(getViewOrThrow(), R.id.root_screen_progress);
    UiUtils.show(getViewOrThrow(), R.id.content_view);
  }

  private void hideAllUi()
  {
    UiUtils.hide(getViewOrThrow(), R.id.root_screen_progress, R.id.content_view);
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
      mAnnualPriceCard.setCardElevation(getResources().getDimension(R.dimen.margin_base_plus_quarter));

      if (!mAnnualPriceCard.isSelected())
      {
        trackYearlyProductSelected();
      }

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
      mMonthlyPriceCard.setCardElevation(getResources().getDimension(R.dimen.margin_base_plus_quarter));
      mAnnualPriceCard.setCardElevation(DEF_ELEVATION);

      if (!mMonthlyPriceCard.isSelected())
        trackMonthlyProductSelected();

      mMonthlyPriceCard.setSelected(true);
      mAnnualPriceCard.setSelected(false);
    }
  }
}
