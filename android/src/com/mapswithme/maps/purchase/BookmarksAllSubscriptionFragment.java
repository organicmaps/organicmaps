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
import com.mapswithme.maps.widget.ParallaxBackgroundPageListener;
import com.mapswithme.maps.widget.ParallaxBackgroundViewPager;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;

@SuppressWarnings("WeakerAccess")
public class BookmarksAllSubscriptionFragment extends AbstractBookmarkSubscriptionFragment
{
  public static final String BUNDLE_DATA = "data";

  @NonNull
  private List<BookmarksAllSubscriptionPage> mPageOrderList = Collections.emptyList();

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

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    BookmarkAllSubscriptionData data = requireArguments().getParcelable(BUNDLE_DATA);
    mPageOrderList = Objects.requireNonNull(data).getOrderList();
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
    final List<BookmarksAllSubscriptionPageData> items = makeItems();
    ParallaxBackgroundViewPager viewPager = root.findViewById(R.id.pager);
    PagerAdapter adapter = new ParallaxFragmentPagerAdapter(requireFragmentManager(), items);
    viewPager.setAdapter(adapter);
    ViewPager.OnPageChangeListener listener =
        new ParallaxBackgroundPageListener(items, root.findViewById(R.id.img2),
                                           root.findViewById(R.id.img1));
    viewPager.addOnPageChangeListener(listener);
    viewPager.startAutoScroll();
  }

  @NonNull
  private List<BookmarksAllSubscriptionPageData> makeItems()
  {
    List<BookmarksAllSubscriptionPageData> items = new ArrayList<>();
    for (BookmarksAllSubscriptionPage page : mPageOrderList)
    {
      int resId = 0;
      switch (page)
      {
        case GUIDES:
          resId = R.drawable.all_pass_premium_60;
          break;
        case BOOKMARKS:
          resId = R.drawable.all_pass_premium_62;
          break;
        case ELEVATION:
          resId = R.drawable.all_pass_premium_61;
          break;
        case LONELY:
          resId = R.drawable.all_pass_premium_63;
          break;
      }
      items.add(new BookmarksAllSubscriptionPageData(resId, page));
    }
    return items;
  }

  private static class ParallaxFragmentPagerAdapter extends FragmentPagerAdapter
  {
    @NonNull
    private final List<BookmarksAllSubscriptionPageData> mItems;

    ParallaxFragmentPagerAdapter(@NonNull FragmentManager fragmentManager,
                                 @NonNull List<BookmarksAllSubscriptionPageData> items)
    {
      super(fragmentManager, BEHAVIOR_RESUME_ONLY_CURRENT_FRAGMENT);
      mItems = items;
    }

    @Override
    public Fragment getItem(int i)
    {
      return BookmarksAllSubscriptionPageFragment.newInstance(mItems.get(i));
    }

    @Override
    public int getCount()
    {
      return mItems.size();
    }
  }
}
