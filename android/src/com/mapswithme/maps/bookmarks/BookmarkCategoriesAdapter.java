package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

public class BookmarkCategoriesAdapter extends AbstractBookmarkCategoryAdapter
{
  public BookmarkCategoriesAdapter(Context context)
  {
    super(context);
  }

  private final static int ITEM = 0;
  private final static int HELP = 1;

  @Override
  public int getCount()
  {
    return super.getCount() + 1;
  }

  @Override
  public int getItemViewType(int position)
  {
    if (position == getCount() - 1) return HELP;
    return ITEM;
  }

  @Override
  public int getViewTypeCount()
  {
    return 2;
  }

  public boolean isActiveItem(int position)
  {
    return getItemViewType(position) != HELP
        && position < getCount()
        && position >= 0;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    if (getItemViewType(position) == HELP)
    {
      final View hintView = LayoutInflater.from(getContext()).inflate(R.layout.bookmark_hint, null);
      if (super.getCount() > 0)
        ((TextView)hintView.findViewById(R.id.bookmarks_usage_hint)).setText(R.string.bookmarks_usage_hint_import_only);
      return hintView;
    }

    if (convertView == null)
    {
      convertView = LayoutInflater.from(getContext()).inflate(R.layout.bmk_category_item, null);
      final PinSetHolder holder = new PinSetHolder((TextView) convertView.findViewById(R.id.psi_name),
                                                   (CheckBox) convertView.findViewById(R.id.pin_set_visible));
      convertView.setTag(holder);
      holder.visibilityCheckBox.setOnCheckedChangeListener(new OnCheckedChangeListener()
      {
        @Override
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
        {
          BookmarkCategoriesAdapter.this
                                   .getBookmarkManager()
                                   .getCategoryById(holder.categoryId)
                                   .setVisibility(isChecked);
        }
      });
    }

    final PinSetHolder psh = (PinSetHolder) convertView.getTag();
    final BookmarkCategory set = getItem(position);
    // category ID
    psh.categoryId = position;
    // name
    psh.name.setText(set.getName() + " ("+String.valueOf(set.getSize())+")");
    // visiblity
    psh.visibilityCheckBox.setChecked(set.isVisible());

    return convertView;
  }

  static class PinSetHolder
  {
    TextView name;
    CheckBox visibilityCheckBox;

    // Data
    int categoryId;

    public PinSetHolder(TextView name, CheckBox visibilityCheckBox)
    {
      this.name = name;
      this.visibilityCheckBox = visibilityCheckBox;
    }
  }
}
