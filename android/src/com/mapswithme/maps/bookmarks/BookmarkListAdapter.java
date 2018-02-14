package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.bookmarks.data.Track;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationListener;
import com.mapswithme.util.Graphics;


public class BookmarkListAdapter extends BaseAdapter
{
  private final Activity mActivity;
  private final long mCategoryId;

  // view types
  static final int TYPE_TRACK = 0;
  static final int TYPE_BOOKMARK = 1;
  static final int TYPE_SECTION = 2;

  private static final int SECTION_TRACKS = 0;
  private static final int SECTION_BMKS = 1;

  private final LocationListener mLocationListener = new LocationListener.Simple()
  {
    @Override
    public void onLocationUpdated(Location location)
    {
      notifyDataSetChanged();
    }
  };

  public BookmarkListAdapter(Activity activity, long catId)
  {
    mActivity = activity;
    mCategoryId = catId;
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
  public int getViewTypeCount()
  {
    return 3; // bookmark + track + section
  }

  @Override
  public int getItemViewType(int position)
  {
    final int bmkPos = getBookmarksSectionPosition();
    final int trackPos = getTracksSectionPosition();

    if (position == bmkPos || position == trackPos)
      return TYPE_SECTION;

    if (position > bmkPos && !isSectionEmpty(SECTION_BMKS))
      return TYPE_BOOKMARK;
    else if (position > trackPos && !isSectionEmpty(SECTION_TRACKS))
      return TYPE_TRACK;

    throw new IllegalArgumentException("Position not found: " + position);
  }

  @Override
  public boolean isEnabled(int position)
  {
    return getItemViewType(position) != TYPE_SECTION;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    final int type = getItemViewType(position);

    if (type == TYPE_SECTION)
    {
      TextView sectionView;

      if (convertView == null)
        sectionView = (TextView) LayoutInflater.from(mActivity).inflate(R.layout.item_category_title, parent, false);
      else
        sectionView = (TextView) convertView;

      final int sectionIndex = getSectionForPosition(position);
      sectionView.setText(getSections().get(sectionIndex));
      return sectionView;
    }

    if (convertView == null)
    {
      final int id = (type == TYPE_BOOKMARK) ? R.layout.item_bookmark : R.layout.item_track;
      convertView = LayoutInflater.from(mActivity).inflate(id, parent, false);
      convertView.setTag(new PinHolder(convertView));
    }

    final PinHolder holder = (PinHolder) convertView.getTag();
    if (type == TYPE_BOOKMARK)
      holder.set((Bookmark) getItem(position));
    else
      holder.set((Track) getItem(position));

    return convertView;
  }

  @Override
  public int getCount()
  {
    return BookmarkManager.INSTANCE.getCategorySize(mCategoryId)
           + (isSectionEmpty(SECTION_TRACKS) ? 0 : 1)
           + (isSectionEmpty(SECTION_BMKS) ? 0 : 1);
  }

  @Override
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
                      - (isSectionEmpty(SECTION_TRACKS) ? 0 : BookmarkManager.INSTANCE.getTracksCount(mCategoryId) + 1);
      final long bookmarkId = BookmarkManager.INSTANCE.getBookmarkIdByPosition(mCategoryId, pos);
      return BookmarkManager.INSTANCE.getBookmark(bookmarkId);
    }
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  private class PinHolder
  {
    ImageView icon;
    TextView name;
    TextView distance;

    public PinHolder(View convertView)
    {
      icon = (ImageView) convertView.findViewById(R.id.iv__bookmark_color);
      name = (TextView) convertView.findViewById(R.id.tv__bookmark_name);
      distance = (TextView) convertView.findViewById(R.id.tv__bookmark_distance);
    }

    void setName(Bookmark bmk)
    {
      name.setText(bmk.getTitle());
    }

    void setName(Track trk)
    {
      name.setText(trk.getName());
    }

    void setDistance(Bookmark bmk)
    {
      final Location loc = LocationHelper.INSTANCE.getSavedLocation();
      if (loc != null)
      {
        final DistanceAndAzimut daa = bmk.getDistanceAndAzimuth(loc.getLatitude(), loc.getLongitude(), 0.0);
        distance.setText(daa.getDistance());
      }
      else
        distance.setText(null);
    }

    void setDistance(Track trk)
    {
      distance.setText(mActivity.getString(R.string.length) + " " + trk.getLengthString());
    }

    void setIcon(Bookmark bookmark)
    {
      icon.setImageResource(bookmark.getIcon().getSelectedResId());
    }

    void setIcon(Track trk)
    {
      final Drawable circle = Graphics.drawCircle(trk.getColor(), R.dimen.track_circle_size, mActivity.getResources());
      icon.setImageDrawable(circle);
    }

    void set(Bookmark bmk)
    {
      setName(bmk);
      setDistance(bmk);
      setIcon(bmk);
    }

    void set(Track track)
    {
      setName(track);
      setDistance(track);
      setIcon(track);
    }
  }

  private int getTracksSectionPosition()
  {
    if (isSectionEmpty(SECTION_TRACKS))
      return -1;

    return 0;
  }

  private int getBookmarksSectionPosition()
  {
    if (isSectionEmpty(SECTION_BMKS))
      return -1;

    return BookmarkManager.INSTANCE.getTracksCount(mCategoryId)
        + (isSectionEmpty(SECTION_TRACKS) ? 0 : 1);
  }

  private List<String> getSections()
  {
    final List<String> sections = new ArrayList<>();
    sections.add(mActivity.getString(R.string.tracks));
    sections.add(mActivity.getString(R.string.bookmarks));
    return sections;
  }

  private int getSectionForPosition(int position)
  {
    if (position == getTracksSectionPosition())
      return SECTION_TRACKS;
    if (position == getBookmarksSectionPosition())
      return SECTION_BMKS;

    throw new IllegalArgumentException("There is no section in position " + position);
  }

  private boolean isSectionEmpty(int section)
  {
    if (section == SECTION_TRACKS)
      return BookmarkManager.INSTANCE.getTracksCount(mCategoryId) == 0;
    if (section == SECTION_BMKS)
      return BookmarkManager.INSTANCE.getBookmarksCount(mCategoryId) == 0;

    throw new IllegalArgumentException("There is no section with index " + section);
  }
}
