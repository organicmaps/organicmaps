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
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.List;

public class AllPassSubscriptionPagerFragment extends AbstractBookmarkSubscriptionFragment
{
  @SuppressWarnings("NullableProblems")
  @NonNull
  private SubscriptionFragmentDelegate mDelegate;

  @NonNull
  @Override
  PurchaseController<PurchaseCallback> createPurchaseController()
  {
    return PurchaseFactory.createBookmarksAllSubscriptionController(requireContext());
  }

  @Nullable
  @Override
  View onSubscriptionCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                                @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.pager_fragment_all_pass_subscription, container,
                                 false);
    mDelegate = new SubscriptionFragmentDelegate(this);
    mDelegate.onSubscriptionCreateView(root);

    setTopStatusBarOffset(root);
    initViewPager(root);
    return root;
  }

  @Override
  void onSubscriptionDestroyView()
  {
    mDelegate.onSubscriptionDestroyView();
  }

  @NonNull
  @Override
  SubscriptionType getSubscriptionType()
  {
    return SubscriptionType.BOOKMARKS_ALL;
  }

  @NonNull
  @Override
  PurchaseUtils.Period getSelectedPeriod()
  {
    return mDelegate.getSelectedPeriod();
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
    mDelegate.onProductDetailsLoading();
  }

  @Override
  public void onReset()
  {
    mDelegate.onReset();
  }

  @Override
  public void onPriceSelection()
  {
    mDelegate.onPriceSelection();
  }

  @Override
  void showButtonProgress()
  {
    mDelegate.showButtonProgress();
  }

  @Override
  void hideButtonProgress()
  {
    mDelegate.hideButtonProgress();
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
