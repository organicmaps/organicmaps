package com.mapswithme.maps.pins;

import java.util.List;

import com.mapswithme.maps.R;
import com.mapswithme.maps.pins.pins.PinSet;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

public class PinSetsAdapter extends AbstractPinSetAdapter
{

  public PinSetsAdapter(Context context, List<PinSet> objects)
  {
    super(context, objects);
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    if (convertView == null)
    {
      convertView = LayoutInflater.from(getContext()).inflate(R.layout.pinsets_item, null);
      convertView.setTag(new PinSetHolder((ImageView) convertView.findViewById(R.id.psi_visible),
          (TextView) convertView.findViewById(R.id.psi_name), (TextView) convertView.findViewById(R.id.psi_set_size)));
    }
    PinSetHolder psh = (PinSetHolder) convertView.getTag();
    PinSet set = getItem(position);
    if (set.isVisible())
    {

    } else
    {

    }
    psh.name.setText(set.getName());
    psh.size.setText(String.valueOf(set.getPins().size()));
    if (getItem(position).isVisible())
    {
      psh.visibility.setImageResource(R.drawable.eye);
    } else
    {
      psh.visibility.setImageDrawable(null);
    }
    return convertView;
  }

  static class PinSetHolder
  {
    ImageView visibility;
    TextView name;
    TextView size;

    public PinSetHolder(ImageView visibility, TextView name, TextView size)
    {
      super();
      this.visibility = visibility;
      this.name = name;
      this.size = size;
    }

  }
}
