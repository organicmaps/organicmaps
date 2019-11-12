package com.mapswithme.maps.purchase;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.cardview.widget.CardView;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

public class BookmarkSubscriptionFragment extends AbstractBookmarkSubscriptionFragment
{
  static final String EXTRA_FROM = "extra_from";

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

    TextView restorePurchasesBtn = root.findViewById(R.id.restore_purchase_btn);
    restorePurchasesBtn.setOnClickListener(v -> openSubscriptionManagementSettings());

    View continueBtn = root.findViewById(R.id.continue_btn);
    continueBtn.setOnClickListener(v -> onContinueButtonClicked());

    annualPriceCard.setSelected(true);
    monthlyPriceCard.setSelected(false);
    annualPriceCard.setCardElevation(getResources().getDimension(R.dimen.margin_base_plus_quarter));

    View termsOfUse = root.findViewById(R.id.term_of_use_link);
    termsOfUse.setOnClickListener(v -> Utils.openUrl(requireActivity(), Framework.nativeGetTermsOfUseLink()));
    View privacyPolicy = root.findViewById(R.id.privacy_policy_link);
    privacyPolicy.setOnClickListener(v -> Utils.openUrl(requireActivity(), Framework.nativeGetPrivacyPolicyLink()));

    Statistics.INSTANCE.trackPurchasePreviewShow(PrivateVariables.bookmarksSubscriptionServerId(),
                                                 PrivateVariables.bookmarksSubscriptionVendor(),
                                                 PrivateVariables.bookmarksSubscriptionYearlyProductId(),
                                                 getExtraFrom());
    return root;
  }

  @Override
  void onSubscriptionDestroyView()
  {
    // Do nothing by default.
  }

  @Nullable
  private String getExtraFrom()
  {
    if (getArguments() == null)
      return null;

    return getArguments().getString(EXTRA_FROM, null);
  }


  private void openSubscriptionManagementSettings()
  {
    Utils.openUrl(requireContext(), "https://play.google.com/store/account/subscriptions");
    Statistics.INSTANCE.trackPurchaseEvent(Statistics.EventName.INAPP_PURCHASE_PREVIEW_RESTORE,
                                           PrivateVariables.bookmarksSubscriptionServerId());
  }

  private void onContinueButtonClicked()
  {
    BookmarkManager.INSTANCE.pingBookmarkCatalog();
    activateState(BookmarkSubscriptionPaymentState.PINGING);

    Statistics.INSTANCE.trackPurchaseEvent(Statistics.EventName.INAPP_PURCHASE_PREVIEW_PAY,
                                           PrivateVariables.bookmarksSubscriptionServerId(),
                                           Statistics.STATISTICS_CHANNEL_REALTIME);
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

  @Override
  public boolean onBackPressed()
  {
    Statistics.INSTANCE.trackPurchaseEvent(Statistics.EventName.INAPP_PURCHASE_PREVIEW_CANCEL,
                                           PrivateVariables.bookmarksSubscriptionServerId());
    return super.onBackPressed();
  }

  private void launchPurchaseFlow()
  {
    CardView annualCard = getViewOrThrow().findViewById(R.id.annual_price_card);
    PurchaseUtils.Period period = annualCard.getCardElevation() > 0 ? PurchaseUtils.Period.P1Y
                                                                    : PurchaseUtils.Period.P1M;
    ProductDetails details = getProductDetailsForPeriod(period);
    launchPurchaseFlow(details.getProductId());
  }

  @Override
  void onAuthorizationFinishSuccessfully()
  {
    launchPurchaseFlow();
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

  @Override
  public void onValidating()
  {
    showButtonProgress();
  }

  @Override
  public void onValidationFinish()
  {
    super.onValidationFinish();
    hideButtonProgress();
  }

  @NonNull
  @Override
  PurchaseController<PurchaseCallback> createPurchaseController()
  {
    return PurchaseFactory.createBookmarksSubscriptionPurchaseController(requireContext());
  }

  @Override
  public void onPinging()
  {
    showButtonProgress();
  }

  @Override
  public void onPingFinish()
  {
    super.onPingFinish();
    hideButtonProgress();
  }

  private void showButtonProgress()
  {
    UiUtils.hide(getViewOrThrow(), R.id.continue_btn);
    UiUtils.show(getViewOrThrow(), R.id.progress);
  }

  private void hideButtonProgress()
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
        Statistics.INSTANCE.trackPurchasePreviewSelect(PrivateVariables.bookmarksSubscriptionServerId(),
                                                       PrivateVariables.bookmarksSubscriptionYearlyProductId());

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
        Statistics.INSTANCE.trackPurchasePreviewSelect(PrivateVariables.bookmarksSubscriptionServerId(),
                                                       PrivateVariables.bookmarksSubscriptionMonthlyProductId());

      mMonthlyPriceCard.setSelected(true);
      mAnnualPriceCard.setSelected(false);
    }
  }
}
