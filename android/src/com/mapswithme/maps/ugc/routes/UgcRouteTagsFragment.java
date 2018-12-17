package com.mapswithme.maps.ugc.routes;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.ActivityCompat;
import android.support.v7.widget.DividerItemDecoration;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.AdapterPositionConverter;
import com.mapswithme.maps.adapter.OnItemClickListener;
import com.mapswithme.maps.adapter.RecyclerCompositeAdapter;
import com.mapswithme.maps.adapter.RepeatablePairPositionConverter;
import com.mapswithme.maps.adapter.TagGroupNameAdapter;
import com.mapswithme.maps.adapter.TagsAdapter;
import com.mapswithme.maps.adapter.TagsCompositeAdapter;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.CatalogCustomProperty;
import com.mapswithme.maps.bookmarks.data.CatalogTag;
import com.mapswithme.maps.bookmarks.data.CatalogTagsGroup;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class UgcRouteTagsFragment extends BaseMwmFragment implements BookmarkManager.BookmarksCatalogListener,
                                                                     OnItemClickListener<Pair<TagsAdapter, TagsAdapter.TagViewHolder>>,
                                                                     AlertDialogCallback
{
  private static final String BUNDLE_SELECTED_TAGS = "bundle_saved_tags";
  private static final String ERROR_LOADING_DIALOG_TAG = "error_loading_dialog";
  private static final int ERROR_LOADING_DIALOG_REQ_CODE = 205;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private RecyclerView mRecycler;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mProgress;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private ViewGroup mTagsContainer;

  @Nullable
  private Bundle mSavedInstanceState;

  @Nullable
  private TagsCompositeAdapter mTagsAdapter;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    ViewGroup root = (ViewGroup) inflater.inflate(R.layout.fragment_ugc_routes, container,false);
    setHasOptionsMenu(true);
    mProgress = root.findViewById(R.id.progress_container);
    mTagsContainer = root.findViewById(R.id.tags_container);
    initRecycler(root);
    UiUtils.hide(mTagsContainer);
    UiUtils.show(mProgress);
    BookmarkManager.INSTANCE.requestRouteTags();
    mSavedInstanceState = savedInstanceState;
    return root;
  }

  private void initRecycler(@NonNull ViewGroup root)
  {
    mRecycler = root.findViewById(R.id.recycler);
    mRecycler.setItemAnimator(null);
    RecyclerView.ItemDecoration decor = ItemDecoratorFactory.createRatingRecordDecorator(
        getContext().getApplicationContext(),
        DividerItemDecoration.VERTICAL, R.drawable.divider_transparent_half_plus_eight);
    mRecycler.addItemDecoration(decor);
  }

  private void onRetryClicked()
  {
    UiUtils.hide(mTagsContainer);
    UiUtils.show(mProgress);
    BookmarkManager.INSTANCE.requestRouteTags();
  }

  private void showErrorLoadingDialog()
  {
    AlertDialog dialog = new AlertDialog.Builder()
        .setTitleId(R.string.title_error_downloading_bookmarks)
        .setMessageId(R.string.tags_loading_error_subtitle)
        .setPositiveBtnId(R.string.try_again)
        .setNegativeBtnId(R.string.cancel)
        .setReqCode(ERROR_LOADING_DIALOG_REQ_CODE)
        .setFragManagerStrategy(new AlertDialog.ActivityFragmentManagerStrategy())
        .build();
    dialog.setTargetFragment(this, ERROR_LOADING_DIALOG_REQ_CODE);
    dialog.show(this, ERROR_LOADING_DIALOG_TAG);
  }

  @Override
  public void onCreateOptionsMenu(Menu menu, MenuInflater inflater)
  {
    inflater.inflate(R.menu.menu_tags_done, menu);
  }

  @Override
  public void onPrepareOptionsMenu(Menu menu)
  {
    super.onPrepareOptionsMenu(menu);
    MenuItem item = menu.findItem(R.id.done);
    item.setVisible(hasSelectedItems());
  }

  private boolean hasSelectedItems()
  {
    return mTagsAdapter != null && mTagsAdapter.hasSelectedItems();
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == R.id.done)
    {
      onDoneOptionItemClicked();
      return true;
    }

    return super.onOptionsItemSelected(item);
  }

  private void onDoneOptionItemClicked()
  {
    if (mTagsAdapter == null)
      return;

    ArrayList<CatalogTag> value = new ArrayList<>(mTagsAdapter.getSelectedTags());
    Intent result = new Intent().putParcelableArrayListExtra(UgcRouteTagsActivity.EXTRA_TAGS, value);
    getActivity().setResult(Activity.RESULT_OK, result);
    getActivity().finish();
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);
    if (mTagsAdapter != null)
      outState.putParcelableArrayList(BUNDLE_SELECTED_TAGS, new ArrayList<>(mTagsAdapter.getSelectedTags()));
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
  public void onImportStarted(@NonNull String serverId)
  {
    /* Do nothing by default */
  }

  @Override
  public void onImportFinished(@NonNull String serverId, long catId, boolean successful)
  {
    /* Do nothing by default */
  }

  @Override
  public void onTagsReceived(boolean successful, @NonNull List<CatalogTagsGroup> tagsGroups)
  {
    UiUtils.showIf(successful && tagsGroups.size() != 0, mTagsContainer);
    UiUtils.hide(mProgress);

    if (tagsGroups.size() == 0 || !successful)
    {
      showErrorLoadingDialog();
      return;
    }
    installTags(tagsGroups);
  }

  @Override
  public void onCustomPropertiesReceived(boolean successful,
                                         @NonNull List<CatalogCustomProperty> properties)
  {
    /* Not ready yet */
  }

  private void installTags(@NonNull List<CatalogTagsGroup> tagsGroups)
  {
    List<CatalogTag> savedStateTags = validateSavedState(mSavedInstanceState);
    TagGroupNameAdapter categoryAdapter = new TagGroupNameAdapter(tagsGroups);
    mTagsAdapter = new TagsCompositeAdapter(getContext(), tagsGroups, savedStateTags, this);
    RecyclerCompositeAdapter compositeAdapter = makeCompositeAdapter(categoryAdapter, mTagsAdapter);
    LinearLayoutManager layoutManager = new LinearLayoutManager(getContext(),
                                                                LinearLayoutManager.VERTICAL,
                                                                false);
    mRecycler.setLayoutManager(layoutManager);
    mRecycler.setAdapter(compositeAdapter);
    ActivityCompat.invalidateOptionsMenu(getActivity());
  }

  @NonNull
  private static List<CatalogTag> validateSavedState(@Nullable Bundle savedState)
  {
    List<CatalogTag> tags;
    if (savedState == null || (tags = savedState.getParcelableArrayList(BUNDLE_SELECTED_TAGS)) == null)
      return Collections.emptyList();

    return tags;
  }

  @NonNull
  private static RecyclerCompositeAdapter makeCompositeAdapter(@NonNull TagGroupNameAdapter categoryAdapter,
                                                               @NonNull TagsCompositeAdapter tagsCompositeAdapter)

  {
    AdapterPositionConverter converter = new RepeatablePairPositionConverter(categoryAdapter,
                                                                             tagsCompositeAdapter);
    return new RecyclerCompositeAdapter(converter, categoryAdapter, tagsCompositeAdapter);
  }

  @Override
  public void onUploadStarted(long originCategoryId)
  {
    /* Do nothing by default */
  }

  @Override
  public void onUploadFinished(@NonNull BookmarkManager.UploadResult uploadResult, @NonNull String description,
                               long originCategoryId, long resultCategoryId)
  {
    /* Do nothing by default */
  }

  @Override
  public void onItemClick(@NonNull View v,
                          @NonNull Pair<TagsAdapter, TagsAdapter.TagViewHolder> item)
  {
    ActivityCompat.invalidateOptionsMenu(getActivity());
    TagsAdapter adapter = item.first;
    int position = item.second.getAdapterPosition();
    adapter.notifyItemChanged(position);
  }

  @Override
  public void onAlertDialogPositiveClick(int requestCode, int which)
  {
    onRetryClicked();
  }

  @Override
  public void onAlertDialogNegativeClick(int requestCode, int which)
  {
    getActivity().setResult(Activity.RESULT_CANCELED);
    getActivity().finish();
  }

  @Override
  public void onAlertDialogCancel(int requestCode)
  {
    getActivity().setResult(Activity.RESULT_CANCELED);
    getActivity().finish();
  }
}
