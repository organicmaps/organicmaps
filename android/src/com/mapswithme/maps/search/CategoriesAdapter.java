package com.mapswithme.maps.search;

import android.content.res.Resources;
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
  private static final int mCategoriesIds[] = {
      R.string.food,
      R.string.hotel,
      R.string.tourism,
      R.string.wifi,
      R.string.transport,
      R.string.fuel,
      R.string.parking,
      R.string.shop,
      R.string.atm,
      R.string.bank,
      R.string.entertainment,
      R.string.hospital,
      R.string.pharmacy,
      R.string.police,
      R.string.toilet,
      R.string.post
  };
  private static final int mIcons[] = {
      R.drawable.ic_food,
      R.drawable.ic_hotel,
      R.drawable.ic_tourism,
      R.drawable.ic_wifi,
      R.drawable.ic_transport,
      R.drawable.ic_gas,
      R.drawable.ic_parking,
      R.drawable.ic_shop,
      R.drawable.ic_atm,
      R.drawable.ic_bank,
      R.drawable.ic_entertainment,
      R.drawable.ic_hospital,
      R.drawable.ic_pharmacy,
      R.drawable.ic_police,
      R.drawable.ic_toilet,
      R.drawable.ic_post
  };

  private final LayoutInflater mInflater;
  private final Resources mResources;
  private final SearchFragment mFragment;

  public CategoriesAdapter(SearchFragment fragment)
  {
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
    UiUtils.setTextAndShow(holder.mName, mResources.getString(mCategoriesIds[position]));
    holder.mImageLeft.setImageResource(mIcons[position]);
  }

  @Override
  public int getItemCount()
  {
    return mCategoriesIds.length;
  }

  private String getSuggestionFromCategoryId(int resId)
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
      Statistics.INSTANCE.trackSearchCategoryClicked(mResources.getResourceEntryName(mCategoriesIds[position]));
      mFragment.setSearchQuery(getSuggestionFromCategoryId(mCategoriesIds[position]));
    }
  }
}