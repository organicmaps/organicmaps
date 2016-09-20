package com.mapswithme.maps.widget.placepage;

import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;

import android.content.res.Resources;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

class NearbyAdapter extends BaseAdapter {

  NearbyAdapter(OnItemClickListener listener) {
    mListener = listener;
  }

  interface OnItemClickListener {
    void onItemClick(SponsoredHotel.NearbyObject item);
  }

  private List<SponsoredHotel.NearbyObject> items = new ArrayList<>();
  private final OnItemClickListener mListener;

  @Override
  public int getCount() {
    return items.size();
  }

  @Override
  public Object getItem(int position) {
    return items.get(position);
  }

  @Override
  public long getItemId(int position) {
    return position;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent) {
    ViewHolder holder;
    if (convertView == null) {
      convertView = LayoutInflater.from(parent.getContext())
              .inflate(R.layout.item_nearby, parent, false);
      holder = new ViewHolder(convertView, mListener);
      convertView.setTag(holder);
    } else {
      holder = (ViewHolder) convertView.getTag();
    }

    holder.bind(items.get(position));

    return convertView;
  }

  public void setItems(
          List<SponsoredHotel.NearbyObject> items) {
    this.items = items;
    notifyDataSetChanged();
  }

  private static class ViewHolder implements View.OnClickListener {
    final OnItemClickListener mListener;
    ImageView mIcon;
    TextView mTitle;
    TextView mType;
    TextView mDistance;
    SponsoredHotel.NearbyObject mItem;

    public ViewHolder(View view, OnItemClickListener listener) {
      mListener = listener;
      mIcon = (ImageView) view.findViewById(R.id.iv__icon);
      mTitle = (TextView) view.findViewById(R.id.tv__title);
      mType = (TextView) view.findViewById(R.id.tv__type);
      mDistance = (TextView) view.findViewById(R.id.tv__distance);
      view.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
      if (mListener != null) {
        mListener.onItemClick(mItem);
      }
    }

    public void bind(SponsoredHotel.NearbyObject item) {
      mItem = item;
      String packageName = mType.getContext().getPackageName();
      final boolean isNightTheme = ThemeUtils.isNightTheme();
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
