package com.mapswithme.maps.bookmarks;

import android.content.res.Resources;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.SortedBlock;
import com.mapswithme.maps.content.DataSource;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;
import com.mapswithme.maps.widget.recycler.RecyclerLongClickListener;

import java.util.List;

public class BookmarkListAdapter extends RecyclerView.Adapter<Holders.BaseBookmarkHolder>
{
  // view types
  static final int TYPE_TRACK = 0;
  static final int TYPE_BOOKMARK = 1;
  static final int TYPE_SECTION = 2;
  static final int TYPE_DESC = 3;

  @NonNull
  private final DataSource<BookmarkCategory> mDataSource;
  @Nullable
  private List<Long> mSearchResults;
  @Nullable
  private List<SortedBlock> mSortedResults;

  @NonNull
  private SectionsDataSource mSectionsDataSource;

  @Nullable
  private RecyclerLongClickListener mLongClickListener;
  @Nullable
  private RecyclerClickListener mClickListener;

  public static class SectionPosition
  {
    public static final int INVALID_POSITION = -1;

    public final int sectionIndex;
    public final int itemIndex;

    SectionPosition(int sectionInd, int itemInd)
    {
      sectionIndex = sectionInd;
      itemIndex = itemInd;
    }

    boolean isTitlePosition()
    {
      return sectionIndex != INVALID_POSITION && itemIndex == INVALID_POSITION;
    }

    boolean isItemPosition()
    {
      return sectionIndex != INVALID_POSITION && itemIndex != INVALID_POSITION;
    }
  }

  public abstract class SectionsDataSource
  {
    @NonNull
    protected final DataSource<BookmarkCategory> mDataSource;

    SectionsDataSource(@NonNull DataSource<BookmarkCategory> dataSource)
    {
      mDataSource = dataSource;
    }

    public BookmarkCategory getCategory()
    {
      return mDataSource.getData();
    }

    public abstract int getSectionsCount();
    public abstract boolean isEditable(int sectionIndex);
    public abstract boolean hasTitle(int sectionIndex);
    public abstract @Nullable String getTitle(int sectionIndex, Resources rs);
    public abstract int getItemsCount(int sectionIndex);
    public abstract int getItemsType(int sectionIndex);
    public abstract long getBookmarkId(SectionPosition pos);
    public abstract long getTrackId(SectionPosition pos);
    public abstract void onDelete(SectionPosition pos);
  }

  private class CategorySectionsDataSource extends SectionsDataSource
  {
    private int mSectionsCount;
    private int mDescriptionSectionIndex;
    private int mBookmarksSectionIndex;
    private int mTracksSectionIndex;

    CategorySectionsDataSource(@NonNull DataSource<BookmarkCategory> dataSource)
    {
      super(dataSource);
      calculateSections();
    }

    private void calculateSections()
    {
      mDescriptionSectionIndex = SectionPosition.INVALID_POSITION;
      mBookmarksSectionIndex = SectionPosition.INVALID_POSITION;
      mTracksSectionIndex = SectionPosition.INVALID_POSITION;

      mSectionsCount = 0;
      if (!getCategory().getAnnotation().isEmpty() || !getCategory().getDescription().isEmpty())
      {
        mDescriptionSectionIndex = mSectionsCount++;
      }
      if (getCategory().getTracksCount() > 0)
        mTracksSectionIndex = mSectionsCount++;
      if (getCategory().getBookmarksCount() > 0)
        mBookmarksSectionIndex = mSectionsCount++;
    }

    @Override
    public int getSectionsCount()
    {
      return mSectionsCount;
    }

    @Override
    public boolean isEditable(int sectionIndex)
    {
      return sectionIndex != mDescriptionSectionIndex && !getCategory().isFromCatalog();
    }

    @Override
    public boolean hasTitle(int sectionIndex)
    {
      return true;
    }

    @Nullable
    public String getTitle(int sectionIndex, Resources rs)
    {
      if (sectionIndex == mDescriptionSectionIndex)
        return rs.getString(R.string.description);
      if (sectionIndex == mTracksSectionIndex)
        return rs.getString(R.string.tracks_title);
      return rs.getString(R.string.bookmarks);
    }

    @Override
    public int getItemsCount(int sectionIndex)
    {
      if (sectionIndex == mDescriptionSectionIndex)
        return 1;
      if (sectionIndex == mTracksSectionIndex)
        return getCategory().getTracksCount();
      if (sectionIndex == mBookmarksSectionIndex)
        return getCategory().getBookmarksCount();
      return 0;
    }

    @Override
    public int getItemsType(int sectionIndex)
    {
      if (sectionIndex == mDescriptionSectionIndex)
        return TYPE_DESC;
      if (sectionIndex == mTracksSectionIndex)
        return TYPE_TRACK;
      if (sectionIndex == mBookmarksSectionIndex)
        return TYPE_BOOKMARK;
      return 0;
    }

    @Override
    public void onDelete(SectionPosition pos)
    {
      calculateSections();
    }

    @Override
    public long getBookmarkId(SectionPosition pos)
    {
      final long bookmarkId = BookmarkManager.INSTANCE.getBookmarkIdByPosition(
          getCategory().getId(), pos.itemIndex);
      return bookmarkId;
    }

    @Override
    public long getTrackId(SectionPosition pos)
    {
      final long trackId = BookmarkManager.INSTANCE.getTrackIdByPosition(
          getCategory().getId(), pos.itemIndex);
      return trackId;
    }
  }

  private class SearchResultsSectionsDataSource extends SectionsDataSource
  {
    @NonNull
    private final List<Long> mSearchResults;

    SearchResultsSectionsDataSource(@NonNull DataSource<BookmarkCategory> dataSource,
                                    @NonNull List<Long> searchResults)
    {
      super(dataSource);
      mSearchResults = searchResults;
    }

    @Override
    public int getSectionsCount()
    {
      return 1;
    }

    @Override
    public boolean isEditable(int sectionIndex)
    {
      return true;
    }

    @Override
    public boolean hasTitle(int sectionIndex)
    {
      return false;
    }

    @Nullable
    public String getTitle(int sectionIndex, Resources rs)
    {
      return null;
    }

    @Override
    public int getItemsCount(int sectionIndex)
    {
      return mSearchResults.size();
    }

    @Override
    public int getItemsType(int sectionIndex)
    {
      return TYPE_BOOKMARK;
    }

    @Override
    public void onDelete(SectionPosition pos)
    {
      mSearchResults.remove(pos.itemIndex);
    }

    @Override
    public long getBookmarkId(SectionPosition pos)
    {
      return mSearchResults.get(pos.itemIndex);
    }

    @Override
    public long getTrackId(SectionPosition pos)
    {
      return 0;
    }
  }

  private class SortedSectionsDataSource extends SectionsDataSource
  {
    @NonNull
    private List<SortedBlock> mSortedBlocks;

    SortedSectionsDataSource(@NonNull DataSource<BookmarkCategory> dataSource,
                             @NonNull List<SortedBlock> sortedBlocks)
    {
      super(dataSource);
      mSortedBlocks = sortedBlocks;
    }

    @Override
    public int getSectionsCount()
    {
      return mSortedBlocks.size();
    }

    @Override
    public boolean isEditable(int sectionIndex)
    {
      return true;
    }

    @Override
    public boolean hasTitle(int sectionIndex)
    {
      return true;
    }

    @Nullable
    public String getTitle(int sectionIndex, Resources rs)
    {
      return mSortedBlocks.get(sectionIndex).getName();
    }

    @Override
    public int getItemsCount(int sectionIndex)
    {
      SortedBlock block = mSortedBlocks.get(sectionIndex);
      if (block.isBookmarksBlock())
        return block.getBookmarkIds().size();
      return block.getTrackIds().size();
    }

    @Override
    public int getItemsType(int sectionIndex)
    {
      SortedBlock block = mSortedBlocks.get(sectionIndex);
      if (block.isBookmarksBlock())
        return TYPE_BOOKMARK;
      return TYPE_TRACK;
    }

    @Override
    public void onDelete(SectionPosition pos)
    {
      SortedBlock block = mSortedBlocks.get(pos.sectionIndex);
      if (block.isBookmarksBlock())
      {
        block.getBookmarkIds().remove(pos.itemIndex);
        if (block.getBookmarkIds().isEmpty())
          mSortedBlocks.remove(pos.sectionIndex);
        return;
      }

      block.getTrackIds().remove(pos.itemIndex);
      if (block.getTrackIds().isEmpty())
        mSortedBlocks.remove(pos.sectionIndex);
    }

    public long getBookmarkId(SectionPosition pos)
    {
      return mSortedBlocks.get(pos.sectionIndex).getBookmarkIds().get(pos.itemIndex);
    }

    public long getTrackId(SectionPosition pos)
    {
      return mSortedBlocks.get(pos.sectionIndex).getTrackIds().get(pos.itemIndex);
    }
  }

  BookmarkListAdapter(@NonNull DataSource<BookmarkCategory> dataSource)
  {
    mDataSource = dataSource;
    refreshSections();
  }

  private void refreshSections()
  {
    if (mSearchResults != null)
      mSectionsDataSource = new SearchResultsSectionsDataSource(mDataSource, mSearchResults);
    else if (mSortedResults != null)
      mSectionsDataSource = new SortedSectionsDataSource(mDataSource, mSortedResults);
    else
      mSectionsDataSource = new CategorySectionsDataSource(mDataSource);
  }

  private SectionPosition getSectionPosition(int position)
  {
    int startSectionRow = 0;
    boolean hasTitle;
    int sectionsCount = mSectionsDataSource.getSectionsCount();
    for (int i = 0; i < sectionsCount; ++i)
    {
      hasTitle = mSectionsDataSource.hasTitle(i);
      int sectionRowsCount = mSectionsDataSource.getItemsCount(i) + (hasTitle ? 1 : 0);
      if (startSectionRow == position && hasTitle)
        return new SectionPosition(i, SectionPosition.INVALID_POSITION);
      if (startSectionRow + sectionRowsCount > position)
        return new SectionPosition(i, position - startSectionRow - (hasTitle ? 1 : 0));
      startSectionRow += sectionRowsCount;
    }
    return new SectionPosition(SectionPosition.INVALID_POSITION, SectionPosition.INVALID_POSITION);
  }

  public void setSearchResults(List<Long> searchResults)
  {
    mSearchResults = searchResults;
    refreshSections();
  }

  public void setSortedResults(List<SortedBlock> sortedResults)
  {
    mSortedResults = sortedResults;
    refreshSections();
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
  public Holders.BaseBookmarkHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    Holders.BaseBookmarkHolder holder = null;
    switch (viewType)
    {
      case TYPE_TRACK:
        holder = new Holders.TrackViewHolder(inflater.inflate(R.layout.item_track, parent,
                                                              false));
        break;
      case TYPE_BOOKMARK:
        holder = new Holders.BookmarkViewHolder(inflater.inflate(R.layout.item_bookmark, parent,
                                                                 false));
        break;
      case TYPE_SECTION:
        TextView tv = (TextView) inflater.inflate(R.layout.item_category_title, parent, false);
        holder = new Holders.SectionViewHolder(tv);
        break;
      case TYPE_DESC:
        View desc = inflater.inflate(R.layout.item_category_description, parent, false);
        holder = new Holders.DescriptionViewHolder(desc, mSectionsDataSource.getCategory());
        break;
    }

    if (holder == null)
      throw new AssertionError("Unsupported view type: " + viewType);

    holder.setOnClickListener(mClickListener);
    holder.setOnLongClickListener(mLongClickListener);
    return holder;
  }

  @Override
  public void onBindViewHolder(Holders.BaseBookmarkHolder holder, int position)
  {
    SectionPosition sp = getSectionPosition(position);
    holder.bind(sp, mSectionsDataSource);
  }

  @Override
  public int getItemViewType(int position)
  {
    SectionPosition sp = getSectionPosition(position);
    if (sp.isTitlePosition())
      return TYPE_SECTION;
    if (sp.isItemPosition())
      return mSectionsDataSource.getItemsType(sp.sectionIndex);
    throw new IllegalArgumentException("Position not found: " + position);
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public int getItemCount()
  {
    int itemCount = 0;
    int sectionsCount = mSectionsDataSource.getSectionsCount();
    for (int i = 0; i < sectionsCount; ++i)
    {
      itemCount += mSectionsDataSource.getItemsCount(i);
      if (mSectionsDataSource.hasTitle(i))
        ++itemCount;
    }
    return itemCount;
  }

  // FIXME: remove this heavy method and use BoomarkInfo class instead.
  public Object getItem(int position)
  {
    if (getItemViewType(position) == TYPE_DESC)
      throw new UnsupportedOperationException("Not supported here! Position = " + position);

    SectionPosition pos = getSectionPosition(position);
    if (getItemViewType(position) == TYPE_TRACK)
    {
      final long trackId = mSectionsDataSource.getTrackId(pos);
      return BookmarkManager.INSTANCE.getTrack(trackId);
    }
    else
    {
      final long bookmarkId = mSectionsDataSource.getBookmarkId(pos);
      return BookmarkManager.INSTANCE.getBookmark(bookmarkId);
    }
  }
}
