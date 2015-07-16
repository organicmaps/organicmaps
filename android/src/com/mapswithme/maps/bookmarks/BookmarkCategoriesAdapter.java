package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

public class BookmarkCategoriesAdapter extends AbstractBookmarkCategoryAdapter
{
  public BookmarkCategoriesAdapter(Context context)
  {
    super(context);
  }

  private final static int TYPE_ITEM = 0;
  private final static int TYPE_HELP = 1;

  @Override
  public int getCount()
  {
    return super.getCount() + 1;
  }

  @Override
  public int getItemViewType(int position)
  {
    return (position == getCount() - 1) ? TYPE_HELP : TYPE_ITEM;
  }

  @Override
  public int getViewTypeCount()
  {
    return 2;
  }

  @Override
  public boolean isEnabled(int position)
  {
    return getItemViewType(position) != TYPE_HELP;
  }

  @Override
  public View getView(final int position, View convertView, ViewGroup parent)
  {
    if (getItemViewType(position) == TYPE_HELP)
    {
      final TextView hintView = (TextView) LayoutInflater.from(getContext()).inflate(R.layout.item_bookmark_hint, parent, false);
      if (super.getCount() > 0)
        hintView.setText(R.string.bookmarks_usage_hint_import_only);
      return hintView;
    }

    if (convertView == null)
    {
      convertView = LayoutInflater.from(getContext()).inflate(R.layout.item_bookmark_category, parent, false);
      final ViewHolder holder = new ViewHolder(convertView);
      convertView.setTag(holder);
    }

    final ViewHolder holder = (ViewHolder) convertView.getTag();
    final BookmarkCategory set = getItem(position);
    holder.name.setText(set.getName());
    holder.size.setText(String.valueOf(set.getSize()));
    holder.setVisibilityState(set.isVisible());
    holder.visibilityMarker.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        BookmarkManager.INSTANCE.toggleCategoryVisibility(position);
        holder.setVisibilityState(set.isVisible());
      }
    });


    return convertView;
  }

  static class ViewHolder
  {
    TextView name;
    ImageView visibilityMarker;
    TextView size;

    public ViewHolder(View root)
    {
      name = (TextView) root.findViewById(R.id.tv__set_name);
      visibilityMarker = (ImageView) root.findViewById(R.id.set_visible);
      size = (TextView) root.findViewById(R.id.tv__set_size);
    }

    void setVisibilityState(boolean visible)
    {
      visibilityMarker.setImageResource(visible ? R.drawable.ic_bookmark_show
                                                : R.drawable.ic_bookmark_hide);
    }
  }
}
