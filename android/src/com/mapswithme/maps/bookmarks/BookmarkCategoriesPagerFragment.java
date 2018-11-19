package com.mapswithme.maps.bookmarks;

import android.app.Activity;
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
import com.mapswithme.maps.auth.Authorizer;
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
  @NonNull
  private BookmarkDownloadController mController;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private Authorizer mAuthorizer;
  @Nullable
  private String mCatalogDeeplink;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mAuthorizer = new Authorizer(this);
    mController = new DefaultBookmarkDownloadController(mAuthorizer,
                                                        new CatalogListenerDecorator(this));
    if (savedInstanceState != null)
      mController.onRestore(savedInstanceState);

    Bundle args = getArguments();
    if (args == null)
      return;

    mCatalogDeeplink = args.getString(ARG_CATALOG_DEEPLINK);
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    if (resultCode == Activity.RESULT_OK && requestCode == DefaultBookmarkDownloadController.REQ_CODE_PAY_BOOKMARK)
    {
      mController.retryDownloadBookmark();
    }
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);
    mController.onSave(outState);
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
    mController.attach(this);
    if (TextUtils.isEmpty(mCatalogDeeplink))
     return;

    mController.downloadBookmark(mCatalogDeeplink);
    mCatalogDeeplink = null;
  }

  @Override
  public void onStop()
  {
    mController.detach();
    super.onStop();
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
    mAuthorizer.onTargetFragmentResult(resultCode, data);
  }

  @Override
  public boolean isTargetAdded()
  {
    return isAdded();
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
