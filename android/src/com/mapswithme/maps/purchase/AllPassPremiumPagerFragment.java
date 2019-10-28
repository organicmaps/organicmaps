package com.mapswithme.maps.purchase;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.widget.DotPager;
import com.mapswithme.maps.widget.ParallaxBackgroundPageListener;
import com.mapswithme.maps.widget.ParallaxBackgroundViewPager;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.List;

public class AllPassPremiumPagerFragment extends BaseMwmFragment
{
  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.pager_fragment_all_pass_premium, container, false);
    initStatusBarPlaceholder(root);
    initViewPager(root);
    return root;
  }

  private void initStatusBarPlaceholder(@NonNull View root)
  {
    View statusBarPlaceholder = root.findViewById(R.id.status_bar_placeholder);
    ViewGroup.LayoutParams params = statusBarPlaceholder.getLayoutParams();
    params.height = UiUtils.getStatusBarHeight(requireContext());
    statusBarPlaceholder.setLayoutParams(params);
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
      return AllPassPremiumFragment.newInstance(i);
    }

    @Override
    public int getCount()
    {
      return mItems.size();
    }
  }
}
