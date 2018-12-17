package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.design.widget.TabLayout;
import android.support.v4.app.FragmentManager;
import android.support.v4.view.ViewPager;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.TargetFragmentCallback;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.util.SharedPropertiesUtils;
import com.mapswithme.util.statistics.Statistics;

import java.util.Arrays;
import java.util.List;

public class BookmarkCategoriesPagerFragment extends BaseMwmFragment
    implements TargetFragmentCallback
{
  final static String ARG_CATEGORIES_PAGE = "arg_categories_page";
  final static String ARG_CATALOG_DEEPLINK = "arg_catalog_deeplink";

  @SuppressWarnings("NullableProblems")
  @NonNull
  private BookmarksPagerAdapter mAdapter;
  @SuppressWarnings("NullableProblems")
  @Nullable
  private String mCatalogDeeplink;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private BookmarksDownloadFragmentDelegate mDelegate;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mDelegate = new BookmarksDownloadFragmentDelegate(this);
    mDelegate.onCreate(savedInstanceState);

    Bundle args = getArguments();
    if (args == null)
      return;

    mCatalogDeeplink = args.getString(ARG_CATALOG_DEEPLINK);
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);
    mDelegate.onSaveInstanceState(outState);
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    mDelegate.onActivityResult(requestCode, resultCode, data);
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
    mAdapter = new BookmarksPagerAdapter(getContext(), fm, dataSet);
    viewPager.setAdapter(mAdapter);
    viewPager.setCurrentItem(saveAndGetInitialPage());
    tabLayout.setupWithViewPager(viewPager);
    viewPager.addOnPageChangeListener(new PageChangeListener());

    return root;
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mDelegate.onStart();
    if (TextUtils.isEmpty(mCatalogDeeplink))
     return;

    mDelegate.downloadBookmark(mCatalogDeeplink);
    mCatalogDeeplink = null;
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mDelegate.onStop();
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

  @Override
  public void onTargetFragmentResult(int resultCode, @Nullable Intent data)
  {
    mDelegate.onTargetFragmentResult(resultCode, data);
  }

  @Override
  public boolean isTargetAdded()
  {
    return mDelegate.isTargetAdded();
  }

  private class PageChangeListener extends ViewPager.SimpleOnPageChangeListener
  {
    @Override
    public void onPageSelected(int position)
    {
      SharedPropertiesUtils.setLastVisibleBookmarkCategoriesPage(getActivity(), position);
      BookmarksPageFactory factory = mAdapter.getItemFactory(position);
      Statistics.INSTANCE.trackBookmarksTabEvent(factory.getAnalytics().getName());
    }
  }
}
