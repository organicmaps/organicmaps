package com.mapswithme.maps.ugc.routes;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
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
import java.util.List;

class TagsCompositeAdapter extends RecyclerView.Adapter<TagsCompositeAdapter.TagsRecyclerHolder>
{
  @NonNull
  private final Context mContext;
  @NonNull
  private final List<CatalogTagsGroup> mCatalogTagsGroups;
  @NonNull
  private final List<ComponentHolder> mComponentHolders;

  TagsCompositeAdapter(@NonNull Context context,
                       @NonNull List<CatalogTagsGroup> groups,
                       @NonNull List<CatalogTag> savedState)
  {
    mContext = context;
    mCatalogTagsGroups = groups;
    mComponentHolders = makeComponents(context, groups, savedState);
    setHasStableIds(true);
  }

  @NonNull
  private static List<ComponentHolder> makeComponents(@NonNull Context context,
                                                      @NonNull List<CatalogTagsGroup> groups,
                                                      @NonNull List<CatalogTag> savedState)
  {
    List<ComponentHolder> result = new ArrayList<>();

    for (CatalogTagsGroup each : groups)
    {
      TagsAdapter.SelectionState state = TagsAdapter.SelectionState.from(savedState, each);
      TagsAdapter adapter = new TagsAdapter((v, item) -> {}, state);
      adapter.setTags(each.getTags());
      Resources res = context.getResources();
      Drawable divider = res.getDrawable(R.drawable.flexbox_divider);
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
    CatalogTagsGroup group = mCatalogTagsGroups.get(position);
    componentHolder.mAdapter.setTags(group.getTags());
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
  Collection<CatalogTag> getSelectedTags()
  {
    List<CatalogTag> tags = new ArrayList<>();
    for (ComponentHolder each : mComponentHolders)
    {
      tags.addAll(each.mAdapter.getSelectedTags());
    }
    return tags;
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
}
