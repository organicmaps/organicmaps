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
import static com.mapswithme.maps.bookmarks.Holders.BookmarkViewHolder.calculateTrackPosition;

public class BookmarkListAdapter extends RecyclerView.Adapter<Holders.BaseBookmarkHolder>
{
  @NonNull
  private final BookmarkCategory mCategory;

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

  BookmarkListAdapter(@NonNull BookmarkCategory category)
  {
    mCategory = category;
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
                                                              false), mCategory);
        break;
      case TYPE_BOOKMARK:
        holder = new Holders.BookmarkViewHolder(inflater.inflate(R.layout.item_bookmark, parent,
                                                                 false), mCategory);
        break;
      case TYPE_SECTION:
        TextView tv = (TextView) inflater.inflate(R.layout.item_category_title, parent, false);
        holder = new Holders.SectionViewHolder(tv, mCategory);
        break;
      case TYPE_DESC:
        View desc = inflater.inflate(R.layout.item_category_description, parent, false);
        holder = new Holders.DescriptionViewHolder(desc, mCategory);
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
    holder.bind(position);
  }

  @Override
  public int getItemViewType(int position)
  {
    final int descPos = getDescSectionPosition(mCategory);
    final int bmkPos = getBookmarksSectionPosition(mCategory);
    final int trackPos = getTracksSectionPosition(mCategory);

    if (position == bmkPos || position == trackPos || position == descPos)
      return TYPE_SECTION;

    if (position > bmkPos && !isSectionEmpty(mCategory, SECTION_BMKS))
      return TYPE_BOOKMARK;
    else if (position > trackPos && !isSectionEmpty(mCategory, SECTION_TRACKS))
      return TYPE_TRACK;
    else if (position > descPos && !isSectionEmpty(mCategory, SECTION_DESC))
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
    return mCategory.size()
           + (isSectionEmpty(mCategory, SECTION_TRACKS) ? 0 : 1)
           + (isSectionEmpty(mCategory, SECTION_BMKS) ? 0 : 1)
           + getDescItemCount(mCategory);
  }

  // FIXME: remove this heavy method and use BoomarkInfo class instead.
  public Object getItem(int position)
  {
    if (getItemViewType(position) == TYPE_DESC)
      throw new UnsupportedOperationException("Not supported here! Position = " + position);

    if (getItemViewType(position) == TYPE_TRACK)
    {
      int relativePos = calculateTrackPosition(mCategory, position);
      final long trackId = BookmarkManager.INSTANCE.getTrackIdByPosition(mCategory.getId(), relativePos);
      return BookmarkManager.INSTANCE.getTrack(trackId);
    }
    else
    {
      final int pos = calculateBookmarkPosition(mCategory, position);
      final long bookmarkId = BookmarkManager.INSTANCE.getBookmarkIdByPosition(mCategory.getId(), pos);
      return BookmarkManager.INSTANCE.getBookmark(bookmarkId);
    }
  }
}
