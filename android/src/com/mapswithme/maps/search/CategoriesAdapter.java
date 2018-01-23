package com.mapswithme.maps.search;

import android.content.res.Resources;
import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;
import android.support.v4.app.Fragment;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.statistics.Statistics;

class CategoriesAdapter extends RecyclerView.Adapter<CategoriesAdapter.ViewHolder>
{
  private final int mCategoryResIds[];
  private final int mIconResIds[];

  private final LayoutInflater mInflater;
  private final Resources mResources;

  static final String LUGGAGE_CATEGORY = "luggagehero";

  interface OnCategorySelectedListener
  {
    void onCategorySelected(String category);
  }

  private OnCategorySelectedListener mListener;

  CategoriesAdapter(Fragment fragment)
  {
    final String packageName = fragment.getActivity().getPackageName();
    final boolean isNightTheme = ThemeUtils.isNightTheme();
    final Resources resources = fragment.getActivity().getResources();

    final String[] keys = DisplayedCategories.getKeys();
    final int numKeys = keys.length;

    mCategoryResIds = new int[numKeys];
    mIconResIds = new int[numKeys];
    for (int i = 0; i < numKeys; i++)
    {
      String key = keys[i];

      mCategoryResIds[i] = resources.getIdentifier(key, "string", packageName);
      if (mCategoryResIds[i] == 0)
      {
        if (key.equals(LUGGAGE_CATEGORY))
        {
          Statistics.INSTANCE.trackSponsoredEventForCustomProvider(
              Statistics.EventName.SEARCH_SPONSOR_CATEGORY_SHOWN,
              Statistics.ParamValue.LUGGAGE_HERO);
          mCategoryResIds[i] = R.string.luggage_storage;
        }
        else
        {
          throw new IllegalStateException("Can't get string resource id for category:" + key);
        }
      }

      String iconId = "ic_category_" + key;
      if (isNightTheme)
        iconId = iconId + "_night";
      mIconResIds[i] = resources.getIdentifier(iconId, "drawable", packageName);
      if (mIconResIds[i] == 0)
        throw new IllegalStateException("Can't get icon resource id:" + iconId);
    }

    if (fragment instanceof OnCategorySelectedListener)
      mListener = (OnCategorySelectedListener) fragment;
    mResources = fragment.getResources();
    mInflater = LayoutInflater.from(fragment.getActivity());
  }

  @Override
  public int getItemViewType(int position)
  {
    if (mCategoryResIds[position] == R.string.luggage_storage)
      return R.layout.item_search_category_luggage;
    return R.layout.item_search_category;
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    final View view;
    if (viewType == R.layout.item_search_category_luggage)
    {
      view = mInflater.inflate(R.layout.item_search_category_luggage, parent, false);
      return new ViewHolder(view, (TextView) view.findViewById(R.id.tv__category));
    }

    view = mInflater.inflate(R.layout.item_search_category, parent, false);
    return new ViewHolder(view, (TextView)view);
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
    if (resId == R.string.luggage_storage)
      return LUGGAGE_CATEGORY + ' ';
    return mResources.getString(resId) + ' ';
  }

  public class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener
  {
    @NonNull
    private final TextView mTitle;

    ViewHolder(@NonNull View v, @NonNull TextView tv)
    {
      super(v);
      v.setOnClickListener(this);
      mTitle = tv;
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
