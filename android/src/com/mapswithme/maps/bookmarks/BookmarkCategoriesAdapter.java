package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;
import com.mapswithme.maps.widget.recycler.RecyclerLongClickListener;
import com.mapswithme.util.ThemeUtils;

public class BookmarkCategoriesAdapter extends BaseBookmarkCategoryAdapter<BookmarkCategoriesAdapter.ViewHolder>
{
  private final static int TYPE_ITEM = 0;
  private final static int TYPE_HELP = 1;
  private RecyclerLongClickListener mLongClickListener;
  private RecyclerClickListener mClickListener;

  public BookmarkCategoriesAdapter(Context context)
  {
    super(context);
  }

  public void setOnClickListener(RecyclerClickListener listener)
  {
    mClickListener = listener;
  }

  public void setOnLongClickListener(RecyclerLongClickListener listener)
  {
    mLongClickListener = listener;
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    View view;
    if (viewType == TYPE_HELP)
    {
      TextView hintView = (TextView) LayoutInflater.from(getContext()).inflate(R.layout.item_bookmark_hint, parent, false);
      if (getItemCount() > 1)
        hintView.setText(R.string.bookmarks_usage_hint_import_only);

      view = hintView;
    }
    else
      view = LayoutInflater.from(getContext()).inflate(R.layout.item_bookmark_category, parent, false);

    final ViewHolder holder = new ViewHolder(view, viewType);
    view.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        mClickListener.onItemClick(v, holder.getAdapterPosition());
      }
    });
    view.setOnLongClickListener(new View.OnLongClickListener()
    {
      @Override
      public boolean onLongClick(View v)
      {
        mLongClickListener.onLongItemClick(v, holder.getAdapterPosition());
        return true;
      }
    });

    return holder;
  }

  @Override
  public void onBindViewHolder(final ViewHolder holder, final int position)
  {
    if (getItemViewType(position) == TYPE_HELP)
      return;

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
  }

  @Override
  public int getItemViewType(int position)
  {
    return (position == getItemCount() - 1) ? TYPE_HELP : TYPE_ITEM;
  }

  @Override
  public int getItemCount()
  {
    return super.getItemCount() + 1;
  }

  static class ViewHolder extends RecyclerView.ViewHolder
  {
    TextView name;
    ImageView visibilityMarker;
    TextView size;

    public ViewHolder(View root, int viewType)
    {
      super(root);

      if (viewType == TYPE_HELP)
      {
        root.setEnabled(false);
        return;
      }

      name = (TextView) root.findViewById(R.id.tv__set_name);
      visibilityMarker = (ImageView) root.findViewById(R.id.iv__set_visible);
      size = (TextView) root.findViewById(R.id.tv__set_size);
    }

    void setVisibilityState(boolean visible)
    {
      visibilityMarker.setImageResource(ThemeUtils.isNightTheme() ? visible ? R.drawable.ic_bookmark_show_night
                                                                            : R.drawable.ic_bookmark_hide_night
                                                                  : visible ? R.drawable.ic_bookmark_show
                                                                            : R.drawable.ic_bookmark_hide);
    }
  }
}
