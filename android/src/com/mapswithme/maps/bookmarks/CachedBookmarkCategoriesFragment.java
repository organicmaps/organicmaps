package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.SharedPropertiesUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.sharing.TargetUtils;
import com.mapswithme.util.statistics.Statistics;

public class CachedBookmarkCategoriesFragment extends BaseBookmarkCategoriesFragment implements
                                                                                     BookmarkManager.BookmarksCatalogListener
{
  @SuppressWarnings("NullableProblems")
  @NonNull
  private ViewGroup mEmptyViewContainer;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mPayloadContainer;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mProgressContainer;

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    View root = super.onCreateView(inflater, container, savedInstanceState);
    mProgressContainer = root.findViewById(R.id.placeholder_loading);
    mEmptyViewContainer = root.findViewById(R.id.placeholder_container);
    mPayloadContainer = root.findViewById(R.id.cached_bookmarks_payload_container);
    View downloadBtn = mEmptyViewContainer.findViewById(R.id.download_routers_btn);
    downloadBtn.setOnClickListener(new DownloadRoutesClickListener());
    View closeHeaderBtn = root.findViewById(R.id.header_close);
    closeHeaderBtn.setOnClickListener(new CloseHeaderClickListener());
    boolean isClosed = SharedPropertiesUtils.isCatalogCategoriesHeaderClosed(getContext());
    UiUtils.showIf(!isClosed, closeHeaderBtn);
    return root;
  }

  @Override
  protected int getLayoutRes()
  {
    return R.layout.fragment_catalog_bookmark_categories;
  }

  @NonNull
  @Override
  protected CatalogBookmarkCategoriesAdapter createAdapter()
  {
    return new CatalogBookmarkCategoriesAdapter(getContext());
  }

  @Override
  public void onFooterClick()
  {
    openBookmarksCatalogScreen();
  }

  @Override
  protected void onShareActionSelected(@NonNull BookmarkCategory category)
  {
    String deepLink = BookmarkManager.INSTANCE.getCatalogDeeplink(category.getId());
    Intent intent = new Intent(Intent.ACTION_SEND)
        .setType(TargetUtils.TYPE_TEXT_PLAIN)
        .putExtra(Intent.EXTRA_SUBJECT, deepLink)
        .putExtra(Intent.EXTRA_TEXT, getString(R.string.share_bookmarks_email_body));
    startActivity(Intent.createChooser(intent, getString(R.string.share)));
  }

  @Override
  protected void onDeleteActionSelected(@NonNull BookmarkCategory category)
  {
    super.onDeleteActionSelected(category);
    updateLoadingPlaceholder();
  }

  @Override
  protected void updateLoadingPlaceholder()
  {
    super.updateLoadingPlaceholder();
    boolean showLoadingPlaceholder = BookmarkManager.INSTANCE.isAsyncBookmarksLoadingInProgress();
    if (showLoadingPlaceholder)
    {
      mProgressContainer.setVisibility(View.VISIBLE);
      mPayloadContainer.setVisibility(View.GONE);
      mEmptyViewContainer.setVisibility(View.GONE);
    }
    else
    {
      boolean isEmptyAdapter = getAdapter().getItemCount() == 0;
      UiUtils.showIf(isEmptyAdapter, mEmptyViewContainer);
      mPayloadContainer.setVisibility(isEmptyAdapter ? View.GONE : View.VISIBLE);
      mProgressContainer.setVisibility(View.GONE);
    }
  }

  @Override
  public void onStart()
  {
    super.onStart();
    BookmarkManager.INSTANCE.addCatalogListener(this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    BookmarkManager.INSTANCE.removeCatalogListener(this);
  }

  @Override
  protected int getCategoryMenuResId()
  {
    return R.menu.menu_catalog_bookmark_categories;
  }

  private void openBookmarksCatalogScreen()
  {
    Intent intent = new Intent(getActivity(), BookmarksCatalogActivity.class)
        .putExtra(BookmarksCatalogFragment.EXTRA_BOOKMARKS_CATALOG_URL,
                  getCatalogUrl());
    getActivity().startActivity(intent);
    Statistics.INSTANCE.trackOpenCatalogScreen();
  }

  @NonNull
  private String getCatalogUrl()
  {
    return BookmarkManager.INSTANCE.getCatalogFrontendUrl();
  }

  @Override
  public void onMoreOperationClick(@NonNull BookmarkCategory item)
  {
    showBottomMenu(item);
  }

  @Override
  public void onImportStarted(@NonNull String serverId)
  {
    mProgressContainer.setVisibility(View.VISIBLE);
    mEmptyViewContainer.setVisibility(View.GONE);
    mPayloadContainer.setVisibility(View.GONE);
  }

  @Override
  public void onImportFinished(@NonNull String serverId, boolean successful)
  {
    if (successful)
    {
      mPayloadContainer.setVisibility(View.VISIBLE);
      mProgressContainer.setVisibility(View.GONE);
      mEmptyViewContainer.setVisibility(View.GONE);
      getAdapter().notifyDataSetChanged();
    }
    else
    {
      boolean isEmptyAdapter = getAdapter().getItemCount() == 0;
      mProgressContainer.setVisibility(View.GONE);
      UiUtils.showIf(isEmptyAdapter, mEmptyViewContainer);
      mPayloadContainer.setVisibility(isEmptyAdapter ? View.GONE : View.VISIBLE);
    }
  }

  @Override
  protected void prepareBottomMenuItems(@NonNull BottomSheetHelper.Builder builder)
  {
    setEnableForMenuItem(R.id.delete_list, builder, true);
    setEnableForMenuItem(R.id.share_list, builder, false);
  }

  private class CloseHeaderClickListener implements View.OnClickListener
  {
    @Override
    public void onClick(View v)
    {
      View header = mPayloadContainer.findViewById(R.id.header);
      header.setVisibility(View.GONE);
      SharedPropertiesUtils.setCatalogCategoriesHeaderClosed(getContext(), true);
    }
  }

  private class DownloadRoutesClickListener implements View.OnClickListener
  {
    @Override
    public void onClick(View v)
    {
      openBookmarksCatalogScreen();
    }
  }
}
