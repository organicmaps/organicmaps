package com.mapswithme.maps.adapter;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.CatalogTagsGroup;

import java.util.List;

public class TagGroupNameAdapter extends RecyclerView.Adapter<TagGroupNameAdapter.TagGroupNameHolder>
{
  @NonNull
  private final List<CatalogTagsGroup> mTagsGroups;

  public TagGroupNameAdapter(@NonNull List<CatalogTagsGroup> tagsGroups)
  {
    mTagsGroups = tagsGroups;
    setHasStableIds(true);
  }

  @Override
  public TagGroupNameHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    View itemView = inflater.inflate(R.layout.tags_category, parent, false);
    return new TagGroupNameHolder(itemView);
  }

  @Override
  public void onBindViewHolder(TagGroupNameHolder holder, int position)
  {
    CatalogTagsGroup item = mTagsGroups.get(position);
    holder.mText.setText(item.getLocalizedName());
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public int getItemCount()
  {
    return mTagsGroups.size();
  }

  static final class TagGroupNameHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final TextView mText;

    TagGroupNameHolder(@NonNull View itemView)
    {
      super(itemView);
      mText = itemView.findViewById(R.id.text);
    }
  }
}
