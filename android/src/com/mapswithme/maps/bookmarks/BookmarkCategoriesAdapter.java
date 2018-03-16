package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;
import com.mapswithme.maps.widget.recycler.RecyclerLongClickListener;

import static com.mapswithme.maps.bookmarks.Holders.CategoryViewHolder;

public class BookmarkCategoriesAdapter extends BaseBookmarkCategoryAdapter<RecyclerView.ViewHolder>
{
  private final static int TYPE_ITEM = 0;
  private final static int TYPE_ACTION_CREATE_GROUP = 1;
  private final static int TYPE_ACTION_HIDE_ALL = 2;
  @Nullable
  private RecyclerLongClickListener mLongClickListener;
  @Nullable
  private RecyclerClickListener mClickListener;
  @Nullable
  private OnAddCategoryListener mOnAddCategoryListener;

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

  void setOnAddCategoryListener(@Nullable OnAddCategoryListener listener)
  {
    mOnAddCategoryListener = listener;
  }

  @Override
  public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(getContext());
    if (viewType == TYPE_ACTION_HIDE_ALL)
    {
      View hideAllView = inflater.inflate(R.layout.item_bookmark_hide_all, parent, false);
      hideAllView.findViewById(R.id.hide_btn).setOnClickListener
          (v ->
           {
             Toast.makeText(getContext(), "Coming soon", Toast.LENGTH_SHORT).show();
           });
      return new Holders.GeneralViewHolder(hideAllView);
    }

    if (viewType == TYPE_ACTION_CREATE_GROUP)
    {
      View createListView = inflater.inflate(R.layout.item_bookmark_create_group, parent, false);
      createListView.setOnClickListener
          (v ->
           {
             if (mOnAddCategoryListener != null)
               mOnAddCategoryListener.onAddCategory();
           });
      return new Holders.GeneralViewHolder(createListView);
    }

    View view = LayoutInflater.from(getContext()).inflate(R.layout.item_bookmark_category,
                                                          parent, false);
    final CategoryViewHolder holder = new CategoryViewHolder(view);
    view.setOnClickListener(
        v ->
        {
          if (mClickListener != null)
            mClickListener.onItemClick(v, holder.getAdapterPosition() - 1);
        });
    view.setOnLongClickListener(
        v ->
        {
          if (mLongClickListener != null)
            mLongClickListener.onLongItemClick(v, holder
                .getAdapterPosition() - 1);
          return true;
        });

    return holder;
  }

  @Override
  public void onBindViewHolder(final RecyclerView.ViewHolder holder, final int position)
  {
    int type = getItemViewType(position);
    if (type == TYPE_ACTION_CREATE_GROUP || type == TYPE_ACTION_HIDE_ALL)
      return;

    CategoryViewHolder categoryHolder = (CategoryViewHolder) holder;
    final BookmarkManager bmManager = BookmarkManager.INSTANCE;
    final long catId = getCategoryIdByPosition(position - 1);
    categoryHolder.setName(bmManager.getCategoryName(catId));
    categoryHolder.setSize(bmManager.getCategorySize(catId));
    categoryHolder.setVisibilityState(bmManager.isVisible(catId));
    categoryHolder.setVisibilityListener(
        v ->
        {
          BookmarkManager.INSTANCE.toggleCategoryVisibility(catId);
          categoryHolder.setVisibilityState(bmManager.isVisible(catId));
        });
  }

  @Override
  public int getItemViewType(int position)
  {
    if (position == 0)
      return TYPE_ACTION_HIDE_ALL;
    return (position == getItemCount() - 1) ? TYPE_ACTION_CREATE_GROUP : TYPE_ITEM;
  }

  @Override
  public long getCategoryIdByPosition(int position)
  {
    return super.getCategoryIdByPosition(position);
  }

  @Override
  public int getItemCount()
  {
    int count = super.getItemCount();
    return count > 0 ? count + 2 /* header + add category btn */ : 0;
  }

  interface OnAddCategoryListener
  {
    void onAddCategory();
  }
}
