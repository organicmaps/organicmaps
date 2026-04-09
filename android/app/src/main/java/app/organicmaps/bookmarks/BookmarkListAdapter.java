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
import app.organicmaps.sdk.bookmarks.data.BookmarkInfo;
import app.organicmaps.sdk.bookmarks.data.BookmarkListRow;
import app.organicmaps.sdk.bookmarks.data.BookmarkListSession;
import app.organicmaps.sdk.bookmarks.data.BookmarkListSnapshot;
import app.organicmaps.sdk.bookmarks.data.IconClickListener;
import app.organicmaps.sdk.bookmarks.data.Track;
import app.organicmaps.widget.recycler.RecyclerClickListener;
import app.organicmaps.widget.recycler.RecyclerLongClickListener;
import java.util.Objects;

public class BookmarkListAdapter extends RecyclerView.Adapter<Holders.BaseBookmarkHolder>
{
  static final int TYPE_TRACK = BookmarkListRow.Type.TRACK;
  static final int TYPE_BOOKMARK = BookmarkListRow.Type.BOOKMARK;
  static final int TYPE_SECTION = BookmarkListRow.Type.SECTION;
  static final int TYPE_DESC = BookmarkListRow.Type.DESCRIPTION;
  static final int MAX_VISIBLE_LINES = 2;

  @NonNull
  private BookmarkListSnapshot mSnapshot = BookmarkListSnapshot.EMPTY;
  @Nullable
  private BookmarkListSession mSession;
  private boolean mSearchResults;

  @Nullable
  private RecyclerClickListener mClickListener;
  @Nullable
  private RecyclerLongClickListener mLongClickListener;
  @Nullable
  private RecyclerClickListener mMoreClickListener;
  @Nullable
  private IconClickListener mIconClickListener;

  void setSession(@Nullable BookmarkListSession session)
  {
    mSession = session;
  }

  void setSnapshot(@NonNull BookmarkListSnapshot snapshot, boolean searchResults)
  {
    mSnapshot = snapshot;
    mSearchResults = searchResults;
  }

  boolean isSearchResults()
  {
    return mSearchResults;
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

  public void setIconClickListener(@Nullable IconClickListener listener)
  {
    mIconClickListener = listener;
  }

  @Override
  @NonNull
  public Holders.BaseBookmarkHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    Holders.BaseBookmarkHolder holder;
    switch (viewType)
    {
    case TYPE_TRACK:
      Holders.TrackViewHolder trackHolder =
          new Holders.TrackViewHolder(inflater.inflate(R.layout.item_track, parent, false));
      trackHolder.setOnClickListener(mClickListener);
      trackHolder.setOnLongClickListener(mLongClickListener);
      trackHolder.setTrackIconClickListener(mIconClickListener);
      trackHolder.setMoreButtonClickListener(mMoreClickListener);
      holder = trackHolder;
      break;
    case TYPE_BOOKMARK:
      Holders.BookmarkViewHolder bookmarkHolder =
          new Holders.BookmarkViewHolder(inflater.inflate(R.layout.item_bookmark, parent, false));
      bookmarkHolder.setOnClickListener(mClickListener);
      bookmarkHolder.setOnLongClickListener(mLongClickListener);
      bookmarkHolder.setBookmarkIconClickListener(mIconClickListener);
      holder = bookmarkHolder;
      break;
    case TYPE_SECTION:
      TextView sectionView = (TextView) inflater.inflate(R.layout.item_category_title, parent, false);
      holder = new Holders.SectionViewHolder(sectionView);
      break;
    case TYPE_DESC:
      View desc = inflater.inflate(R.layout.item_category_description, parent, false);
      TextView moreBtn = desc.findViewById(R.id.more_btn);
      TextView text = desc.findViewById(R.id.text);
      TextView title = desc.findViewById(R.id.title);
      setMoreButtonVisibility(text, moreBtn);
      holder = new Holders.DescriptionViewHolder(desc);
      text.setOnClickListener(v -> onMoreButtonClicked(text, moreBtn));
      moreBtn.setOnClickListener(v -> onMoreButtonClicked(text, moreBtn));
      title.setOnClickListener(v -> onMoreButtonClicked(text, moreBtn));
      break;
    default: throw new AssertionError("Unsupported view type: " + viewType);
    }

    return holder;
  }

  @Override
  public void onBindViewHolder(@NonNull Holders.BaseBookmarkHolder holder, int position)
  {
    BookmarkListRow row = Objects.requireNonNull(mSession).getRow(position);
    holder.bind(
        row, mSnapshot.getType(position) == TYPE_SECTION ? getSectionTitle(row, holder.itemView.getResources()) : null);
  }

  @Override
  public int getItemViewType(int position)
  {
    return mSnapshot.getType(position);
  }

  @Override
  public long getItemId(int position)
  {
    return mSnapshot.getStableId(position);
  }

  @Override
  public int getItemCount()
  {
    return mSnapshot.size();
  }

  @NonNull
  private String getSectionTitle(@NonNull BookmarkListRow row, @NonNull Resources rs)
  {
    switch (row.getSectionKind())
    {
    case BookmarkListRow.SectionKind.DESCRIPTION: return rs.getString(R.string.description);
    case BookmarkListRow.SectionKind.TRACKS: return rs.getString(R.string.tracks_title);
    case BookmarkListRow.SectionKind.BOOKMARKS: return rs.getString(R.string.bookmarks);
    case BookmarkListRow.SectionKind.CUSTOM: return Objects.requireNonNull(row.getTitle());
    default: throw new IllegalArgumentException("Unsupported section row: " + row.getSectionKind());
    }
  }

  public Object getItem(int position)
  {
    BookmarkListRow row = Objects.requireNonNull(mSession).getRow(position);
    if (row.getType() == TYPE_TRACK)
      return Objects.requireNonNull(row.getTrack());
    if (row.getType() == TYPE_BOOKMARK)
      return Objects.requireNonNull(row.getBookmark());
    throw new UnsupportedOperationException("Unsupported row type: " + row.getType());
  }

  int getPositionById(long id, int type)
  {
    // Use metadata arrays for O(N) scan without JNI calls.
    // Stable ID encoding: bookmark = bookmarkId, track = -trackId - 1.
    for (int position = 0; position < mSnapshot.size(); position++)
    {
      if (mSnapshot.getType(position) != type)
        continue;

      long stableId = mSnapshot.getStableId(position);
      if (type == TYPE_BOOKMARK && stableId == id)
        return position;
      if (type == TYPE_TRACK && stableId == -id - 1)
        return position;
    }
    return -1;
  }

  private void setMoreButtonVisibility(TextView text, TextView moreBtn)
  {
    text.post(() -> setShortModeDescription(text, moreBtn));
  }

  private void onMoreButtonClicked(TextView textView, TextView moreBtn)
  {
    if (isShortModeDescription(textView))
      setExpandedModeDescription(textView, moreBtn);
    else
      setShortModeDescription(textView, moreBtn);
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
