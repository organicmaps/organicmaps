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
  private int mCheckedPosition;

  public static final int VIEW_TYPE_CATEGORY = 0;
  public static final int VIEW_TYPE_ADD_NEW = 1;

  public ChooseBookmarkCategoryAdapter(Context context, int pos)
  {
    super(context);
    mCheckedPosition = pos;
  }

  @Override
  public int getViewTypeCount()
  {
    return super.getViewTypeCount() + 1;
  }

  @Override
  public int getCount()
  {
    return super.getCount() + 1;
  }

  @Override
  public int getItemViewType(int position)
  {
    return position == getCount() - 1 ? VIEW_TYPE_ADD_NEW : VIEW_TYPE_CATEGORY;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    final int viewType = getItemViewType(position);
    if (convertView == null)
    {
      if (viewType == VIEW_TYPE_CATEGORY)
      {
        convertView = LayoutInflater.from(getContext()).inflate(R.layout.item_bookmark_set_chooser, parent, false);
        convertView.setTag(new SingleChoiceHolder(convertView));
      }
      else
        convertView = LayoutInflater.from(getContext()).inflate(R.layout.item_bookmark_set_create, parent, false);
    }

    if (viewType == VIEW_TYPE_CATEGORY)
    {
      final SingleChoiceHolder holder = (SingleChoiceHolder) convertView.getTag();
      boolean checked = mCheckedPosition == position;
      holder.name.setText(getItem(position).getName());
      holder.checked.setChecked(checked);
    }

    return convertView;
  }

  private static class SingleChoiceHolder
  {
    TextView name;
    RadioButton checked;

    public SingleChoiceHolder(View convertView)
    {
      name = (TextView) convertView.findViewById(R.id.tv__set_name);
      checked = (RadioButton) convertView.findViewById(R.id.rb__selected);
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
