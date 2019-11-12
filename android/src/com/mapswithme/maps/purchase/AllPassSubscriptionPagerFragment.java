package com.mapswithme.maps.purchase;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentPagerAdapter;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.DotPager;
import com.mapswithme.maps.widget.ParallaxBackgroundPageListener;
import com.mapswithme.maps.widget.ParallaxBackgroundViewPager;
import com.mapswithme.maps.widget.SubscriptionButton;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

import java.util.ArrayList;
import java.util.List;

public class AllPassSubscriptionPagerFragment extends AbstractBookmarkSubscriptionFragment
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
  @Override
  PurchaseController<PurchaseCallback> createPurchaseController()
  {
    return PurchaseFactory.createBookmarksSubscriptionPurchaseController(requireContext());
  }

  @Nullable
  @Override
  View onSubscriptionCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                                @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.pager_fragment_all_pass_subscription, container,
                                 false);

    mAnnualButton = root.findViewById(R.id.annual_sub_btn);
    mAnnualButton.setOnClickListener(v -> {
      mSelectedPeriod = PurchaseUtils.Period.P1Y;
      pingBookmarkCatalog();
    });
    mMonthlyButton = root.findViewById(R.id.month_sub_btn);
    mMonthlyButton.setOnClickListener(v -> {
      mSelectedPeriod = PurchaseUtils.Period.P1M;
      pingBookmarkCatalog();
    });

    setTopStatusBarOffset(root);
    initViewPager(root);
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

  @NonNull
  @Override
  PurchaseUtils.Period getSelectedPeriod()
  {
    return mSelectedPeriod;
  }

  private void setTopStatusBarOffset(@NonNull View view)
  {
    View statusBarPlaceholder = view.findViewById(R.id.status_bar_placeholder);
    ViewGroup.LayoutParams params = statusBarPlaceholder.getLayoutParams();
    int statusBarHeight = UiUtils.getStatusBarHeight(requireContext());
    params.height = statusBarHeight;
    statusBarPlaceholder.setLayoutParams(params);
    View header = view.findViewById(R.id.header);
    ViewGroup.MarginLayoutParams headerParams = (ViewGroup.MarginLayoutParams) header.getLayoutParams();
    headerParams.topMargin  = Math.max(0, headerParams.topMargin - statusBarHeight);
    header.setLayoutParams(headerParams);
  }

  private void initViewPager(@NonNull View root)
  {
    final List<Integer> items = makeItems();

    ParallaxBackgroundViewPager viewPager = root.findViewById(R.id.pager);
    PagerAdapter adapter = new ParallaxFragmentPagerAdapter(requireFragmentManager(),
                                                            items);
    DotPager pager = makeDotPager(root.findViewById(R.id.indicator), viewPager, adapter);
    pager.show();
    ViewPager.OnPageChangeListener listener = new ParallaxBackgroundPageListener(requireActivity(),
                                                                                 viewPager, items);
    viewPager.addOnPageChangeListener(listener);
    viewPager.startAutoScroll();
  }

  @NonNull
  private DotPager makeDotPager(@NonNull ViewGroup indicatorContainer, @NonNull ViewPager viewPager,
                                @NonNull PagerAdapter adapter)
  {
    return new DotPager.Builder(requireContext(), viewPager, adapter)
        .setIndicatorContainer(indicatorContainer)
        .setActiveDotDrawable(R.drawable.all_pass_marker_active)
        .setInactiveDotDrawable(R.drawable.all_pass_marker_inactive)
        .build();
  }

  @NonNull
  private static List<Integer> makeItems()
  {
    List<Integer> items = new ArrayList<>();
    items.add(R.id.img3);
    items.add(R.id.img2);
    items.add(R.id.img1);
    return items;
  }

  @Override
  public void onProductDetailsLoading()
  {
    super.onProductDetailsLoading();
    mAnnualButton.showProgress();
    mMonthlyButton.showProgress();
  }

  @Override
  public void onReset()
  {
    // Do nothing
  }

  @Override
  public void onPriceSelection()
  {
    mAnnualButton.hideProgress();
    mMonthlyButton.hideProgress();
    updatePaymentButtons();
  }

  @Override
  void showButtonProgress()
  {
    if (mSelectedPeriod == PurchaseUtils.Period.P1Y)
      mAnnualButton.showProgress();
    else
      mMonthlyButton.showProgress();
  }

  @Override
  void hideButtonProgress()
  {
    if (mSelectedPeriod == PurchaseUtils.Period.P1Y)
      mAnnualButton.hideProgress();
    else
      mMonthlyButton.hideProgress();
  }

  private void updatePaymentButtons()
  {
    updateYearlyButton();
    updateMonthlyButton();
  }

  private void updateMonthlyButton()
  {
    ProductDetails details = getProductDetailsForPeriod(PurchaseUtils.Period.P1Y);
    String price = Utils.formatCurrencyString(details.getPrice(), details.getCurrencyCode());
    mAnnualButton.setPrice(price);
    mAnnualButton.setName(getString(R.string.annual_subscription_title));
    String sale = getString(R.string.annual_save_component, calculateYearlySaving());
    mAnnualButton.setSale(sale);
  }

  private void updateYearlyButton()
  {
    ProductDetails details = getProductDetailsForPeriod(PurchaseUtils.Period.P1M);
    String price = Utils.formatCurrencyString(details.getPrice(), details.getCurrencyCode());
    mMonthlyButton.setPrice(price);
    mMonthlyButton.setName(getString(R.string.montly_subscription_title));
  }

  private class ParallaxFragmentPagerAdapter extends FragmentPagerAdapter
  {
    @NonNull
    private final List<Integer> mItems;

    ParallaxFragmentPagerAdapter(@NonNull FragmentManager fragmentManager,
                                 @NonNull List<Integer> items)
    {
      super(fragmentManager);
      mItems = items;
    }

    @Override
    public Fragment getItem(int i)
    {
      return AllPassSubscriptionFragment.newInstance(i);
    }

    @Override
    public int getCount()
    {
      return mItems.size();
    }
  }
}
