package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.viewpager.widget.ViewPager;
import com.google.android.material.tabs.TabLayout;
import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.TargetFragmentCallback;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.maps.purchase.PurchaseUtils;
import com.mapswithme.util.SharedPropertiesUtils;
import com.mapswithme.util.statistics.Statistics;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class BookmarkCategoriesPagerFragment extends BaseMwmFragment
    implements TargetFragmentCallback, AlertDialogCallback, AuthCompleteListener,
               KmlImportController.ImportKmlCallback, BookmarkManager.BookmarksLoadingListener
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
  @NonNull
  private final Runnable mImportKmlTask = new ImportKmlTask();
  @Nullable
  private KmlImportController mKmlImportController;

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

    FragmentManager fm = requireActivity().getSupportFragmentManager();
    List<BookmarksPageFactory> dataSet = getAdapterDataSet();
    mAdapter = new BookmarksPagerAdapter(requireContext(), fm, dataSet);
    mViewPager.setAdapter(mAdapter);
    mViewPager.setCurrentItem(saveAndGetInitialPage());
    tabLayout.setupWithViewPager(mViewPager);
    mViewPager.addOnPageChangeListener(new PageChangeListener());
    mDelegate.onCreateView(savedInstanceState);
    mKmlImportController = new KmlImportController(requireActivity(), this);
    return root;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    Bundle args = getArguments();
    if (args != null)
    {
      BookmarkCategory category = args.getParcelable(BookmarksListFragment.EXTRA_CATEGORY);
      if (category == null)
        return;

      BookmarkListActivity.startForResult(requireActivity(), category);
    }
  }

  @Override
  public void onStart()
  {
    super.onStart();
    if (mKmlImportController != null)
      mKmlImportController.onStart();
    BookmarkManager.INSTANCE.addLoadingListener(this);
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
    if (!BookmarkManager.INSTANCE.isAsyncBookmarksLoadingInProgress())
      mImportKmlTask.run();
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
    if (mKmlImportController != null)
      mKmlImportController.onStop();
    mDelegate.onStop();
    BookmarkManager.INSTANCE.removeLoadingListener(this);
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
      SharedPropertiesUtils.setLastVisibleBookmarkCategoriesPage(requireActivity(), page);
      return page;
    }

    return SharedPropertiesUtils.getLastVisibleBookmarkCategoriesPage(requireActivity());
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

  @Override
  public void onFinishKmlImport()
  {
    if (getFragmentManager() == null)
      return;
    for (Fragment fragment : getFragmentManager().getFragments())
    {
      if (fragment != this && fragment instanceof KmlImportController.ImportKmlCallback)
        ((KmlImportController.ImportKmlCallback) fragment).onFinishKmlImport();
    }
  }

  @Override
  public void onBookmarksLoadingStarted()
  {
    // No op
  }

  @Override
  public void onBookmarksLoadingFinished()
  {
    mImportKmlTask.run();
  }

  @Override
  public void onBookmarksFileLoaded(boolean success)
  {
    // No op
  }

  private class PageChangeListener extends ViewPager.SimpleOnPageChangeListener
  {
    @Override
    public void onPageSelected(int position)
    {
      SharedPropertiesUtils.setLastVisibleBookmarkCategoriesPage(requireActivity(), position);
      BookmarksPageFactory factory = mAdapter.getItemFactory(position);
      Statistics.INSTANCE.trackBookmarksTabEvent(factory.getAnalytics().getName());
    }
  }

  private void importKml()
  {
    if (mKmlImportController != null)
      mKmlImportController.importKml();
  }

  private class ImportKmlTask implements Runnable
  {
    private boolean alreadyDone;

    @Override
    public void run()
    {
      if (alreadyDone)
        return;

      importKml();
      alreadyDone = true;
    }
  }
}
