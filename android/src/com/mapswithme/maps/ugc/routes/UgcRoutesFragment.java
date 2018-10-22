package com.mapswithme.maps.ugc.routes;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
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
import com.mapswithme.maps.adapter.AdapterIndexConverter;
import com.mapswithme.maps.adapter.RecyclerCompositeAdapter;
import com.mapswithme.maps.adapter.RepeatablePairIndexConverter;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.bookmarks.OnItemClickListener;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.CatalogTag;
import com.mapswithme.maps.bookmarks.data.CatalogTagsGroup;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class UgcRoutesFragment extends BaseMwmFragment implements BookmarkManager.BookmarksCatalogListener,
                                                                  OnItemClickListener<Pair<TagsAdapter, TagsAdapter.TagViewHolder>>
{
  private static final String BUNDLE_SAVED_TAGS = "bundle_saved_tags";

  @SuppressWarnings("NullableProblems")
  @NonNull
  private RecyclerView mRecycler;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mProgress;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mRetryBtnContainer;

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
    LayoutInflater layoutInflater = LayoutInflater.from(getContext());
    ViewGroup root = (ViewGroup) layoutInflater.inflate(R.layout.ugc_routes_frag, container,
                                                         false);
    setHasOptionsMenu(true);
    mProgress = root.findViewById(R.id.progress_container);
    mTagsContainer = root.findViewById(R.id.tags_container);
    mRetryBtnContainer = root.findViewById(R.id.retry_btn_container);
    initRecycler(root);
    View retryBtn = mRetryBtnContainer.findViewById(R.id.retry_btn);
    retryBtn.setOnClickListener(v -> onRetryClicked());
    UiUtils.hide(mTagsContainer, mRetryBtnContainer);
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
    UiUtils.hide(mTagsContainer, mRetryBtnContainer);
    UiUtils.show(mProgress);
    BookmarkManager.INSTANCE.requestRouteTags();
  }

  @Override
  public void onCreateOptionsMenu(Menu menu, MenuInflater inflater)
  {
    inflater.inflate(R.menu.menu_done, menu);
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
      outState.putParcelableArrayList(BUNDLE_SAVED_TAGS, new ArrayList<>(mTagsAdapter.getSelectedTags()));
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
  public void onTagsReceived(boolean successful, @NonNull CatalogTagsGroup[] tagsGroups)
  {
    UiUtils.showIf(successful && tagsGroups.length != 0, mTagsContainer);
    UiUtils.hideIf(successful && tagsGroups.length != 0, mRetryBtnContainer);
    UiUtils.hide(mProgress);

    if (tagsGroups.length == 0)
      return;
    installTags(tagsGroups);
  }

  private void installTags(@NonNull CatalogTagsGroup[] tagsGroups)
  {
    List<CatalogTag> savedStateTags = validateSavedState(mSavedInstanceState);
    List<CatalogTagsGroup> groups = Collections.unmodifiableList(Arrays.asList(tagsGroups));
    CategoryAdapter categoryAdapter = new CategoryAdapter(groups);
    mTagsAdapter = new TagsCompositeAdapter(getContext(), groups, savedStateTags, this);
    RecyclerCompositeAdapter compositeAdapter = makeCompositeAdapter(categoryAdapter, mTagsAdapter);
    mRecycler.setItemAnimator(null);
    LinearLayoutManager layoutManager = new LinearLayoutManager(getContext(), LinearLayoutManager.VERTICAL, false);
    mRecycler.setLayoutManager(layoutManager);
    mRecycler.setAdapter(compositeAdapter);
  }

  @NonNull
  private static List<CatalogTag> validateSavedState(@Nullable Bundle savedState)
  {
    List<CatalogTag> tags;
    if (savedState == null || (tags = savedState.getParcelableArrayList(BUNDLE_SAVED_TAGS)) == null)
      return Collections.emptyList();

    return tags;
  }

  @NonNull
  private static RecyclerCompositeAdapter makeCompositeAdapter(@NonNull CategoryAdapter categoryAdapter,
                                                               @NonNull TagsCompositeAdapter tagsCompositeAdapter)

  {
    AdapterIndexConverter converter = new RepeatablePairIndexConverter(categoryAdapter,
                                                                       tagsCompositeAdapter);
    return new RecyclerCompositeAdapter(converter, categoryAdapter, tagsCompositeAdapter);
  }

  @Override
  public void onUploadStarted(long originCategoryId)
  {
    /* Do nothing by default */
  }

  @Override
  public void onUploadFinished(int uploadResult, @NonNull String description,
                               long originCategoryId, long resultCategoryId)
  {
    /* Do nothing by default */
  }

  @Override
  public void onItemClick(@NonNull View v,
                          @NonNull Pair<TagsAdapter, TagsAdapter.TagViewHolder> item)
  {
    TagsAdapter adapter = item.first;
    int position = item.second.getAdapterPosition();
    adapter.notifyItemChanged(position);
  }
}
