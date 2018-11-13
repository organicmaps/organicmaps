package com.mapswithme.maps.adapter;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.google.android.flexbox.FlexboxLayoutManager;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.CatalogTag;
import com.mapswithme.maps.bookmarks.data.CatalogTagsGroup;
import com.mapswithme.maps.widget.recycler.TagItemDecoration;
import com.mapswithme.maps.widget.recycler.UgcRouteTagItemDecorator;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;

public class TagsCompositeAdapter extends RecyclerView.Adapter<TagsCompositeAdapter.TagsRecyclerHolder>
{
  @NonNull
  private final Context mContext;
  @NonNull
  private final List<CatalogTagsGroup> mCatalogTagsGroups;
  @NonNull
  private final List<ComponentHolder> mComponentHolders;

  public TagsCompositeAdapter(@NonNull Context context,
                              @NonNull List<CatalogTagsGroup> groups,
                              @NonNull List<CatalogTag> savedState,
                              @NonNull OnItemClickListener<Pair<TagsAdapter, TagsAdapter.TagViewHolder>> clickListener)
  {
    mContext = context;
    mCatalogTagsGroups = groups;
    mComponentHolders = makeRecyclerComponents(context, groups, savedState, clickListener);
    setHasStableIds(true);
  }

  @NonNull
  private List<ComponentHolder> makeRecyclerComponents(@NonNull Context context,
                                                       @NonNull List<CatalogTagsGroup> groups,
                                                       @NonNull List<CatalogTag> savedState,
                                                       @NonNull OnItemClickListener<Pair<TagsAdapter, TagsAdapter.TagViewHolder>> externalListener)
  {
    List<ComponentHolder> result = new ArrayList<>();

    for (int i = 0; i < groups.size(); i++)
    {
      CatalogTagsGroup each = groups.get(i);
      TagsAdapter.SelectionState state = TagsAdapter.SelectionState.from(savedState, each);
      OnItemClickListener<TagsAdapter.TagViewHolder> listener = new TagsListClickListener(externalListener, i);
      TagsAdapter adapter = new TagsAdapter(listener, state, each.getTags());
      Resources res = context.getResources();
      Drawable divider = res.getDrawable(R.drawable.divider_transparent_base);
      TagItemDecoration decor = new UgcRouteTagItemDecorator(divider);

      ComponentHolder holder = new ComponentHolder(adapter, decor);
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
    holder.mRecycler.setLayoutManager(new FlexboxLayoutManager(mContext));
    holder.mRecycler.setItemAnimator(null);
    initDecor(holder, componentHolder);
    holder.mRecycler.setAdapter(componentHolder.mAdapter);
  }

  private void initDecor(@NonNull TagsRecyclerHolder holder,
                         @NonNull ComponentHolder componentHolder)
  {
    RecyclerView.ItemDecoration decor;
    int index = 0;
    while ((decor = holder.mRecycler.getItemDecorationAt(index)) != null)
    {
      holder.mRecycler.removeItemDecoration(decor);
      index++;
    }
    holder.mRecycler.addItemDecoration(componentHolder.mDecor);
  }

  @Override
  public int getItemCount()
  {
    return mCatalogTagsGroups.size();
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @NonNull
  public Collection<CatalogTag> getSelectedTags()
  {
    List<CatalogTag> tags = new ArrayList<>();
    for (ComponentHolder each : mComponentHolders)
    {
      tags.addAll(each.mAdapter.getSelectedTags());
    }
    return Collections.unmodifiableList(tags);
  }

  public boolean hasSelectedItems()
  {
    for (ComponentHolder each : mComponentHolders)
    {
      if (!each.mAdapter.getSelectedTags().isEmpty())
        return true;
    }
    return false;
  }

  private static class ComponentHolder
  {
    @NonNull
    private final TagsAdapter mAdapter;
    @NonNull
    private final RecyclerView.ItemDecoration mDecor;

    private ComponentHolder(@NonNull TagsAdapter adapter,
                            @NonNull RecyclerView.ItemDecoration decor)
    {
      mAdapter = adapter;
      mDecor = decor;
    }
  }

  static class TagsRecyclerHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final RecyclerView mRecycler;

    TagsRecyclerHolder(@NonNull View itemView)
    {
      super(itemView);
      mRecycler = itemView.findViewById(R.id.recycler);
    }
  }

  private class TagsListClickListener implements OnItemClickListener<TagsAdapter.TagViewHolder>
  {
    @NonNull
    private final OnItemClickListener<Pair<TagsAdapter, TagsAdapter.TagViewHolder>> mListener;
    private final int mIndex;

    TagsListClickListener(@NonNull OnItemClickListener<Pair<TagsAdapter, TagsAdapter.TagViewHolder>> clickListener,
                          int index)
    {
      mListener = clickListener;
      mIndex = index;
    }

    @Override
    public void onItemClick(@NonNull View v, @NonNull TagsAdapter.TagViewHolder item)
    {
      ComponentHolder components = mComponentHolders.get(mIndex);
      Pair<TagsAdapter, TagsAdapter.TagViewHolder> pair = new Pair<>(components.mAdapter, item);
      mListener.onItemClick(v, pair);
    }
  }
}
