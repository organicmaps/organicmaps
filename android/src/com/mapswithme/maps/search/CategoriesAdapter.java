package com.mapswithme.maps.search;

import android.content.res.Resources;
import android.content.res.TypedArray;
import android.support.annotation.DrawableRes;
import android.support.annotation.StringRes;
import android.support.v4.app.Fragment;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.statistics.Statistics;

class CategoriesAdapter extends RecyclerView.Adapter<CategoriesAdapter.ViewHolder>
{
  private final int mCategoryResIds[];
  private final int mIconResIds[];

  private final LayoutInflater mInflater;
  private final Resources mResources;

  interface OnCategorySelectedListener
  {
    void onCategorySelected(String category);
  }

  private OnCategorySelectedListener mListener;

  CategoriesAdapter(Fragment fragment)
  {
    TypedArray categories = fragment.getActivity().getResources().obtainTypedArray(R.array.search_category_name_ids);
    TypedArray icons = fragment.getActivity().getResources().obtainTypedArray(ThemeUtils.isNightTheme() ? R.array.search_category_icon_night_ids
                                                                                                        : R.array.search_category_icon_ids);
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

    if (fragment instanceof OnCategorySelectedListener)
      mListener = (OnCategorySelectedListener) fragment;
    mResources = fragment.getResources();
    mInflater = LayoutInflater.from(fragment.getActivity());
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
    holder.setTextAndIcon(mCategoryResIds[position], mIconResIds[position]);
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
    private final TextView mTitle;

    ViewHolder(View v)
    {
      super(v);
      v.setOnClickListener(this);
      mTitle = (TextView) v;
    }

    @Override
    public void onClick(View v)
    {
      final int position = getAdapterPosition();
      Statistics.INSTANCE.trackSearchCategoryClicked(mResources.getResourceEntryName(mCategoryResIds[position]));
      if (mListener != null)
        mListener.onCategorySelected(getSuggestionFromCategory(mCategoryResIds[position]));
    }

    void setTextAndIcon(@StringRes int textResId, @DrawableRes int iconResId)
    {
      mTitle.setText(textResId);
      mTitle.setCompoundDrawablesWithIntrinsicBounds(iconResId, 0, 0, 0);
    }
  }
}