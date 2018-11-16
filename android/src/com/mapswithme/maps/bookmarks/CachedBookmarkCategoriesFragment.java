package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.cocosw.bottomsheet.BottomSheet;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.CatalogCustomProperty;
import com.mapswithme.maps.bookmarks.data.CatalogTagsGroup;
import com.mapswithme.util.SharedPropertiesUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.sharing.TargetUtils;
import com.mapswithme.util.statistics.Statistics;

import java.util.List;

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
    initHeader(view);
    initPlaceholder();
  }

  private void initPlaceholder()
  {
    View downloadRoutesPlaceHolderBtn = mEmptyViewContainer.findViewById(R.id.download_routers_btn);
    downloadRoutesPlaceHolderBtn.setOnClickListener(new DownloadRoutesClickListener());
  }

  private void initHeader(@NonNull View view)
  {
    View downloadRoutesBtn = view.findViewById(R.id.download_routes_layout);
    downloadRoutesBtn.setOnClickListener(btn -> openBookmarksCatalogScreen());
    View closeHeaderBtn = view.findViewById(R.id.header_close);
    closeHeaderBtn.setOnClickListener(new CloseHeaderClickListener());
    boolean isClosed = SharedPropertiesUtils.isCatalogCategoriesHeaderClosed(getContext());
    View header = view.findViewById(R.id.header);
    UiUtils.hideIf(isClosed, header);
    ImageView imageView = downloadRoutesBtn.findViewById(R.id.image);
    BookmarkCategoriesPageResProvider resProvider = getAdapter().getFactory().getResProvider();
    imageView.setImageResource(resProvider.getFooterImage());
    TextView textView = downloadRoutesBtn.findViewById(R.id.text);
    textView.setText(resProvider.getFooterText());
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
    BookmarksCatalogActivity.startForResult(this, BookmarksCatalogActivity.REQ_CODE_CATALOG);
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

  @Override
  public void onMoreOperationClick(@NonNull BookmarkCategory item)
  {
    showBottomMenu(item);
  }

  @NonNull
  @Override
  BookmarkManager.BookmarksCatalogListener createCatalogListener()
  {
    return new BookmarkCategoriesCatalogListener();
  }

  @Override
  protected void prepareBottomMenuItems(@NonNull BottomSheet bottomSheet)
  {
    setEnableForMenuItem(R.id.delete, bottomSheet, true);
    setEnableForMenuItem(R.id.share, bottomSheet, false);
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

  private class BookmarkCategoriesCatalogListener implements BookmarkManager.BookmarksCatalogListener
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

    @Override
    public void onTagsReceived(boolean successful, @NonNull List<CatalogTagsGroup> tagsGroups)
    {
      //TODO(@alexzatsepin): Implement me if necessary
    }

    @Override
    public void onCustomPropertiesReceived(boolean successful,
                                           @NonNull List<CatalogCustomProperty> properties)
    {
      //TODO(@alexzatsepin): Implement me if necessary
    }

    @Override
    public void onUploadStarted(long originCategoryId)
    {
      //TODO(@alexzatsepin): Implement me if necessary
    }

    @Override
    public void onUploadFinished(@NonNull BookmarkManager.UploadResult uploadResult,
                                 @NonNull String description, long originCategoryId,
                                 long resultCategoryId)
    {
      //TODO(@alexzatsepin): Implement me if necessary
    }
  }
}
