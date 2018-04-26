package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.location.Location;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationListener;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;
import com.mapswithme.maps.widget.recycler.RecyclerLongClickListener;

import static com.mapswithme.maps.bookmarks.Holders.BaseBookmarkHolder.SECTION_BMKS;
import static com.mapswithme.maps.bookmarks.Holders.BaseBookmarkHolder.SECTION_TRACKS;
import static com.mapswithme.maps.bookmarks.Holders.BaseBookmarkHolder.getTracksSectionPosition;

public class BookmarkListAdapter extends RecyclerView.Adapter<Holders.BaseBookmarkHolder>
{
  private final Activity mActivity;
  private final long mCategoryId;

  // view types
  static final int TYPE_TRACK = 0;
  static final int TYPE_BOOKMARK = 1;
  static final int TYPE_SECTION = 2;

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

  BookmarkListAdapter(Activity activity, long catId)
  {
    mActivity = activity;
    mCategoryId = catId;
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
    LayoutInflater inflater = LayoutInflater.from(mActivity);
    Holders.BaseBookmarkHolder holder = null;
    switch (viewType)
    {
      case TYPE_TRACK:
        holder = new Holders.TrackViewHolder(inflater.inflate(R.layout.item_track,
                                                            parent, false), mCategoryId);
        break;
      case TYPE_BOOKMARK:
        holder = new Holders.BookmarkViewHolder(inflater.inflate(R.layout.item_bookmark,
                                                               parent,false), mCategoryId);
        break;
      case TYPE_SECTION:
        holder = new Holders.SectionViewHolder((TextView) inflater.inflate(R.layout.item_category_title,
                                                              parent, false), mCategoryId);
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
    final int bmkPos = Holders.BaseBookmarkHolder.getBookmarksSectionPosition(mCategoryId);
    final int trackPos = getTracksSectionPosition(mCategoryId);

    if (position == bmkPos || position == trackPos)
      return TYPE_SECTION;

    if (position > bmkPos && !Holders.BaseBookmarkHolder.isSectionEmpty(mCategoryId, SECTION_BMKS))
      return TYPE_BOOKMARK;
    else if (position > trackPos && !Holders.BaseBookmarkHolder.isSectionEmpty(mCategoryId, SECTION_TRACKS))
      return TYPE_TRACK;

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
    return BookmarkManager.INSTANCE.getCategorySize(mCategoryId)
           + (Holders.BaseBookmarkHolder.isSectionEmpty(mCategoryId, SECTION_TRACKS) ? 0 : 1)
           + (Holders.BaseBookmarkHolder.isSectionEmpty(mCategoryId, SECTION_BMKS) ? 0 : 1);
  }

  public Object getItem(int position)
  {
    if (getItemViewType(position) == TYPE_TRACK)
    {
      final long trackId = BookmarkManager.INSTANCE.getTrackIdByPosition(mCategoryId, position - 1);
      return BookmarkManager.INSTANCE.getTrack(trackId);
    }
    else
    {
      final int pos = position - 1
                      - (Holders.BaseBookmarkHolder.isSectionEmpty(mCategoryId, SECTION_TRACKS)
                         ? 0 : BookmarkManager.INSTANCE.getTracksCount(mCategoryId) + 1);
      final long bookmarkId = BookmarkManager.INSTANCE.getBookmarkIdByPosition(mCategoryId, pos);
      return BookmarkManager.INSTANCE.getBookmark(bookmarkId);
    }
  }

}
