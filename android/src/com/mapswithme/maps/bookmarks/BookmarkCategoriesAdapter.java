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
  private final static int TYPE_CATEGORY_ITEM = 0;
  private final static int TYPE_ACTION_CREATE_GROUP = 1;
  private final static int TYPE_ACTION_HIDE_ALL = 2;
  @Nullable
  private RecyclerLongClickListener mLongClickListener;
  @Nullable
  private RecyclerClickListener mClickListener;
  @Nullable
  private CategoryListInterface mCategoryListInterface;

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

  void setCategoryListInterface(@Nullable CategoryListInterface listener)
  {
    mCategoryListInterface = listener;
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
             if (mCategoryListInterface != null)
               mCategoryListInterface.onAddCategory();
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
            mClickListener.onItemClick(v, toCategoryPosition(holder.getAdapterPosition()));
        });
    view.setOnLongClickListener(
        v ->
        {
          if (mLongClickListener != null)
            mLongClickListener.onLongItemClick(v, toCategoryPosition(holder.getAdapterPosition()));
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
    final long catId = getCategoryIdByPosition(toCategoryPosition(position));
    categoryHolder.setName(bmManager.getCategoryName(catId));
    categoryHolder.setSize(bmManager.getCategorySize(catId));
    categoryHolder.setVisibilityState(bmManager.isVisible(catId));
    categoryHolder.setVisibilityListener(
        v ->
        {
          BookmarkManager.INSTANCE.toggleCategoryVisibility(catId);
          categoryHolder.setVisibilityState(bmManager.isVisible(catId));
        });
    categoryHolder.setMoreListener(v -> {
      if (mCategoryListInterface != null)
        mCategoryListInterface.onMoreOperationClick(toCategoryPosition(position));
    });
  }

  @Override
  public int getItemViewType(int position)
  {
    if (position == 0)
      return TYPE_ACTION_HIDE_ALL;
    return (position == getItemCount() - 1) ? TYPE_ACTION_CREATE_GROUP : TYPE_CATEGORY_ITEM;
  }

  private int toCategoryPosition(int adapterPosition)
  {

    int type = getItemViewType(adapterPosition);
    if (type != TYPE_CATEGORY_ITEM)
      throw new AssertionError("An element at specified position is not category!");

    // The header "Hide All" is located at first index, so subtraction is needed.
    return adapterPosition - 1;
  }

  @Override
  public int getItemCount()
  {
    int count = super.getItemCount();
    return count > 0 ? count + 2 /* header + add category btn */ : 0;
  }

  interface CategoryListInterface
  {
    void onAddCategory();
    void onMoreOperationClick(int position);
  }
}
