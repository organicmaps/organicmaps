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

@SuppressWarnings("WeakerAccess")
public class BookmarksAllSubscriptionFragment extends AbstractBookmarkSubscriptionFragment
{
  @NonNull
  @Override
  PurchaseController<PurchaseCallback> createPurchaseController()
  {
    return PurchaseFactory.createBookmarksAllSubscriptionController(requireContext());
  }

  @NonNull
  @Override
  SubscriptionFragmentDelegate createFragmentDelegate(@NonNull AbstractBookmarkSubscriptionFragment fragment)
  {
    return new TwoButtonsSubscriptionFragmentDelegate(fragment);
  }

  @Nullable
  @Override
  View onSubscriptionCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                                @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.pager_fragment_bookmarks_all_subscription, container,
                                 false);

    setTopStatusBarOffset(root);
    initViewPager(root);
    return root;
  }

  @NonNull
  @Override
  SubscriptionType getSubscriptionType()
  {
    return SubscriptionType.BOOKMARKS_ALL;
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
        .setActiveDotDrawable(R.drawable.bookmarks_all_marker_active)
        .setInactiveDotDrawable(R.drawable.bookmarks_all_marker_inactive)
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
      return BookmarksAllSubscriptionPageFragment.newInstance(i);
    }

    @Override
    public int getCount()
    {
      return mItems.size();
    }
  }
}
