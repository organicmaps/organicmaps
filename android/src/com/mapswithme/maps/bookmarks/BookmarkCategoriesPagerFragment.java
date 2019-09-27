package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.google.android.material.tabs.TabLayout;
import androidx.fragment.app.FragmentManager;
import androidx.viewpager.widget.ViewPager;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.TargetFragmentCallback;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.maps.purchase.PurchaseUtils;
import com.mapswithme.util.SharedPropertiesUtils;
import com.mapswithme.util.statistics.Statistics;

import java.util.Arrays;
import java.util.List;

public class BookmarkCategoriesPagerFragment extends BaseMwmFragment
    implements TargetFragmentCallback, AlertDialogCallback, AuthCompleteListener
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

  @SuppressWarnings("NullableProblems")
  @NonNull
  private AlertDialogCallback mInvalidSubsDialogCallback;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private ViewPager mViewPager;

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
    mInvalidSubsDialogCallback = new InvalidSubscriptionAlertDialogCallback(this);
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
    mViewPager = root.findViewById(R.id.viewpager);
    TabLayout tabLayout = root.findViewById(R.id.sliding_tabs_layout);

    FragmentManager fm = getActivity().getSupportFragmentManager();
    List<BookmarksPageFactory> dataSet = getAdapterDataSet();
    mAdapter = new BookmarksPagerAdapter(getContext(), fm, dataSet);
    mViewPager.setAdapter(mAdapter);
    mViewPager.setCurrentItem(saveAndGetInitialPage());
    tabLayout.setupWithViewPager(mViewPager);
    mViewPager.addOnPageChangeListener(new PageChangeListener());
    mDelegate.onCreateView(savedInstanceState);
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
  public void onResume()
  {
    super.onResume();
    mDelegate.onResume();
  }

  @Override
  public void onPause()
  {
    super.onPause();
    mDelegate.onPause();
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mDelegate.onStop();
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    mDelegate.onDestroyView();
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

  @Override
  public void onAlertDialogPositiveClick(int requestCode, int which)
  {
    mInvalidSubsDialogCallback.onAlertDialogPositiveClick(requestCode, which);
  }

  @Override
  public void onAlertDialogNegativeClick(int requestCode, int which)
  {
    if (PurchaseUtils.REQ_CODE_CHECK_INVALID_SUBS_DIALOG == requestCode)
      mViewPager.setAdapter(mAdapter);

    mInvalidSubsDialogCallback.onAlertDialogNegativeClick(requestCode, which);
  }

  @Override
  public void onAlertDialogCancel(int requestCode)
  {
    mInvalidSubsDialogCallback.onAlertDialogCancel(requestCode);
  }

  @Override
  public void onAuthCompleted()
  {
    mViewPager.setAdapter(mAdapter);
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
