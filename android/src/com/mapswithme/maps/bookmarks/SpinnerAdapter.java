package com.mapswithme.maps.bookmarks;

import java.util.List;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Icon;

public class SpinnerAdapter extends ArrayAdapter<Icon>
{

  public SpinnerAdapter(Context context, List<Icon> set)
  {
    super(context, 0, 0, set);
  }

  @Override
  public View getDropDownView(int position, View convertView, ViewGroup parent)
  {
    return getCustomView(position, convertView, parent);
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    return getCustomView(position, convertView, parent);
  }

  public View getCustomView(int position, View convertView, ViewGroup parent)
  {
    if (convertView == null)
    {
      LayoutInflater inflater = LayoutInflater.from(getContext());
      convertView = inflater.inflate(R.layout.pins_row, parent, false);
      convertView.setTag(new SpinnerViewHolder((TextView) convertView.findViewById(R.id.row_color_name),
                                               ((ImageView) convertView.findViewById(R.id.row_image))));
    }
    SpinnerViewHolder holder = (SpinnerViewHolder) convertView.getTag();
    holder.name.setText(getItem(position).getName());
    holder.icon.setImageBitmap(getItem(position).getIcon());

    return convertView;
  }

  private class SpinnerViewHolder
  {
    TextView name;
    ImageView icon;

    public SpinnerViewHolder(TextView name, ImageView icon)
    {
      super();
      this.name = name;
      this.icon = icon;
    }

  }
}
