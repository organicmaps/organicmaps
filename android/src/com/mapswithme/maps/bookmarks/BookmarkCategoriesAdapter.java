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
import com.mapswithme.maps.bookmarks.BookmarkListAdapter.DataChangedListener;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

public class BookmarkCategoriesAdapter extends AbstractBookmarkCategoryAdapter
{
  private DataChangedListener mListener;
  public BookmarkCategoriesAdapter(Context context, DataChangedListener dcl)
  {
    super(context);
    mListener = dcl;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
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

    PinSetHolder psh = (PinSetHolder) convertView.getTag();
    BookmarkCategory set = getItem(position);
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

  @Override
  public void notifyDataSetChanged()
  {
    super.notifyDataSetChanged();
    if (mListener != null)
    {
      mListener.onDataChanged(isEmpty() ? View.VISIBLE : View.GONE);
    }
  }
}
