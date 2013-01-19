package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

public class BookmarkCategoriesAdapter extends AbstractBookmarkCategoryAdapter
{

  public BookmarkCategoriesAdapter(Context context)
  {
    super(context);
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    if (convertView == null)
    {
      convertView = LayoutInflater.from(getContext()).inflate(R.layout.bmk_category_item, null);
      convertView.setTag(new PinSetHolder((TextView) convertView.findViewById(R.id.psi_name)));
    }
    PinSetHolder psh = (PinSetHolder) convertView.getTag();
    BookmarkCategory set = getItem(position);
    psh.name.setText(set.getName() + " ("+String.valueOf(set.getSize())+")");
    return convertView;
  }

  static class PinSetHolder
  {
    TextView name;

    public PinSetHolder(TextView name)
    {
      super();
      this.name = name;
    }

  }
}
