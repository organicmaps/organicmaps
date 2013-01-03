package com.mapswithme.maps.bookmarks;

import java.util.List;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

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
      convertView.setTag(new PinSetHolder((TextView) convertView.findViewById(R.id.psi_name), (TextView) convertView.findViewById(R.id.psi_set_size)));
    }
    PinSetHolder psh = (PinSetHolder) convertView.getTag();
    BookmarkCategory set = getItem(position);
    psh.name.setText(set.getName());
    psh.size.setText(String.valueOf(set.getSize()));
    return convertView;
  }

  static class PinSetHolder
  {
    TextView name;
    TextView size;

    public PinSetHolder(TextView name, TextView size)
    {
      super();
      this.name = name;
      this.size = size;
    }

  }
}
