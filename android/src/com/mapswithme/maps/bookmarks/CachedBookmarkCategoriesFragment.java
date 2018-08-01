package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
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

public class CachedBookmarkCategoriesFragment extends BaseBookmarkCategoriesFragment
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
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mProgressContainer = view.findViewById(R.id.placeholder_loading);
    mEmptyViewContainer = view.findViewById(R.id.placeholder_container);
    mPayloadContainer = view.findViewById(R.id.cached_bookmarks_payload_container);
    View downloadBtn = mEmptyViewContainer.findViewById(R.id.download_routers_btn);
    downloadBtn.setOnClickListener(new DownloadRoutesClickListener());
    View closeHeaderBtn = view.findViewById(R.id.header_close);
    closeHeaderBtn.setOnClickListener(new CloseHeaderClickListener());
    boolean isClosed = SharedPropertiesUtils.isCatalogCategoriesHeaderClosed(getContext());
    UiUtils.showIf(!isClosed, closeHeaderBtn);
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
  protected int getCategoryMenuResId()
  {
    return R.menu.menu_catalog_bookmark_categories;
  }

  private void openBookmarksCatalogScreen()
  {
    Intent intent = new Intent(getActivity(), BookmarksCatalogActivity.class)
        .putExtra(BookmarksCatalogFragment.EXTRA_BOOKMARKS_CATALOG_URL,
                  getCatalogUrl());
    startActivityForResult(intent, BookmarksCatalogActivity.REQ_CODE_CATALOG);
    Statistics.INSTANCE.trackOpenCatalogScreen();
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    if (requestCode == BookmarksCatalogActivity.REQ_CODE_CATALOG && resultCode == Activity.RESULT_OK)
    {
      getActivity().setResult(Activity.RESULT_OK, data);
      getActivity().finish();
    }
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

  @NonNull
  @Override
  BookmarkManager.BookmarksCatalogListener createCatalogListener()
  {
    return new BookmarkManager.BookmarksCatalogListener()
    {
      @Override
      public void onImportStarted(@NonNull String serverId)
      {
        UiUtils.show(mProgressContainer);
        UiUtils.hide(mEmptyViewContainer, mPayloadContainer);
      }

      @Override
      public void onImportFinished(@NonNull String serverId, long catId, boolean successful)
      {
        if (successful)
        {
          UiUtils.show(mPayloadContainer);
          UiUtils.hide(mProgressContainer, mEmptyViewContainer);
          getAdapter().notifyDataSetChanged();
        }
        else
        {
          boolean isEmptyAdapter = getAdapter().getItemCount() == 0;
          UiUtils.hide(mProgressContainer);
          UiUtils.showIf(isEmptyAdapter, mEmptyViewContainer);
          UiUtils.hideIf(isEmptyAdapter, mPayloadContainer);
        }
      }
    };
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
