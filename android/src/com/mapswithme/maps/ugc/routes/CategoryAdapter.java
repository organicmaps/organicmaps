package com.mapswithme.maps.ugc.routes;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.CatalogTagsGroup;

import java.util.List;

class CategoryAdapter extends RecyclerView.Adapter<CategoryAdapter.CategoryViewHolder>
{
  @NonNull
  private final List<CatalogTagsGroup> mTagsGroups;

  CategoryAdapter(@NonNull List<CatalogTagsGroup> tagsGroups)
  {
    mTagsGroups = tagsGroups;
    setHasStableIds(true);
  }

  @Override
  public CategoryViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    View itemView = inflater.inflate(R.layout.tags_category, parent, false);
    return new CategoryViewHolder(itemView);
  }

  @Override
  public void onBindViewHolder(CategoryViewHolder holder, int position)
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

  static final class CategoryViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final TextView mText;

    CategoryViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mText = itemView.findViewById(R.id.text);
    }
  }
}
