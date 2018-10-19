package com.mapswithme.maps.ugc.routes;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.StateListDrawable;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.google.android.flexbox.FlexboxLayoutManager;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.bookmarks.OnItemClickListener;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.CatalogTag;
import com.mapswithme.maps.bookmarks.data.CatalogTagsGroup;
import com.mapswithme.maps.widget.recycler.TagItemDecoration;
import com.mapswithme.maps.widget.recycler.UgcRouteTagItemDecorator;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Objects;

public class UgcRoutesFragment extends BaseMwmFragment implements BookmarkManager.BookmarksCatalogListener

{
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
  private List<TagsAdapter> mAdapter = new ArrayList<>();

  @SuppressWarnings("NullableProblems")
  @NonNull
  private ViewGroup mTagsContainer;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    LayoutInflater layoutInflater = LayoutInflater.from(getContext());
    ViewGroup root = (ViewGroup) layoutInflater.inflate(R.layout.ugc_routes_frag, container,
                                                         false);
    setHasOptionsMenu(true);
    mProgress = root.findViewById(R.id.progress_container);
    mTagsContainer = root.findViewById(R.id.tags_container);
    mRetryBtnContainer = root.findViewById(R.id.retry_btn_container);
    mRecycler = root.findViewById(R.id.recycler);
    View retryBtn = mRetryBtnContainer.findViewById(R.id.retry_btn);
    retryBtn.setOnClickListener(v -> onRetryClicked());
    UiUtils.hide(mTagsContainer, mRetryBtnContainer);
    UiUtils.show(mProgress);
    BookmarkManager.INSTANCE.requestRouteTags();
    return root;
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
      ArrayList<CatalogTag> catalogTags = new ArrayList<CatalogTag>();
      for (TagsAdapter each : mAdapter)
      {
        catalogTags.addAll(each.getSelectedTags());
      }
      Intent result = new Intent().putParcelableArrayListExtra(UgcRouteTagsActivity.EXTRA_TAGS, catalogTags);
      getActivity().setResult(Activity.RESULT_OK, result);
      getActivity().finish();
      return true;
    }

    return super.onOptionsItemSelected(item);
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

  }

  @Override
  public void onImportFinished(@NonNull String serverId, long catId, boolean successful)
  {

  }

  @Override
  public void onTagsReceived(boolean successful, @NonNull CatalogTagsGroup[] tagsGroups)
  {
    UiUtils.showIf(successful && tagsGroups.length != 0, mTagsContainer);
    UiUtils.hideIf(successful && tagsGroups.length != 0, mRetryBtnContainer);
    UiUtils.hide(mProgress);

    if (tagsGroups.length == 0)
      return;
    addTags(tagsGroups);
  }


  private static final class CategoryViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final TextView mText;

    public CategoryViewHolder(View itemView)
    {
      super(itemView);
      mText = itemView.findViewById(R.id.text);
    }
  }

  private static class CategoryAdapter extends RecyclerView.Adapter<CategoryViewHolder>
  {
    @NonNull
    private final List<CatalogTagsGroup> mCatalogTagsGroups;

    private CategoryAdapter(@NonNull List<CatalogTagsGroup> catalogTagsGroups)
    {
      mCatalogTagsGroups = catalogTagsGroups;
    }

    @Override
    public CategoryViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
    {
      LayoutInflater inflater = LayoutInflater.from(parent.getContext());
      return new CategoryViewHolder(inflater.inflate(R.layout.tags_category, parent, false));
    }

    @Override
    public void onBindViewHolder(CategoryViewHolder holder, int position)
    {
      CatalogTagsGroup item = mCatalogTagsGroups.get(position);
      holder.mText.setText(item.getLocalizedName());
    }

    @Override
    public int getItemCount()
    {
      return mCatalogTagsGroups.size();
    }
  }


  private static class TagsRecyclerHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final RecyclerView mRecyclerView;

    public TagsRecyclerHolder(View itemView)
    {
      super(itemView);
      mRecyclerView = itemView.findViewById(R.id.recycler);
    }
  }

  private static class TagsCompositeAdapter extends RecyclerView.Adapter<TagsRecyclerHolder>
  {
    @NonNull
    private final Context mContext;
    @NonNull
    private final List<CatalogTagsGroup> mCatalogTagsGroups;
    private final List<ComponentHolder> mComponentHolders;

    private TagsCompositeAdapter(@NonNull Context context,
                                 @NonNull List<CatalogTagsGroup> catalogTagsGroups)
    {
      mContext = context;
      mCatalogTagsGroups = catalogTagsGroups;
      mComponentHolders = makeComponents(context, catalogTagsGroups);
    }

    @NonNull
    private static List<ComponentHolder> makeComponents(@NonNull Context context,
                                                        @NonNull List<CatalogTagsGroup> groups)
    {

      List<ComponentHolder> result = new ArrayList<>();

      for (CatalogTagsGroup each : groups)
      {
        TagsAdapter adapter = new TagsAdapter((v, item) -> {});
        adapter.setTags(each.getTags());
        FlexboxLayoutManager layoutManager = new FlexboxLayoutManager(context);
        Resources res = context.getResources();
        Drawable divider = res.getDrawable(R.drawable.flexbox_divider);
        TagItemDecoration decor = new UgcRouteTagItemDecorator(divider);

        ComponentHolder holder = new ComponentHolder(adapter, layoutManager, decor);
        result.add(holder);
      }
      return result;
    }

    @Override
    public TagsRecyclerHolder onCreateViewHolder(ViewGroup parent, int viewType)
    {
      LayoutInflater inflater = LayoutInflater.from(parent.getContext());
      return new TagsRecyclerHolder(inflater.inflate(R.layout.tags_recycler, parent, false));
    }

    @Override
    public void onBindViewHolder(TagsRecyclerHolder holder, int position)
    {
      ComponentHolder componentHolder = mComponentHolders.get(position);
      holder.mRecyclerView.setLayoutManager(new FlexboxLayoutManager(mContext));
      holder.mRecyclerView.setItemAnimator(null);
      initDecor(holder, componentHolder);
      componentHolder.mAdapter.setTags(mCatalogTagsGroups.get(position).getTags());
      holder.mRecyclerView.setAdapter(componentHolder.mAdapter);
    }

    private void initDecor(TagsRecyclerHolder holder, ComponentHolder componentHolder)
    {
      RecyclerView.ItemDecoration itemDecoration;
      int i = 0;
      while ((itemDecoration = holder.mRecyclerView.getItemDecorationAt(i)) != null )
      {
        holder.mRecyclerView.removeItemDecoration(itemDecoration);
        i++;
      }
      holder.mRecyclerView.addItemDecoration(componentHolder.mDecoration);
    }

    @Override
    public int getItemCount()
    {
      return mCatalogTagsGroups.size();
    }


    private static class ComponentHolder
    {
      @NonNull
      private final TagsAdapter mAdapter;
      @NonNull
      private final RecyclerView.LayoutManager mLayoutManager;
      @NonNull
      private final RecyclerView.ItemDecoration mDecoration;

      private ComponentHolder(@NonNull TagsAdapter adapter,
                              @NonNull RecyclerView.LayoutManager layoutManager,
                              @NonNull RecyclerView.ItemDecoration decoration)
      {
        mAdapter = adapter;
        mLayoutManager = layoutManager;
        mDecoration = decoration;
      }
    }

  }

  private void addTags(@NonNull CatalogTagsGroup[] tagsGroups)
  {
    List<CatalogTagsGroup> catalogTagsGroups = new ArrayList<>(Arrays.asList(tagsGroups));
    CatalogTagsGroup[] copyOf = Arrays.copyOf(tagsGroups, tagsGroups.length);
    catalogTagsGroups.addAll(Arrays.asList(copyOf));
    catalogTagsGroups.addAll(Arrays.asList(copyOf));
    CategoryAdapter categoryAdapter = new CategoryAdapter(catalogTagsGroups);
    TagsCompositeAdapter tagsCompositeAdapter = new TagsCompositeAdapter(getContext(),
                                                                         catalogTagsGroups);
    RecyclerCompositeAdapter.RepeatablePairIndexConverter converter = new
        RecyclerCompositeAdapter.RepeatablePairIndexConverter(categoryAdapter, tagsCompositeAdapter);

    RecyclerCompositeAdapter compositeAdapter = new RecyclerCompositeAdapter(converter,
                                                                             categoryAdapter,
                                                                             tagsCompositeAdapter);
    mRecycler.setItemAnimator(null);
    mRecycler.setLayoutManager(new LinearLayoutManager(getContext(), LinearLayoutManager.VERTICAL, false));
    mRecycler.setAdapter(compositeAdapter);

 /*   for (CatalogTagsGroup tagsGroup : tagsGroups)
    {
      RecyclerView recycler = (RecyclerView) getLayoutInflater().inflate(R.layout.tags_recycler,
                                                                         mTagsContainer,
                                                                         false);

      mTagsContainer.addView(recycler);
      FlexboxLayoutManager layoutManager = new FlexboxLayoutManager(getContext());
      Resources res = getResources();
      Drawable divider = res.getDrawable(R.drawable.flexbox_divider);
      TagItemDecoration decor = new UgcRouteTagItemDecorator(divider);
      recycler.addItemDecoration(decor);
      recycler.setLayoutManager(layoutManager);
      final TagsAdapter adapter = new TagsAdapter((v, item) -> {});
      recycler.setAdapter(adapter);
      recycler.setItemAnimator(null);
      recycler.setNestedScrollingEnabled(false);
      adapter.setTags(tagsGroup.getTags());
      mAdapter.add(adapter);
    }*/
  }

  @Override
  public void onUploadStarted(long originCategoryId)
  {

  }

  @Override
  public void onUploadFinished(int uploadResult, @NonNull String description,
                               long originCategoryId, long resultCategoryId)
  {

  }

  private static class TagsAdapter extends RecyclerView.Adapter<TagViewHolder>
  {
    @NonNull
    private final SelectionState mState;

    @NonNull
    private final OnItemClickListener<TagViewHolder> mListener;

    @NonNull
    private List<CatalogTag> mTags = Collections.emptyList();

    TagsAdapter(@NonNull OnItemClickListener<TagViewHolder> listener)
    {
      mListener = new ClickListenerWrapper(listener);
      mState = new SelectionState();
      setHasStableIds(true);
    }

    @Override
    public TagViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
    {
      View itemView = LayoutInflater.from(parent.getContext()).inflate(R.layout.tag_item, parent, false);
      return new TagViewHolder(itemView, mListener);
    }

    @Override
    public long getItemId(int position)
    {
      return mTags.get(position).hashCode();
    }

    @Override
    public void onBindViewHolder(TagViewHolder holder, int position)
    {
      CatalogTag tag = mTags.get(position);
      holder.itemView.setSelected(mState.contains(tag));
      holder.mTag = tag;
      StateListDrawable selector = TagsResFactory.makeSelector(holder.itemView.getContext(),tag.getColor());
      holder.itemView.setBackgroundDrawable(selector);
      ColorStateList color = TagsResFactory.makeColor(holder.mText.getContext(), tag.getColor());
      holder.mText.setTextColor(color);
      holder.mText.setText(tag.getLocalizedName());
    }

    @Override
    public int getItemCount()
    {
      return mTags.size();
    }

    public void setTags(@NonNull List<CatalogTag> tags)
    {
      mTags = tags;
      notifyDataSetChanged();
    }

    Collection<CatalogTag> getSelectedTags()
    {
      return Collections.unmodifiableCollection(mTags);
    }

    private static final class SelectionState {
      @NonNull
      private final List<CatalogTag> mTags = new ArrayList<>();

      private void add(@NonNull CatalogTag tag){
        mTags.add(tag);
      }

      private boolean remove(@NonNull CatalogTag tag)
      {
        return mTags.remove(tag);
      }

      private boolean contains(@NonNull CatalogTag tag)
      {
        return mTags.contains(tag);
      }
    }

    private class ClickListenerWrapper implements OnItemClickListener<TagViewHolder>
    {
      @NonNull
      private final OnItemClickListener<TagViewHolder> mListener;

      public ClickListenerWrapper(@NonNull OnItemClickListener<TagViewHolder> listener)
      {
        mListener = listener;
      }

      @Override
      public void onItemClick(@NonNull View v, @NonNull TagViewHolder item)
      {
        if (mState.contains(item.getEntity()))
          mState.remove(item.getEntity());
        else
          mState.add(item.getEntity());

        mListener.onItemClick(v, item);

        notifyItemChanged(item.getAdapterPosition());
      }
    }
  }

  static class TagViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final TextView mText;
    @NonNull
    private final OnItemClickListener<TagViewHolder> mListener;
    @Nullable
    private CatalogTag mTag;

    TagViewHolder(View itemView, @NonNull OnItemClickListener<TagViewHolder> listener)
    {
      super(itemView);
      mText = itemView.findViewById(R.id.text);
      mListener = listener;
      itemView.setOnClickListener(v -> mListener.onItemClick(v, this));
    }

    @NonNull
    private CatalogTag getEntity()
    {
      return Objects.requireNonNull(mTag);
    }
  }
}
