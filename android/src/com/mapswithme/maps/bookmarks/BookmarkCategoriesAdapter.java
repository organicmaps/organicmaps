package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.OnItemClickListener;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.UiUtils;

import static com.mapswithme.maps.bookmarks.Holders.CategoryViewHolder;
import static com.mapswithme.maps.bookmarks.Holders.HeaderViewHolder;
import static com.mapswithme.util.UiUtils.PHRASE_SEPARATOR;

public class BookmarkCategoriesAdapter extends BaseBookmarkCategoryAdapter<RecyclerView.ViewHolder>
{
  private final static int TYPE_CATEGORY_ITEM = 0;
  private final static int TYPE_ACTION_FOOTER = 1;
  private final static int TYPE_ACTION_HEADER = 2;
  private final static int HEADER_POSITION = 0;
  @NonNull
  private final BookmarkCategoriesPageResProvider mResProvider;
  @Nullable
  private OnItemLongClickListener<BookmarkCategory> mLongClickListener;
  @Nullable
  private OnItemClickListener<BookmarkCategory> mClickListener;
  @Nullable
  private CategoryListCallback mCategoryListCallback;
  @NonNull
  private final MassOperationAction mMassOperationAction = new MassOperationAction();
  @NonNull
  private final BookmarkCategory.Type mType;

  BookmarkCategoriesAdapter(@NonNull Context context, @NonNull BookmarkCategory.Type type)
  {
    super(context.getApplicationContext());
    mType = type;
    mResProvider = type.getFactory().getResProvider();
  }

  BookmarkCategoriesAdapter(@NonNull Context context)
  {
    this(context, BookmarkCategory.Type.PRIVATE);
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

    if (viewType == TYPE_ACTION_FOOTER)
    {
      View item = inflater.inflate(R.layout.item_bookmark_create_group, parent, false);
      item.setOnClickListener(new FooterClickListener());
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
    if (type == TYPE_ACTION_FOOTER)
    {
      bindFooterHolder(holder);
      return;
    }

    if (type == TYPE_ACTION_HEADER)
    {
      bindHeaderHolder(holder);
      return;
    }

    bindCategoryHolder(holder, position);
  }

  private void bindFooterHolder(@NonNull RecyclerView.ViewHolder holder)
  {
    Holders.GeneralViewHolder generalViewHolder = (Holders.GeneralViewHolder) holder;
    generalViewHolder.getImage().setImageResource(mResProvider.getFooterImage());
    generalViewHolder.getText().setText(mResProvider.getFooterText());
  }

  private void bindHeaderHolder(@NonNull RecyclerView.ViewHolder holder)
  {
    HeaderViewHolder headerViewHolder = (HeaderViewHolder) holder;
    headerViewHolder.setAction(mMassOperationAction,
                               mResProvider,
                               BookmarkManager.INSTANCE.areAllCategoriesInvisible(mType));
    headerViewHolder.getText().setText(mResProvider.getHeaderText());
  }

  private void bindCategoryHolder(@NonNull RecyclerView.ViewHolder holder, int position)
  {
    final BookmarkCategory category = getCategoryByPosition(toCategoryPosition(position));
    CategoryViewHolder categoryHolder = (CategoryViewHolder) holder;
    categoryHolder.setCategory(category);
    categoryHolder.setName(category.getName());
    bindSize(categoryHolder, category);
    bindAuthor(categoryHolder, category);
    bindAccessRules(category, categoryHolder);
    categoryHolder.setVisibilityState(category.isVisible());
    ToggleVisibilityClickListener listener = new ToggleVisibilityClickListener(categoryHolder);
    categoryHolder.setVisibilityListener(listener);
    categoryHolder.setMoreListener(v -> onMoreOperationClicked(category));
  }

  private void bindAccessRules(@NonNull BookmarkCategory category,
                               @NonNull CategoryViewHolder categoryHolder)
  {
    BookmarkCategory.AccessRules rules = category.getAccessRules();
    categoryHolder.mAccessRuleImage.setImageResource(rules.getDrawableResId());
    String representation = categoryHolder.itemView.getResources().getString(rules.getNameResId())
                            + UiUtils.PHRASE_SEPARATOR;
    categoryHolder.mAccessRule.setText(representation);
    UiUtils.hideIf(rules == BookmarkCategory.AccessRules.ACCESS_RULES_P2P
                   || rules == BookmarkCategory.AccessRules.ACCESS_RULES_PAID);
  }

  private void onMoreOperationClicked(@NonNull BookmarkCategory category)
  {
    if (mCategoryListCallback != null)
      mCategoryListCallback.onMoreOperationClick(category);
  }

  private void bindSize(@NonNull CategoryViewHolder categoryHolder,
                        @NonNull BookmarkCategory category)
  {
    BookmarkCategory.CountAndPlurals template = category.getPluralsCountTemplate();
    categoryHolder.setSize(template.getPlurals(), template.getCount());
  }

  private void bindAuthor(@NonNull CategoryViewHolder categoryHolder,
                          @NonNull BookmarkCategory category)
  {
    BookmarkCategory.Author author = category.getAuthor();
    CharSequence authorName = author == null
                              ? null
                              : getAuthorRepresentation(author, getContext());
    categoryHolder.getAuthorName().setText(authorName);
  }

  @NonNull
  private static String getAuthorRepresentation(@NonNull BookmarkCategory.Author author,
                                                @NonNull Context context)
  {
    return PHRASE_SEPARATOR + BookmarkCategory.Author.getRepresentation(context, author);
  }

  @Override
  public int getItemViewType(int position)
  {
    if (position == 0)
      return TYPE_ACTION_HEADER;

    return (position == getItemCount() - 1) && mType.getFactory().hasAdapterFooter() ? TYPE_ACTION_FOOTER
                                                                                     : TYPE_CATEGORY_ITEM;
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
    return count > 0 ? count + 1 /* header */ + (mType.getFactory().hasAdapterFooter() ? 1 : 0) : 0;
  }

  @NonNull
  public BookmarksPageFactory getFactory()
  {
    return mType.getFactory();
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
      BookmarkManager.INSTANCE.setAllCategoriesVisibility(false, mType);
      notifyDataSetChanged();
    }

    @Override
    public void onShowAll()
    {
      BookmarkManager.INSTANCE.setAllCategoriesVisibility(true, mType);
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

  private class FooterClickListener implements View.OnClickListener
  {
    @Override
    public void onClick(View v)
    {
      if (mCategoryListCallback != null)
        mCategoryListCallback.onFooterClick();
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
      BookmarkManager.INSTANCE.toggleCategoryVisibility(mHolder.getEntity().getId());
      notifyItemChanged(mHolder.getAdapterPosition());
      notifyItemChanged(HEADER_POSITION);
    }
  }
}
