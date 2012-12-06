package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

public class BookmarkListAdapter extends BaseAdapter
{
  Context mContext;
  BookmarkCategory mCategory;
  public BookmarkListAdapter(Context context, BookmarkCategory cat)
  {
    mContext = context;
    mCategory = cat;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    if (convertView == null)
    {
      convertView = LayoutInflater.from(mContext).inflate(R.layout.pin_item, null);
      convertView.setTag(new PinHolder((ImageView) convertView.findViewById(R.id.pi_arrow), (ImageView) convertView
          .findViewById(R.id.pi_pin_color), (TextView) convertView.findViewById(R.id.pi_name), (TextView) convertView
          .findViewById(R.id.pi_distance)));
    }
    Bookmark item = mCategory.getBookmark(position);
    PinHolder holder = (PinHolder) convertView.getTag();
    holder.name.setText(item.getName());
    holder.icon.setImageBitmap(item.getIcon().getIcon());
    Log.d("lat lot", item.getLat() + " " + item.getLon());
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

  @Override
  public int getCount()
  {
    return mCategory.getSize();
  }

  @Override
  public Bookmark getItem(int position)
  {
    return mCategory.getBookmark(position);
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }
}
