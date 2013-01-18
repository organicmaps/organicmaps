package com.mapswithme.maps.pins;

import java.util.List;
import java.util.jar.Attributes.Name;

import com.mapswithme.maps.R;
import com.mapswithme.maps.pins.pins.Pin;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

public class PinAdapter extends ArrayAdapter<Pin>
{

  public PinAdapter(Context context, List<Pin> objects)
  {
    super(context, 0, objects);
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    if (convertView == null)
    {
      convertView = LayoutInflater.from(getContext()).inflate(R.layout.pin_item, null);
      convertView.setTag(new PinHolder((ImageView) convertView.findViewById(R.id.pi_arrow), (ImageView) convertView
          .findViewById(R.id.pi_pin_color), (TextView) convertView.findViewById(R.id.pi_name), (TextView) convertView
          .findViewById(R.id.pi_distance)));
    }
    Pin item = getItem(position);
    PinHolder holder = (PinHolder) convertView.getTag();
    holder.name.setText(item.getName());
    holder.icon.setImageBitmap(item.getIcon().getIcon());
    return convertView;
  }

  private static class PinHolder
  {
    ImageView arrow;
    ImageView icon;
    TextView name;
    TextView distance;

    public PinHolder(ImageView arrow, ImageView icon, TextView name, TextView distance)
    {
      super();
      this.arrow = arrow;
      this.icon = icon;
      this.name = name;
      this.distance = distance;
    }
  }
}
