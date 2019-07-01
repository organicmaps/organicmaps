package com.mapswithme.maps.purchase;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.CardView;
import android.text.Html;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;

public class BookmarkSubscriptionFragment extends BaseMwmFragment
{
  private static final int DEF_ELEVATION = 0;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mContentView;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mRootScreenProgress;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.bookmark_subscription_fragment, container, false);
    mContentView = root.findViewById(R.id.content_view);
    mRootScreenProgress = root.findViewById(R.id.root_screen_progress);
    CardView annualPriceCard = root.findViewById(R.id.annual_price_card);
    CardView monthlyPriceCard = root.findViewById(R.id.monthly_price_card);
    AnnualCardClickListener annualCardListener = new AnnualCardClickListener(monthlyPriceCard,
                                                                             annualPriceCard);
    annualPriceCard.setOnClickListener(annualCardListener);
    MonthlyCardClickListener monthlyCardListener = new MonthlyCardClickListener(monthlyPriceCard,
                                                                                annualPriceCard);
    monthlyPriceCard.setOnClickListener(monthlyCardListener);
    annualPriceCard.setCardElevation(getResources().getDimension(R.dimen.margin_base_plus_quarter));
    TextView restorePurchasesLink = root.findViewById(R.id.restore_purchase_btn);

    final Spanned html = makeRestorePurchaseHtml(requireContext());
    restorePurchasesLink.setText(html);
    restorePurchasesLink.setMovementMethod(LinkMovementMethod.getInstance());
    return root;
  }

  private void showContent()
  {
    mContentView.setVisibility(View.VISIBLE);
    mRootScreenProgress.setVisibility(View.GONE);
  }

  private static Spanned makeRestorePurchaseHtml(@NonNull Context context)
  {
    final String restorePurchaseLink = "";
    return Html.fromHtml(context.getString(R.string.restore_purchase_link,
                                           restorePurchaseLink));
  }

  private class AnnualCardClickListener implements View.OnClickListener
  {
    @NonNull
    private final CardView mMonthlyPriceCard;

    @NonNull
    private final CardView mAnnualPriceCard;

    public AnnualCardClickListener(@NonNull CardView monthlyPriceCard,
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
    }
  }

  private class MonthlyCardClickListener implements View.OnClickListener
  {
    @NonNull
    private final CardView mMonthlyPriceCard;

    @NonNull
    private final CardView mAnnualPriceCard;

    public MonthlyCardClickListener(@NonNull CardView monthlyPriceCard,
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
    }
  }
}
