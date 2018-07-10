package com.mapswithme.maps.bookmarks;

import android.location.Location;
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
import com.mapswithme.maps.content.DataSource;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationListener;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;
import com.mapswithme.maps.widget.recycler.RecyclerLongClickListener;

import static com.mapswithme.maps.bookmarks.Holders.BaseBookmarkHolder.SECTION_BMKS;
import static com.mapswithme.maps.bookmarks.Holders.BaseBookmarkHolder.SECTION_DESC;
import static com.mapswithme.maps.bookmarks.Holders.BaseBookmarkHolder.SECTION_TRACKS;
import static com.mapswithme.maps.bookmarks.Holders.BaseBookmarkHolder.getBookmarksSectionPosition;
import static com.mapswithme.maps.bookmarks.Holders.BaseBookmarkHolder.getDescItemCount;
import static com.mapswithme.maps.bookmarks.Holders.BaseBookmarkHolder.getDescSectionPosition;
import static com.mapswithme.maps.bookmarks.Holders.BaseBookmarkHolder.getTracksSectionPosition;
import static com.mapswithme.maps.bookmarks.Holders.BaseBookmarkHolder.isSectionEmpty;
import static com.mapswithme.maps.bookmarks.Holders.BookmarkViewHolder.calculateBookmarkPosition;
import static com.mapswithme.maps.bookmarks.Holders.BaseBookmarkHolder.calculateTrackPosition;

public class BookmarkListAdapter extends RecyclerView.Adapter<Holders.BaseBookmarkHolder>
{
  @NonNull
  private final DataSource<BookmarkCategory> mDataSource;

  // view types
  static final int TYPE_TRACK = 0;
  static final int TYPE_BOOKMARK = 1;
  static final int TYPE_SECTION = 2;
  static final int TYPE_DESC = 3;

  private final LocationListener mLocationListener = new LocationListener.Simple()
  {
    @Override
    public void onLocationUpdated(Location location)
    {
      notifyDataSetChanged();
    }
  };

  @Nullable
  private RecyclerLongClickListener mLongClickListener;
  @Nullable
  private RecyclerClickListener mClickListener;

  BookmarkListAdapter(@NonNull DataSource<BookmarkCategory> dataSource)
  {
    mDataSource = dataSource;
  }

  public void setOnClickListener(@Nullable RecyclerClickListener listener)
  {
    mClickListener = listener;
  }

  void setOnLongClickListener(@Nullable RecyclerLongClickListener listener)
  {
    mLongClickListener = listener;
  }

  public void startLocationUpdate()
  {
    LocationHelper.INSTANCE.addListener(mLocationListener, true);
  }

  public void stopLocationUpdate()
  {
    LocationHelper.INSTANCE.removeListener(mLocationListener);
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
        holder = new Holders.DescriptionViewHolder(desc, getCategory());
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
    holder.bind(position, getCategory());
  }

  @Override
  public int getItemViewType(int position)
  {
    final int descPos = getDescSectionPosition(getCategory());
    final int bmkPos = getBookmarksSectionPosition(getCategory());
    final int trackPos = getTracksSectionPosition(getCategory());

    if (position == bmkPos || position == trackPos || position == descPos)
      return TYPE_SECTION;

    if (position > bmkPos && !isSectionEmpty(getCategory(), SECTION_BMKS))
      return TYPE_BOOKMARK;
    else if (position > trackPos && !isSectionEmpty(getCategory(), SECTION_TRACKS))
      return TYPE_TRACK;
    else if (position > descPos && !isSectionEmpty(getCategory(), SECTION_DESC))
      return TYPE_DESC;

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
    return getCategory().size()
           + (isSectionEmpty(getCategory(), SECTION_TRACKS) ? 0 : 1)
           + (isSectionEmpty(getCategory(), SECTION_BMKS) ? 0 : 1)
           + getDescItemCount(getCategory());
  }

  // FIXME: remove this heavy method and use BoomarkInfo class instead.
  public Object getItem(int position)
  {
    if (getItemViewType(position) == TYPE_DESC)
      throw new UnsupportedOperationException("Not supported here! Position = " + position);

    if (getItemViewType(position) == TYPE_TRACK)
    {
      int relativePos = calculateTrackPosition(getCategory(), position);
      final long trackId = BookmarkManager.INSTANCE.getTrackIdByPosition(getCategory().getId(), relativePos);
      return BookmarkManager.INSTANCE.getTrack(trackId);
    }
    else
    {
      final int pos = calculateBookmarkPosition(getCategory(), position);
      final long bookmarkId = BookmarkManager.INSTANCE.getBookmarkIdByPosition(getCategory().getId(), pos);
      return BookmarkManager.INSTANCE.getBookmark(bookmarkId);
    }
  }

  @NonNull
  private BookmarkCategory getCategory()
  {
    return mDataSource.getData();
  }
}
