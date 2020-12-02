package com.mapswithme.maps.widget.placepage;

import android.content.res.Resources;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;

import java.util.ArrayList;
import java.util.List;

class NearbyAdapter extends BaseAdapter
{
  @NonNull
  private List<Sponsored.NearbyObject> mItems = new ArrayList<>();
  @Nullable
  private final OnItemClickListener mListener;

  NearbyAdapter(@Nullable OnItemClickListener listener)
  {
    mListener = listener;
  }

  interface OnItemClickListener
  {
    void onItemClick(@NonNull Sponsored.NearbyObject item);
  }

  @Override
  public int getCount()
  {
    return mItems.size();
  }

  @Override
  public Object getItem(int position)
  {
    return mItems.get(position);
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    ViewHolder holder;
    if (convertView == null)
    {
      convertView = LayoutInflater.from(parent.getContext())
                                  .inflate(R.layout.item_nearby, parent, false);
      holder = new ViewHolder(convertView, mListener);
      convertView.setTag(holder);
    } else
    {
      holder = (ViewHolder) convertView.getTag();
    }

    holder.bind(mItems.get(position));

    return convertView;
  }

  public void setItems(@NonNull List<Sponsored.NearbyObject> items)
  {
    this.mItems = items;
    notifyDataSetChanged();
  }

  private static class ViewHolder implements View.OnClickListener
  {
    @Nullable
    final OnItemClickListener mListener;
    @NonNull
    ImageView mIcon;
    @NonNull
    TextView mTitle;
    @NonNull
    TextView mType;
    @NonNull
    TextView mDistance;
    @Nullable
    Sponsored.NearbyObject mItem;

    public ViewHolder(View view, @Nullable OnItemClickListener listener)
    {
      mListener = listener;
      mIcon = (ImageView) view.findViewById(R.id.iv__icon);
      mTitle = (TextView) view.findViewById(R.id.tv__title);
      mType = (TextView) view.findViewById(R.id.tv__type);
      mDistance = (TextView) view.findViewById(R.id.tv__distance);
      view.setOnClickListener(this);
    }

    @Override
    public void onClick(View v)
    {
      if (mListener != null && mItem != null)
        mListener.onItemClick(mItem);
    }

    public void bind(@NonNull Sponsored.NearbyObject item)
    {
      mItem = item;
      String packageName = mType.getContext().getPackageName();
      final boolean isNightTheme = ThemeUtils.isNightTheme(mType.getContext());
      Resources resources = mType.getResources();
      int categoryRes = resources.getIdentifier(item.getCategory(), "string", packageName);
      if (categoryRes == 0)
        throw new IllegalStateException("Can't get string resource id for category:" + item.getCategory());

      String iconId = "ic_category_" + item.getCategory();
      if (isNightTheme)
        iconId = iconId + "_night";
      int iconRes = resources.getIdentifier(iconId, "drawable", packageName);
      if (iconRes == 0)
        throw new IllegalStateException("Can't get icon resource id:" + iconId);
      mIcon.setImageResource(iconRes);
      mTitle.setText(item.getTitle());
      mType.setText(categoryRes);
      mDistance.setText(item.getDistance());
    }
  }
}
