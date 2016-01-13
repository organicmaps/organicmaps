package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;

import java.util.List;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Icon;

public class IconsAdapter extends ArrayAdapter<Icon>
{
  private String mCheckedIconType;

  public IconsAdapter(Context context, List<Icon> list)
  {
    super(context, 0, 0, list);
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    SpinnerViewHolder holder;
    if (convertView == null)
    {
      LayoutInflater inflater = LayoutInflater.from(getContext());
      convertView = inflater.inflate(R.layout.color_row, parent, false);
      holder = new SpinnerViewHolder(convertView);
      convertView.setTag(holder);
    }
    else
      holder = (SpinnerViewHolder) convertView.getTag();

    final Icon icon = getItem(position);
    if (icon.getType().equals(mCheckedIconType))
      holder.icon.setImageResource(getItem(position).getSelectedResId());
    else
      holder.icon.setImageResource(getItem(position).getResId());

    return convertView;
  }

  private static class SpinnerViewHolder
  {
    final ImageView icon;

    SpinnerViewHolder(View convertView)
    {
      icon = (ImageView) convertView.findViewById(R.id.iv__color);
    }
  }

  public void chooseItem(String position)
  {
    mCheckedIconType = position;
    notifyDataSetChanged();
  }
}
