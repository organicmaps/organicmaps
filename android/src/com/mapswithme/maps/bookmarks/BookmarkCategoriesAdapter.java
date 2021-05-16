package com.mapswithme.maps.bookmarks;

import static com.mapswithme.maps.bookmarks.Holders.CategoryViewHolder;
import static com.mapswithme.maps.bookmarks.Holders.HeaderViewHolder;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.OnItemClickListener;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

import java.util.List;

public class BookmarkCategoriesAdapter extends BaseBookmarkCategoryAdapter<RecyclerView.ViewHolder>
{
  private final static int HEADER_POSITION = 0;
  private final static int TYPE_ACTION_HEADER = 0;
  private final static int TYPE_CATEGORY_ITEM = 1;
  private final static int TYPE_ACTION_ADD = 2;
  @Nullable
  private OnItemLongClickListener<BookmarkCategory> mLongClickListener;
  @Nullable
  private OnItemClickListener<BookmarkCategory> mClickListener;
  @Nullable
  private CategoryListCallback mCategoryListCallback;
  @NonNull
  private final MassOperationAction mMassOperationAction = new MassOperationAction();

  BookmarkCategoriesAdapter(@NonNull Context context, @NonNull List<BookmarkCategory> categories)
  {
    super(context.getApplicationContext(), categories);
  }

  public void setOnClickListener(@Nullable OnItemClickListener<BookmarkCategory> listener)
  {
    mClickListener = listener;
  }

  void setOnLongClickListener(@Nullable OnItemLongClickListener<BookmarkCategory> listener)
  {
    mLongClickListener = listener;
  }

  void setCategoryListCallback(@Nullable CategoryListCallback listener)
  {
    mCategoryListCallback = listener;
  }

  @Override
  public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    if (viewType == TYPE_ACTION_HEADER)
    {
      View header = inflater.inflate(R.layout.item_bookmark_group_list_header, parent, false);
      return new Holders.HeaderViewHolder(header);
    }

    if (viewType == TYPE_ACTION_ADD)
    {
      View item = inflater.inflate(R.layout.item_bookmark_create_group, parent, false);
      item.setOnClickListener(new AddButtonClickListener());
      return new Holders.GeneralViewHolder(item);
    }

    View view = inflater.inflate(R.layout.item_bookmark_category, parent,false);
    final CategoryViewHolder holder = new CategoryViewHolder(view);
    view.setOnClickListener(new CategoryItemClickListener(holder));
    view.setOnLongClickListener(new LongClickListener(holder));

    return holder;
  }

  @Override
  public void onBindViewHolder(final RecyclerView.ViewHolder holder, final int position)
  {
    int type = getItemViewType(position);
    if (type == TYPE_ACTION_ADD)
    {
      bindAddButtonHolder(holder);
      return;
    }

    if (type == TYPE_ACTION_HEADER)
    {
      bindHeaderHolder(holder);
      return;
    }

    bindCategoryHolder(holder, position);
  }

  private void bindAddButtonHolder(@NonNull RecyclerView.ViewHolder holder)
  {
    Holders.GeneralViewHolder generalViewHolder = (Holders.GeneralViewHolder) holder;
    generalViewHolder.getImage().setImageResource(R.drawable.ic_checkbox_add);
    generalViewHolder.getText().setText(R.string.bookmarks_create_new_group);
  }

  private void bindHeaderHolder(@NonNull RecyclerView.ViewHolder holder)
  {
    HeaderViewHolder headerViewHolder = (HeaderViewHolder) holder;
    headerViewHolder.setAction(mMassOperationAction,
                               BookmarkManager.INSTANCE.areAllCategoriesInvisible());
    headerViewHolder.getText().setText(R.string.bookmarks_groups);
  }

  private void bindCategoryHolder(@NonNull RecyclerView.ViewHolder holder, int position)
  {
    final BookmarkCategory category = getCategoryByPosition(toCategoryPosition(position));
    CategoryViewHolder categoryHolder = (CategoryViewHolder) holder;
    categoryHolder.setCategory(category);
    categoryHolder.setName(category.getName());
    bindSize(categoryHolder, category);
    categoryHolder.setVisibilityState(category.isVisible());
    ToggleVisibilityClickListener listener = new ToggleVisibilityClickListener(categoryHolder);
    categoryHolder.setVisibilityListener(listener);
  }

  private void bindSize(@NonNull CategoryViewHolder categoryHolder,
                        @NonNull BookmarkCategory category)
  {
    BookmarkCategory.CountAndPlurals template = category.getPluralsCountTemplate();
    categoryHolder.setSize(template.getPlurals(), template.getCount());
  }

  @Override
  public int getItemViewType(int position)
  {
    if (position == 0)
      return TYPE_ACTION_HEADER;

    if (position == getItemCount() - 1)
      return TYPE_ACTION_ADD;

    return TYPE_CATEGORY_ITEM;
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
    if (count == 0)
      return 0;
    return 1 /* header */ + count + 1 /* add button */;
  }

  private class LongClickListener implements View.OnLongClickListener
  {
    @NonNull
    private final CategoryViewHolder mHolder;

    LongClickListener(@NonNull CategoryViewHolder holder)
    {
      mHolder = holder;
    }

    @Override
    public boolean onLongClick(View view)
    {
      if (mLongClickListener != null)
      {
        mLongClickListener.onItemLongClick(view, mHolder.getEntity());
      }
      return true;
    }
  }

  private class MassOperationAction implements HeaderViewHolder.HeaderAction
  {
    @Override
    public void onHideAll()
    {
      BookmarkManager.INSTANCE.setAllCategoriesVisibility(false);
      notifyDataSetChanged();
    }

    @Override
    public void onShowAll()
    {
      BookmarkManager.INSTANCE.setAllCategoriesVisibility(true);
      notifyDataSetChanged();
    }
  }

  private class CategoryItemClickListener implements View.OnClickListener
  {
    @NonNull
    private final CategoryViewHolder mHolder;

    CategoryItemClickListener(@NonNull CategoryViewHolder holder)
    {
      mHolder = holder;
    }

    @Override
    public void onClick(View v)
    {
      if (mClickListener != null)
        mClickListener.onItemClick(v, mHolder.getEntity());
    }
  }

  private class AddButtonClickListener implements View.OnClickListener
  {
    @Override
    public void onClick(View v)
    {
      if (mCategoryListCallback != null)
        mCategoryListCallback.onAddButtonClick();
    }
  }

  private class ToggleVisibilityClickListener implements View.OnClickListener
  {
    @NonNull
    private final CategoryViewHolder mHolder;

    ToggleVisibilityClickListener(@NonNull CategoryViewHolder holder)
    {
      mHolder = holder;
    }

    @Override
    public void onClick(View v)
    {
      BookmarkCategory category = mHolder.getEntity();
      BookmarkManager.INSTANCE.toggleCategoryVisibility(category);
      notifyItemChanged(mHolder.getAdapterPosition());
      notifyItemChanged(HEADER_POSITION);
    }
  }
}
