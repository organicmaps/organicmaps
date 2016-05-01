package com.mapswithme.maps.editor;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.editor.data.FeatureCategory;
import com.mapswithme.util.UiUtils;

public class FeatureCategoryAdapter extends RecyclerView.Adapter<FeatureCategoryAdapter.FeatureViewHolder>
{
  private FeatureCategory[] mCategories;
  private final FeatureCategoryFragment mFragment;
  private FeatureCategory mSelectedCategory;

  public FeatureCategoryAdapter(@NonNull FeatureCategoryFragment host, @NonNull FeatureCategory[] categories, @Nullable FeatureCategory category)
  {
    mFragment = host;
    mCategories = categories;
    mSelectedCategory = category;
  }

  public void setCategories(FeatureCategory[] categories)
  {
    mCategories = categories;
    notifyDataSetChanged();
  }

  @Override
  public FeatureViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return new FeatureViewHolder(LayoutInflater.from(parent.getContext()).inflate(R.layout.item_feature_category, parent, false));
  }

  @Override
  public void onBindViewHolder(FeatureViewHolder holder, int position)
  {
    holder.bind(position);
  }

  @Override
  public int getItemCount()
  {
    return mCategories.length;
  }

  protected class FeatureViewHolder extends RecyclerView.ViewHolder
  {
    TextView name;
    View selected;

    public FeatureViewHolder(View itemView)
    {
      super(itemView);
      name = (TextView) itemView.findViewById(R.id.name);
      selected = itemView.findViewById(R.id.selected);
      UiUtils.hide(selected);
      itemView.setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          onCategorySelected(getAdapterPosition());
        }
      });
    }

    public void bind(int position)
    {
      name.setText(mCategories[position].name);
      UiUtils.showIf(mSelectedCategory != null && mCategories[position].category == mSelectedCategory.category, selected);
    }
  }

  private void onCategorySelected(int adapterPosition)
  {
    mFragment.selectCategory(mCategories[adapterPosition]);
  }
}
