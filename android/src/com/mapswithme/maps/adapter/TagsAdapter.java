package com.mapswithme.maps.adapter;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.drawable.StateListDrawable;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.CatalogTag;
import com.mapswithme.maps.bookmarks.data.CatalogTagsGroup;
import com.mapswithme.maps.ugc.routes.TagsResFactory;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Objects;

public class TagsAdapter extends RecyclerView.Adapter<TagsAdapter.TagViewHolder>
{
  @NonNull
  private final SelectionState mState;

  @NonNull
  private final OnItemClickListener<TagViewHolder> mListener;

  @NonNull
  private final List<CatalogTag> mTags;

  TagsAdapter(@NonNull OnItemClickListener<TagViewHolder> listener, @NonNull SelectionState state,
              @NonNull List<CatalogTag> tags)
  {
    mListener = new ClickListenerWrapper(listener);
    mState = state;
    mTags = tags;
    setHasStableIds(true);
  }

  @Override
  public TagViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    View itemView = LayoutInflater.from(parent.getContext())
                                  .inflate(R.layout.tag_item, parent, false);
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
    Context context = holder.itemView.getContext();
    StateListDrawable selector = TagsResFactory.makeSelector(context, tag.getColor());
    holder.itemView.setBackgroundDrawable(selector);
    ColorStateList color = TagsResFactory.makeColor(context, tag.getColor());
    holder.mText.setTextColor(color);
    holder.mText.setText(tag.getLocalizedName());
  }

  @Override
  public int getItemCount()
  {
    return mTags.size();
  }

  @NonNull
  Collection<CatalogTag> getSelectedTags()
  {
    return Collections.unmodifiableCollection(mState.mTags);
  }

  static final class SelectionState
  {
    private SelectionState()
    {
    }

    @NonNull
    private final List<CatalogTag> mTags = new ArrayList<>();

    private void addAll(@NonNull CatalogTag... tags)
    {
      mTags.addAll(Arrays.asList(tags));
    }

    private void addAll(@NonNull List<CatalogTag> tags)
    {
      mTags.addAll(tags);
    }

    private boolean removeAll(@NonNull CatalogTag... tags)
    {
      return mTags.removeAll(Arrays.asList(tags));
    }

    private boolean remove(@NonNull List<CatalogTag> tags)
    {
      return mTags.removeAll(tags);
    }

    private boolean contains(@NonNull CatalogTag tag)
    {
      return mTags.contains(tag);
    }

    @NonNull
    public static SelectionState empty()
    {
      return new SelectionState();
    }

    @NonNull
    public static SelectionState from(@NonNull List<CatalogTag> savedTags,
                                      @NonNull CatalogTagsGroup src)
    {
      SelectionState state = empty();
      for (CatalogTag each : savedTags)
      {
        if (src.getTags().contains(each))
          state.addAll(each);
      }
      return state;
    }
  }

  public static class TagViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final TextView mText;
    @NonNull
    private final OnItemClickListener<TagViewHolder> mListener;
    @Nullable
    private CatalogTag mTag;

    TagViewHolder(@NonNull View itemView, @NonNull OnItemClickListener<TagViewHolder> listener)
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

  private class ClickListenerWrapper implements OnItemClickListener<TagViewHolder>
  {
    @NonNull
    private final OnItemClickListener<TagViewHolder> mListener;

    ClickListenerWrapper(@NonNull OnItemClickListener<TagViewHolder> listener)
    {
      mListener = listener;
    }

    @Override
    public void onItemClick(@NonNull View v, @NonNull TagViewHolder item)
    {
      if (mState.contains(item.getEntity()))
        mState.removeAll(item.getEntity());
      else
        mState.addAll(item.getEntity());

      mListener.onItemClick(v, item);
    }
  }
}
