package com.mapswithme.maps.bookmarks;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.design.widget.TabLayout;
import android.support.v4.app.FragmentManager;
import android.support.v4.view.ViewPager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.util.SharedPropertiesUtils;

import java.util.Arrays;
import java.util.List;

public class BookmarkCategoriesPagerFragment extends BaseMwmFragment
{
  final static String ARG_CATEGORIES_PAGE = "arg_categories_page";

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_bookmark_categories_pager, container, false);
    ViewPager viewPager = root.findViewById(R.id.viewpager);
    TabLayout tabLayout = root.findViewById(R.id.sliding_tabs_layout);

    FragmentManager fm = getActivity().getSupportFragmentManager();
    List<BookmarksPageFactory> dataSet = getAdapterDataSet();
    BookmarksPagerAdapter adapter = new BookmarksPagerAdapter(getContext(), fm, dataSet);
    viewPager.setAdapter(adapter);
    viewPager.setCurrentItem(saveAndGetInitialPage());
    tabLayout.setupWithViewPager(viewPager);
    viewPager.addOnPageChangeListener(new PageChangeListener());

    return root;
  }

  private int saveAndGetInitialPage()
  {
    Bundle args = getArguments();
    if (args != null && args.containsKey(ARG_CATEGORIES_PAGE))
    {
      int page = args.getInt(ARG_CATEGORIES_PAGE);
      SharedPropertiesUtils.setLastVisibleBookmarkCategoriesPage(getActivity(), page);
      return page;
    }

    return SharedPropertiesUtils.getLastVisibleBookmarkCategoriesPage(getActivity());
  }

  @NonNull
  private static List<BookmarksPageFactory> getAdapterDataSet()
  {
    return Arrays.asList(BookmarksPageFactory.values());
  }

  private class PageChangeListener extends ViewPager.SimpleOnPageChangeListener
  {
    @Override
    public void onPageSelected(int position)
    {
      SharedPropertiesUtils.setLastVisibleBookmarkCategoriesPage(getActivity(), position);
    }
  }
}
