package com.mapswithme.maps.bookmarks;

import android.content.Context;
import android.content.res.Resources;
import android.view.LayoutInflater;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;
import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.OnItemClickListener;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

import java.util.List;

public class BookmarkCollectionAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder>
{
  private final static int TYPE_HEADER_ITEM = 0;
  private final static int TYPE_COLLECTION_ITEM = 1;
  private final static int TYPE_CATEGORY_ITEM = 2;

  @NonNull
  private final Context mContext;
  @NonNull
  private List<BookmarkCategory> mItemsCollection;
  @NonNull
  private List<BookmarkCategory> mItemsCategory;
  @NonNull
  private int mSectionCount;
  @NonNull
  private int mCollectionSectionIndex;
  @NonNull
  private int mCategorySectionIndex;
  @Nullable
  private OnItemClickListener<BookmarkCategory> mClickListener;

  static class SectionPosition
  {
    static final int INVALID_POSITION = -1;

    private final int mSectionIndex;
    private final int mItemIndex;

    SectionPosition(int sectionInd, int itemInd)
    {
      mSectionIndex = sectionInd;
      mItemIndex = itemInd;
    }

    int getSectionIndex()
    {
      return mSectionIndex;
    }

    int getItemIndex()
    {
      return mItemIndex;
    }

    boolean isTitlePosition()
    {
      return mSectionIndex != INVALID_POSITION && mItemIndex == INVALID_POSITION;
    }

    boolean isItemPosition()
    {
      return mSectionIndex != INVALID_POSITION && mItemIndex != INVALID_POSITION;
    }
  }

  private void calculateSections()
  {
    mCollectionSectionIndex = SectionPosition.INVALID_POSITION;
    mCategorySectionIndex = SectionPosition.INVALID_POSITION;

    mSectionCount = 0;
    if (mItemsCollection.size() > 0)
      mCollectionSectionIndex = mSectionCount++;
    if (mItemsCategory.size() > 0)
      mCategorySectionIndex = mSectionCount++;
  }

  public String getTitle(int sectionIndex, @NonNull Resources rs)
  {
    if (sectionIndex == mCollectionSectionIndex)
      // TODO (@velichkomarija): Replace categories
      return rs.getString(R.string.categories);
    return rs.getString(R.string.categories);
  }

  public int getItemsCount(int sectionIndex)
  {
    if (sectionIndex == mCollectionSectionIndex)
      return mItemsCollection.size();
    if (sectionIndex == mCategorySectionIndex)
      return mItemsCategory.size();
    return 0;
  }

  public int getItemsType(int sectionIndex)
  {
    if (sectionIndex == mCollectionSectionIndex)
      return TYPE_COLLECTION_ITEM;
    if (sectionIndex == mCategorySectionIndex)
      return TYPE_CATEGORY_ITEM;
    throw new AssertionError("Invalid section index: " + sectionIndex);
  }

  @NonNull
  private List<BookmarkCategory> getItemsListByType(int type)
  {
    if (type == TYPE_COLLECTION_ITEM)
    {
      return mItemsCollection;
    }
    else
    {
      return mItemsCategory;
    }
  }

  @NonNull
  public BookmarkCategory getCategoryByPosition(int position, int type)
  {
    List<BookmarkCategory> categories = getItemsListByType(type);

    if (position < 0 || position > categories.size() - /* header */ 1)
      throw new ArrayIndexOutOfBoundsException(position);
    return categories.get(position);
  }

  BookmarkCollectionAdapter(@NonNull Context context,
                            @NonNull List<BookmarkCategory> itemsCategories,
                            @NonNull List<BookmarkCategory> itemsCollection)
  {
    mContext = context;
    mItemsCategory = itemsCategories;
    mItemsCollection = itemsCollection;
    calculateSections();
  }

  public void setOnClickListener(@Nullable OnItemClickListener<BookmarkCategory> listener)
  {
    mClickListener = listener;
  }

  private SectionPosition getSectionPosition(int position)
  {
    int startSectionRow = 0;
    for (int i = 0; i < mSectionCount; ++i)
    {
      int sectionRowsCount = getItemsCount(i) + /* header */ 1;
      if (startSectionRow == position)
        return new SectionPosition(i, SectionPosition.INVALID_POSITION);
      if (startSectionRow + sectionRowsCount > position)
        return new SectionPosition(i, position - startSectionRow - /* header */ 1);
      startSectionRow += sectionRowsCount;
    }
    return new SectionPosition(SectionPosition.INVALID_POSITION, SectionPosition.INVALID_POSITION);
  }

  @NonNull
  @Override
  public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    RecyclerView.ViewHolder holder = null;

    if (viewType == TYPE_HEADER_ITEM)
    {
      holder = new Holders.HeaderViewHolder(inflater.inflate(R.layout.item_bookmark_group_list_header,
                                                             parent, false));
      // TODO (@velichkomarija) : Add click listener for this.
    }
    if (viewType == TYPE_CATEGORY_ITEM || viewType == TYPE_COLLECTION_ITEM)
    {
      holder = new Holders.CollectionViewHolder(inflater.inflate(R.layout.item_bookmark_collection,
                                                                 parent, false));
      // TODO (@velichkomarija): add click listeners.
    }

    if (holder == null)
      throw new AssertionError("Unsupported view type: " + viewType);

    return holder;
  }

  @Override
  public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position)
  {
    int type = getItemViewType(position);

    if (type == TYPE_HEADER_ITEM)
      bindHeaderHolder(holder);
    else
      bindCollectionHolder(holder, position, type);
  }

  private void bindCollectionHolder(RecyclerView.ViewHolder holder, int position, int type)
  {
    final  BookmarkCategory category = getCategoryByPosition(position, type);
    Holders.CollectionViewHolder collectionViewHolder = (Holders.CollectionViewHolder) holder;
    collectionViewHolder.setCategory(category);
    collectionViewHolder.setName(category.getName());
    bindSize(collectionViewHolder, category);
    collectionViewHolder.setVisibilityState(category.isVisible());

    // TODO (@velichkomarija): ToggleVisibilityClickListener for visibility.
  }

  private void bindSize(Holders.CollectionViewHolder holder, BookmarkCategory category)
  {
    BookmarkCategory.CountAndPlurals template = category.getPluralsCountTemplate();
    holder.setSize(template.getPlurals(), template.getCount());
  }

  private void bindHeaderHolder(@NonNull RecyclerView.ViewHolder holder)
  {
    Holders.HeaderViewHolder headerViewHolder = (Holders.HeaderViewHolder) holder;
    // TODO: (@velichkomarija) : Hide and All button.
  }

  @Override
  public int getItemViewType(int position)
  {
    SectionPosition sectionPosition = getSectionPosition(position);
    if (sectionPosition.isTitlePosition())
      return TYPE_HEADER_ITEM;
    if (sectionPosition.isItemPosition())
      return getItemsType(sectionPosition.getSectionIndex());
    throw new AssertionError("Position not found: " + position);
  }

  @Override
  public int getItemCount()
  {
    return mItemsCategory.size() + mItemsCollection.size();
  }
}
