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

import java.util.Arrays;
import java.util.List;

public class BookmarkCategoriesPagerFragment extends BaseMwmFragment
{

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
    View root = inflater.inflate(R.layout.fragment_bookmark_categories_pager, null);
    ViewPager viewPager = root.findViewById(R.id.viewpager);
    TabLayout tabLayout = root.findViewById(R.id.sliding_tabs_layout);

    FragmentManager fm = getActivity().getSupportFragmentManager();
    BookmarksPagerAdapter adapter = new BookmarksPagerAdapter(getContext(),
                                                              fm,
                                                              prepareAdapterDataSet());
    viewPager.setAdapter(adapter);
    tabLayout.setupWithViewPager(viewPager);
    return root;
  }

  @NonNull
  private static List<BookmarksPageFactory> prepareAdapterDataSet()
  {
    return Arrays.asList(BookmarksPageFactory.PRIVATE, BookmarksPageFactory.CATALOG);
  }
}
