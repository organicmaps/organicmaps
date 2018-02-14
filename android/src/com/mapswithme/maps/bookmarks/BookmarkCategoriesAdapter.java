package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;
import com.mapswithme.maps.widget.recycler.RecyclerLongClickListener;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;

public class BookmarkCategoriesAdapter extends BaseBookmarkCategoryAdapter<BookmarkCategoriesAdapter.ViewHolder>
{
  private final static int TYPE_ITEM = 0;
  private final static int TYPE_HELP = 1;
  @Nullable
  private RecyclerLongClickListener mLongClickListener;
  @Nullable
  private RecyclerClickListener mClickListener;

  BookmarkCategoriesAdapter(@NonNull Context context)
  {
    super(context);
  }

  public void setOnClickListener(@Nullable RecyclerClickListener listener)
  {
    mClickListener = listener;
  }

  void setOnLongClickListener(@Nullable RecyclerLongClickListener listener)
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
    view.setOnClickListener(
        v ->
        {
          if (mClickListener != null)
            mClickListener.onItemClick(v, holder.getAdapterPosition());
        });
    view.setOnLongClickListener(
        v ->
        {
          if (mLongClickListener != null)
            mLongClickListener.onLongItemClick(v, holder
                .getAdapterPosition());
          return true;
        });

    return holder;
  }

  @Override
  public void onBindViewHolder(final ViewHolder holder, final int position)
  {
    if (getItemViewType(position) == TYPE_HELP)
      return;

    final BookmarkManager bmManager = BookmarkManager.INSTANCE;
    final long catId = getItem(position);
    holder.name.setText(bmManager.getCategoryName(catId));
    holder.size.setText(String.valueOf(bmManager.getCategorySize(catId)));
    holder.setVisibilityState(bmManager.isVisible(catId));
    holder.visibilityMarker.setOnClickListener(
        v ->
        {
          BookmarkManager.INSTANCE.toggleCategoryVisibility(catId);
          holder.setVisibilityState(bmManager.isVisible(catId));
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
    int count = super.getItemCount();
    return count > 0 ? count + 1 : 0;
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

      name = root.findViewById(R.id.tv__set_name);
      visibilityMarker = root.findViewById(R.id.iv__set_visible);
      size = root.findViewById(R.id.tv__set_size);
    }

    void setVisibilityState(boolean visible)
    {
      Drawable drawable;
      if (visible)
      {
        visibilityMarker.setBackgroundResource(UiUtils.getStyledResourceId(
            visibilityMarker.getContext(), R.attr.activeIconBackground));
        drawable = Graphics.tint(visibilityMarker.getContext(), R.drawable.ic_bookmark_show, R.attr.activeIconTint);
      }
      else
      {
        visibilityMarker.setBackgroundResource(UiUtils.getStyledResourceId(
            visibilityMarker.getContext(), R.attr.steadyIconBackground));
        drawable = Graphics.tint(visibilityMarker.getContext(), R.drawable.ic_bookmark_hide,
                                 R.attr.steadyIconTint);
      }
      visibilityMarker.setImageDrawable(drawable);
    }
  }
}
