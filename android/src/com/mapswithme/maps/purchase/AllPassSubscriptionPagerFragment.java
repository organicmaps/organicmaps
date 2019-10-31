package com.mapswithme.maps.purchase;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.databinding.DataBindingUtil;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentPagerAdapter;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.databinding.PagerFragmentAllPassSubscriptionBinding;
import com.mapswithme.maps.widget.DotPager;
import com.mapswithme.maps.widget.ParallaxBackgroundPageListener;
import com.mapswithme.maps.widget.ParallaxBackgroundViewPager;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class AllPassSubscriptionPagerFragment extends BaseMwmFragment
{

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    PagerFragmentAllPassSubscriptionBinding dataBinding
        = DataBindingUtil.inflate(inflater, R.layout.pager_fragment_all_pass_subscription, container, false);
    View root = dataBinding.getRoot();
    dataBinding.setButtons(makeButtons());
    dataBinding.setAnnualClickListener(v -> {});
    dataBinding.setMonthClickListener(v -> {});
    setTopStatusBarOffset(dataBinding);
    initViewPager(root);
    return root;
  }

  @NonNull
  private static List<SubsButtonEntity> makeButtons()
  {
    return Arrays.asList(new SubsButtonEntity("sadasdasdsd", "20usd", "-20%"),
                         new SubsButtonEntity("sadasdasdsd", "30usd"));
  }

  private void setTopStatusBarOffset(@NonNull PagerFragmentAllPassSubscriptionBinding binding)
  {
    ViewGroup.LayoutParams params = binding.statusBarPlaceholder.getLayoutParams();
    int statusBarHeight = UiUtils.getStatusBarHeight(requireContext());
    params.height = statusBarHeight;
    binding.statusBarPlaceholder.setLayoutParams(params);
    ViewGroup.MarginLayoutParams headerParams = (ViewGroup.MarginLayoutParams) binding.header.getLayoutParams();
    headerParams.topMargin  = Math.max(0, headerParams.topMargin - statusBarHeight);
    binding.header.setLayoutParams(headerParams);
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
      return AllPassSubscriptionFragment.newFragment(i);
    }

    @Override
    public int getCount()
    {
      return mItems.size();
    }
  }
}
