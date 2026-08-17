package app.organicmaps.bookmarks;

import android.content.res.Resources;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkInfo;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.IconClickListener;
import app.organicmaps.sdk.bookmarks.data.SortedBlock;
import app.organicmaps.sdk.content.DataSource;
import app.organicmaps.widget.recycler.RecyclerClickListener;
import app.organicmaps.widget.recycler.RecyclerLongClickListener;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.Set;

public class BookmarkListAdapter extends RecyclerView.Adapter<Holders.BaseBookmarkHolder>
{
  // view types
  static final int TYPE_TRACK = 0;
  static final int TYPE_BOOKMARK = 1;
  static final int TYPE_SECTION = 2;
  static final int TYPE_DESC = 3;
  static final int MAX_VISIBLE_LINES = 2;

  @NonNull
  private final DataSource<BookmarkCategory> mDataSource;
  @Nullable
  private List<Long> mSearchResults;
  @Nullable
  private List<SortedBlock> mSortedResults;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private SectionsDataSource mSectionsDataSource;

  @Nullable
  private RecyclerClickListener mClickListener;
  @Nullable
  private RecyclerLongClickListener mLongClickListener;
  @Nullable
  private SelectionStateProvider mSelectionStateProvider;
  private RecyclerClickListener mMoreClickListener;
  private RecyclerClickListener mEyeClickListener;
  private IconClickListener mIconClickListener;

  /**
   * The multi-selection state lives outside the adapter: it has to outlive a configuration change, and the
   * adapter itself, which is rebuilt when a load finishes while the screen is still starting up.
   */
  interface SelectionStateProvider
  {
    boolean isSelectionMode();
    boolean isSelected(int itemType, long itemId);
  }

  public static abstract class SectionsDataSource
  {
    @NonNull
    private final DataSource<BookmarkCategory> mDataSource;

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
    @Nullable
    public abstract String getTitle(int sectionIndex, @NonNull Resources rs);
    public abstract int getItemsCount(int sectionIndex);
    public abstract int getItemsType(int sectionIndex);
    public abstract long getBookmarkId(@NonNull SectionPosition pos);
    public abstract long getTrackId(@NonNull SectionPosition pos);

    /**
     * Appends every id of a bookmarks or tracks section at once. Reading a section one position at a time would
     * be quadratic, see {@link BookmarkCategory#getBookmarkIds()}.
     */
    public abstract void collectIds(int sectionIndex, @NonNull Collection<Long> ids);
  }

  private static class CategorySectionsDataSource extends SectionsDataSource
  {
    private int mSectionsCount;
    private int mBookmarksSectionIndex;
    private int mTracksSectionIndex;
    private int mDescriptionSectionIndex;
    @NonNull
    private long[] mBookmarkIds;
    @NonNull
    private long[] mTrackIds;

    CategorySectionsDataSource(@NonNull DataSource<BookmarkCategory> dataSource)
    {
      super(dataSource);
      calculateSections();
    }

    private void calculateSections()
    {
      // Read once for the lifetime of this data source, which ends when refreshSections() builds the next one -
      // every path that can change the category calls it. Asking the core for one id at a time is quadratic,
      // see BookmarkCategory#getBookmarkIds(), and the section layout below counts these same arrays, so the
      // rows drawn and the sections holding them cannot disagree.
      mTrackIds = getCategory().getTrackIds();
      mBookmarkIds = getCategory().getBookmarkIds();

      mBookmarksSectionIndex = SectionPosition.INVALID_POSITION;
      mTracksSectionIndex = SectionPosition.INVALID_POSITION;

      mSectionsCount = 0;
      // We must always show the description, even if it's blank.
      mDescriptionSectionIndex = mSectionsCount++;
      if (mTrackIds.length > 0)
        mTracksSectionIndex = mSectionsCount++;
      if (mBookmarkIds.length > 0)
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
      return sectionIndex != mDescriptionSectionIndex;
    }

    @Override
    public boolean hasTitle(int sectionIndex)
    {
      return true;
    }

    @Nullable
    public String getTitle(int sectionIndex, @NonNull Resources rs)
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
        return mTrackIds.length;
      if (sectionIndex == mBookmarksSectionIndex)
        return mBookmarkIds.length;
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
      throw new AssertionError("Invalid section index: " + sectionIndex);
    }

    @Override
    public long getBookmarkId(@NonNull SectionPosition pos)
    {
      return mBookmarkIds[pos.getItemIndex()];
    }

    @Override
    public long getTrackId(@NonNull SectionPosition pos)
    {
      return mTrackIds[pos.getItemIndex()];
    }

    @Override
    public void collectIds(int sectionIndex, @NonNull Collection<Long> ids)
    {
      if (sectionIndex == mTracksSectionIndex)
        addAll(ids, mTrackIds);
      else if (sectionIndex == mBookmarksSectionIndex)
        addAll(ids, mBookmarkIds);
    }
  }

  private static class SearchResultsSectionsDataSource extends SectionsDataSource
  {
    @NonNull
    private final List<Long> mSearchResults;

    SearchResultsSectionsDataSource(@NonNull DataSource<BookmarkCategory> dataSource, @NonNull List<Long> searchResults)
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
    public String getTitle(int sectionIndex, @NonNull Resources rs)
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
    public long getBookmarkId(@NonNull SectionPosition pos)
    {
      return mSearchResults.get(pos.getItemIndex());
    }

    @Override
    public long getTrackId(@NonNull SectionPosition pos)
    {
      throw new AssertionError("Tracks unsupported in search results.");
    }

    @Override
    public void collectIds(int sectionIndex, @NonNull Collection<Long> ids)
    {
      throw new AssertionError("Selection unsupported in search results.");
    }
  }

  private static class SortedSectionsDataSource extends SectionsDataSource
  {
    @NonNull
    private final List<SortedBlock> mSortedBlocks;

    SortedSectionsDataSource(@NonNull DataSource<BookmarkCategory> dataSource, @NonNull List<SortedBlock> sortedBlocks)
    {
      super(dataSource);
      mSortedBlocks = sortedBlocks;
    }

    private boolean isDescriptionSection(int sectionIndex)
    {
      return sectionIndex == 0;
    }

    @NonNull
    private SortedBlock getSortedBlock(int sectionIndex)
    {
      if (isDescriptionSection(sectionIndex))
        throw new IllegalArgumentException("Invalid section index for sorted block.");
      return mSortedBlocks.get(sectionIndex - 1);
    }

    @Override
    public int getSectionsCount()
    {
      // Sorting does not take the description away, blank or not: the unsorted list shows it either way.
      return mSortedBlocks.size() + 1;
    }

    @Override
    public boolean isEditable(int sectionIndex)
    {
      return !isDescriptionSection(sectionIndex);
    }

    @Override
    public boolean hasTitle(int sectionIndex)
    {
      return true;
    }

    @Nullable
    public String getTitle(int sectionIndex, @NonNull Resources rs)
    {
      if (isDescriptionSection(sectionIndex))
        return rs.getString(R.string.description);
      return getSortedBlock(sectionIndex).getName();
    }

    @Override
    public int getItemsCount(int sectionIndex)
    {
      if (isDescriptionSection(sectionIndex))
        return 1;
      SortedBlock block = getSortedBlock(sectionIndex);
      if (block.isBookmarksBlock())
        return block.getBookmarkIds().size();
      return block.getTrackIds().size();
    }

    @Override
    public int getItemsType(int sectionIndex)
    {
      if (isDescriptionSection(sectionIndex))
        return TYPE_DESC;
      if (getSortedBlock(sectionIndex).isBookmarksBlock())
        return TYPE_BOOKMARK;
      return TYPE_TRACK;
    }

    public long getBookmarkId(@NonNull SectionPosition pos)
    {
      return getSortedBlock(pos.getSectionIndex()).getBookmarkIds().get(pos.getItemIndex());
    }

    public long getTrackId(@NonNull SectionPosition pos)
    {
      return getSortedBlock(pos.getSectionIndex()).getTrackIds().get(pos.getItemIndex());
    }

    @Override
    public void collectIds(int sectionIndex, @NonNull Collection<Long> ids)
    {
      final SortedBlock block = getSortedBlock(sectionIndex);
      ids.addAll(block.isBookmarksBlock() ? block.getBookmarkIds() : block.getTrackIds());
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

  void refreshDataSource()
  {
    mDataSource.invalidate();
    refreshSections();
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

  void setSearchResults(@Nullable long[] searchResults)
  {
    if (searchResults != null)
    {
      mSearchResults = new ArrayList<>(searchResults.length);
      for (long id : searchResults)
        mSearchResults.add(id);
    }
    else
    {
      mSearchResults = null;
    }
    refreshSections();
  }

  void setSortedResults(@Nullable SortedBlock[] sortedResults)
  {
    if (sortedResults != null)
      mSortedResults = new ArrayList<>(Arrays.asList(sortedResults));
    else
      mSortedResults = null;
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

  public void setMoreListener(@Nullable RecyclerClickListener listener)
  {
    mMoreClickListener = listener;
  }

  public void setEyeListener(@Nullable RecyclerClickListener listener)
  {
    mEyeClickListener = listener;
  }

  public void setIconClickListener(IconClickListener listener)
  {
    mIconClickListener = listener;
  }

  void setSelectionStateProvider(@Nullable SelectionStateProvider provider)
  {
    mSelectionStateProvider = provider;
  }

  @Override
  @NonNull
  public Holders.BaseBookmarkHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    Holders.BaseBookmarkHolder holder = null;
    switch (viewType)
    {
    case TYPE_TRACK:
      Holders.TrackViewHolder trackHolder =
          new Holders.TrackViewHolder(inflater.inflate(R.layout.item_track, parent, false));
      trackHolder.setOnClickListener(mClickListener);
      trackHolder.setOnLongClickListener(mLongClickListener);
      trackHolder.setTrackIconClickListener(mIconClickListener);
      trackHolder.setMoreButtonClickListener(mMoreClickListener);
      trackHolder.setEyeClickListener(mEyeClickListener);
      holder = trackHolder;
      break;
    case TYPE_BOOKMARK:
      Holders.BookmarkViewHolder bookmarkHolder =
          new Holders.BookmarkViewHolder(inflater.inflate(R.layout.item_bookmark, parent, false));
      bookmarkHolder.setOnClickListener(mClickListener);
      bookmarkHolder.setOnLongClickListener(mLongClickListener);
      bookmarkHolder.setBookmarkIconClickListener(mIconClickListener);
      bookmarkHolder.setMoreButtonClickListener(mMoreClickListener);
      holder = bookmarkHolder;
      break;
    case TYPE_SECTION:
      TextView tv = (TextView) inflater.inflate(R.layout.item_bookmark_section_title, parent, false);
      holder = new Holders.SectionViewHolder(tv);
      break;
    case TYPE_DESC:
      View desc = inflater.inflate(R.layout.item_category_description, parent, false);
      TextView moreBtn = desc.findViewById(R.id.more_btn);
      TextView text = desc.findViewById(R.id.text);
      TextView title = desc.findViewById(R.id.title);
      setMoreButtonVisibility(text, moreBtn);
      holder = new Holders.DescriptionViewHolder(desc, mSectionsDataSource.getCategory());
      text.setOnClickListener(v -> onMoreButtonClicked(text, moreBtn));
      moreBtn.setOnClickListener(v -> onMoreButtonClicked(text, moreBtn));
      title.setOnClickListener(v -> onMoreButtonClicked(text, moreBtn));
      break;
    }

    if (holder == null)
      throw new AssertionError("Unsupported view type: " + viewType);

    return holder;
  }

  @Override
  public void onBindViewHolder(@NonNull Holders.BaseBookmarkHolder holder, int position)
  {
    SectionPosition sp = getSectionPosition(position);
    holder.bind(sp, mSectionsDataSource);

    final int sectionIndex = sp.getSectionIndex();
    final int itemsType = mSectionsDataSource.getItemsType(sectionIndex);
    final int itemsCount = mSectionsDataSource.getItemsCount(sectionIndex);
    holder.bindCardPosition(sp.getItemIndex() == 0, sp.getItemIndex() == itemsCount - 1);

    // Everything below is derived from sp: re-entering getItemIdAt(int)/getItemViewType(int) here would walk the
    // sections again, which getSectionPosition() has just done.
    final boolean selectionMode = mSelectionStateProvider != null && mSelectionStateProvider.isSelectionMode();
    final long itemId =
        selectionMode && sp.isItemPosition() && isSelectableType(itemsType) ? getItemIdAt(sp, itemsType) : -1;
    holder.bindSelection(selectionMode, itemId != -1 && mSelectionStateProvider.isSelected(itemsType, itemId));
  }

  @Override
  public int getItemViewType(int position)
  {
    SectionPosition sp = getSectionPosition(position);
    if (sp.isTitlePosition())
      return TYPE_SECTION;
    if (sp.isItemPosition())
      return mSectionsDataSource.getItemsType(sp.getSectionIndex());
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
      int sectionItemsCount = mSectionsDataSource.getItemsCount(i);
      if (sectionItemsCount == 0)
        continue;
      itemCount += sectionItemsCount;
      if (mSectionsDataSource.hasTitle(i))
        ++itemCount;
    }
    return itemCount;
  }

  void removeDeletedItem(long itemId, int type)
  {
    if (mSearchResults != null)
    {
      if (type == TYPE_BOOKMARK)
        mSearchResults.remove(Long.valueOf(itemId));
      // The cached sorted snapshot is hidden while search results are shown. Drop it because it
      // can contain the deleted bookmark or track and would be reused after search is closed.
      mSortedResults = null;
      return;
    }

    if (mSortedResults == null)
      return;

    for (int i = 0; i < mSortedResults.size(); ++i)
    {
      SortedBlock block = mSortedResults.get(i);
      final List<Long> ids = type == TYPE_BOOKMARK ? block.getBookmarkIds() : block.getTrackIds();
      if (!ids.remove(Long.valueOf(itemId)))
        continue;

      if (block.getBookmarkIds().isEmpty() && block.getTrackIds().isEmpty())
      {
        mSortedResults.remove(i);
        dropEmptySortedResults();
      }
      return;
    }
  }

  /**
   * An empty snapshot means "nothing sorted", not "sorted into no blocks": kept around it would hide every item
   * the category gains afterwards, until the screen is opened again.
   */
  private void dropEmptySortedResults()
  {
    if (mSortedResults != null && mSortedResults.isEmpty())
      mSortedResults = null;
  }

  /**
   * Same as {@link #removeDeletedItem(long, int)} for a whole selection, in a single pass: removing the ids one by
   * one rescans every sorted block per id.
   */
  void removeDeletedItems(@NonNull Set<Long> bookmarkIds, @NonNull Set<Long> trackIds)
  {
    // Search results are not handled: a batch comes from selection mode, which search excludes.
    if (mSortedResults == null)
      return;

    for (int i = mSortedResults.size() - 1; i >= 0; --i)
    {
      final SortedBlock block = mSortedResults.get(i);
      block.getBookmarkIds().removeAll(bookmarkIds);
      block.getTrackIds().removeAll(trackIds);
      if (block.getBookmarkIds().isEmpty() && block.getTrackIds().isEmpty())
        mSortedResults.remove(i);
    }
    dropEmptySortedResults();
  }

  boolean isSearchResults()
  {
    return mSearchResults != null;
  }

  boolean isSortedResults()
  {
    return mSortedResults != null;
  }

  /**
   * @return how many rows can be selected, counted per section instead of per row so that it stays cheap enough
   *     to call on every menu invalidation.
   */
  int getSelectableCount()
  {
    int count = 0;
    final int sectionsCount = mSectionsDataSource.getSectionsCount();
    for (int i = 0; i < sectionsCount; ++i)
      if (isSelectableType(mSectionsDataSource.getItemsType(i)))
        count += mSectionsDataSource.getItemsCount(i);
    return count;
  }

  void collectSelectableIds(@NonNull Collection<Long> bookmarkIds, @NonNull Collection<Long> trackIds)
  {
    final int sectionsCount = mSectionsDataSource.getSectionsCount();
    for (int i = 0; i < sectionsCount; ++i)
    {
      final int itemsType = mSectionsDataSource.getItemsType(i);
      if (itemsType == TYPE_BOOKMARK)
        mSectionsDataSource.collectIds(i, bookmarkIds);
      else if (itemsType == TYPE_TRACK)
        mSectionsDataSource.collectIds(i, trackIds);
    }
  }

  private static boolean isSelectableType(int itemsType)
  {
    return itemsType == TYPE_BOOKMARK || itemsType == TYPE_TRACK;
  }

  private static void addAll(@NonNull Collection<Long> ids, @NonNull long[] values)
  {
    for (long value : values)
      ids.add(value);
  }

  /**
   * @return the bookmark or track id of the row, or -1 for a section title, the category description and a
   *     position that is no longer in the list.
   */
  long getItemIdAt(int position)
  {
    final SectionPosition pos = getSectionPosition(position);
    if (!pos.isItemPosition())
      return -1;

    final int itemType = mSectionsDataSource.getItemsType(pos.getSectionIndex());
    return isSelectableType(itemType) ? getItemIdAt(pos, itemType) : -1;
  }

  private long getItemIdAt(@NonNull SectionPosition pos, int itemType)
  {
    return itemType == TYPE_BOOKMARK ? mSectionsDataSource.getBookmarkId(pos) : mSectionsDataSource.getTrackId(pos);
  }

  int getPositionById(long id, int type)
  {
    final int itemCount = getItemCount();
    for (int position = 0; position < itemCount; position++)
    {
      final int itemType = getItemViewType(position);
      if (itemType != type)
        continue;

      final SectionPosition pos = getSectionPosition(position);
      if (type == TYPE_BOOKMARK && mSectionsDataSource.getBookmarkId(pos) == id)
        return position;
      if (type == TYPE_TRACK && mSectionsDataSource.getTrackId(pos) == id)
        return position;
    }
    return -1;
  }

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
      BookmarkInfo info = BookmarkManager.INSTANCE.getBookmarkInfo(bookmarkId);
      if (info == null)
        throw new RuntimeException("Bookmark no longer exists " + bookmarkId);
      return info;
    }
  }

  private void setMoreButtonVisibility(TextView text, TextView moreBtn)
  {
    text.post(() -> setShortModeDescription(text, moreBtn));
  }

  private void onMoreButtonClicked(TextView textView, TextView moreBtn)
  {
    if (isShortModeDescription(textView))
    {
      setExpandedModeDescription(textView, moreBtn);
    }
    else
    {
      setShortModeDescription(textView, moreBtn);
    }
  }

  private boolean isShortModeDescription(TextView text)
  {
    return text.getMaxLines() == MAX_VISIBLE_LINES;
  }

  private void setExpandedModeDescription(TextView textView, TextView moreBtn)
  {
    textView.setMaxLines(Integer.MAX_VALUE);
    moreBtn.setVisibility(View.GONE);
  }

  private void setShortModeDescription(TextView textView, TextView moreBtn)
  {
    textView.setMaxLines(MAX_VISIBLE_LINES);

    boolean isDescriptionTooLong = textView.getLineCount() > MAX_VISIBLE_LINES;
    moreBtn.setVisibility(isDescriptionTooLong ? View.VISIBLE : View.GONE);
  }
}
