package com.mapswithme.maps.search;

import android.content.res.Resources;
import android.content.res.TypedArray;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

public class CategoriesAdapter extends RecyclerView.Adapter<CategoriesAdapter.ViewHolder>
{
  private final int mCategoryResIds[];
  private final int mIconResIds[];

  private final LayoutInflater mInflater;
  private final Resources mResources;
  private final SearchFragment mFragment;

  public CategoriesAdapter(SearchFragment fragment)
  {
    TypedArray categories = fragment.getActivity().getResources().obtainTypedArray(R.array.search_category_name_ids);
    TypedArray icons = fragment.getActivity().getResources().obtainTypedArray(R.array.search_category_icon_ids);
    int len = categories.length();
    if (icons.length() != len)
      throw new IllegalStateException("Categories and icons arrays must have the same length.");

    mCategoryResIds = new int[len];
    mIconResIds = new int[len];
    for (int i = 0; i < len; i++)
    {
      mCategoryResIds[i] = categories.getResourceId(i, 0);
      mIconResIds[i] = icons.getResourceId(i, 0);
    }
    categories.recycle();
    icons.recycle();

    mFragment = fragment;
    mResources = mFragment.getResources();
    mInflater = LayoutInflater.from(mFragment.getActivity());
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    final View view = mInflater.inflate(R.layout.item_search_category, parent, false);
    return new ViewHolder(view);
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position)
  {
    UiUtils.setTextAndShow(holder.mName, mResources.getString(mCategoryResIds[position]));
    holder.mImageLeft.setImageResource(mIconResIds[position]);
  }

  @Override
  public int getItemCount()
  {
    return mCategoryResIds.length;
  }

  private String getSuggestionFromCategory(int resId)
  {
    return mResources.getString(resId) + ' ';
  }

  public class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener
  {
    public TextView mName;
    public ImageView mImageLeft;

    public ViewHolder(View v)
    {
      super(v);
      v.setOnClickListener(this);
      mName = (TextView) v.findViewById(R.id.tv__search_category);
      mImageLeft = (ImageView) v.findViewById(R.id.iv__search_category);
    }

    @Override
    public void onClick(View v)
    {
      final int position = getAdapterPosition();
      Statistics.INSTANCE.trackSearchCategoryClicked(mResources.getResourceEntryName(mCategoryResIds[position]));
      mFragment.setSearchQuery(getSuggestionFromCategory(mCategoryResIds[position]));
    }
  }
}