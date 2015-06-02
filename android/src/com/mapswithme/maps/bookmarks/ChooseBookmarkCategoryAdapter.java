package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RadioButton;
import android.widget.TextView;

import com.mapswithme.maps.R;

public class ChooseBookmarkCategoryAdapter extends AbstractBookmarkCategoryAdapter
{
  private int mCheckedPosition = -1;

  public ChooseBookmarkCategoryAdapter(Context context, int pos)
  {
    super(context);
    mCheckedPosition = pos;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    if (convertView == null)
    {
      convertView = LayoutInflater.from(getContext()).inflate(R.layout.item_set_chooser, parent, false);

      convertView.setTag(new SingleChoiceHolder((TextView) convertView.findViewById(R.id.sci_set_name),
          (RadioButton) convertView.findViewById(R.id.sci_checkbox)));
    }

    final SingleChoiceHolder holder = (SingleChoiceHolder) convertView.getTag();
    boolean checked = mCheckedPosition == position;
    holder.name.setText(getItem(position).getName());
    holder.name.setTextAppearance(getContext(), checked ? android.R.style.TextAppearance_Large : android.R.style.TextAppearance_Medium);
    holder.checked.setChecked(checked);

    return convertView;
  }

  private static class SingleChoiceHolder
  {
    TextView name;
    RadioButton checked;

    public SingleChoiceHolder(TextView name, RadioButton checked)
    {
      this.name = name;
      this.checked = checked;
    }
  }

  public void chooseItem(int position)
  {
    mCheckedPosition = position;
    notifyDataSetChanged();
  }

  public int getCheckedItemPosition()
  {
    return mCheckedPosition;
  }
}
