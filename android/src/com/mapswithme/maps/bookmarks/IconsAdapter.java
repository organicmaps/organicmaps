package com.mapswithme.maps.bookmarks;

import java.util.List;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.RadioButton;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Icon;

//SingleChoise list view don't add radiobutton to custom view
public class IconsAdapter extends ArrayAdapter<Icon> implements Chooseable
{
  private int mCheckedPosition = 0;
  public IconsAdapter(Context context, List<Icon> list)
  {
    super(context, 0, 0, list);
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    if (convertView == null)
    {
      LayoutInflater inflater = LayoutInflater.from(getContext());
      convertView = inflater.inflate(R.layout.color_row, parent, false);
      convertView.setTag(new SpinnerViewHolder((TextView) convertView.findViewById(R.id.row_color_name),
                                               (ImageView) convertView.findViewById(R.id.row_color_image),
                                               (RadioButton)convertView.findViewById(R.id.row_color_checked)
                                               ));
    }
    SpinnerViewHolder holder = (SpinnerViewHolder) convertView.getTag();
 //   holder.name.setText(getItem(position).getName());
    holder.icon.setImageBitmap(getItem(position).getIcon());
    holder.checked.setChecked(position == mCheckedPosition);
    return convertView;
  }

  private class SpinnerViewHolder
  {
//    TextView name;
    ImageView icon;
    RadioButton checked;

    public SpinnerViewHolder(TextView name, ImageView icon, RadioButton check)
    {
//      this.name = name;
      this.icon = icon;
      this.checked = check;
      checked.setVisibility(View.VISIBLE);
    }

  }

  @Override
  public void chooseItem(int position)
  {
    mCheckedPosition = position;
    notifyDataSetChanged();
  }

  @Override
  public int getCheckedItemPosition()
  {
    return mCheckedPosition;
  }
}
