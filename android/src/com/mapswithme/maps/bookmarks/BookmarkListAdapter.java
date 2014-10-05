package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.bookmarks.data.Track;
import com.mapswithme.maps.location.LocationService;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;


public class BookmarkListAdapter extends BaseAdapter
    implements LocationService.LocationListener
{
  private final Activity mContext;
  private final BookmarkCategory mCategory;

  // reuse drawables
  private final Map<String, Drawable> mBmkToCircle = new HashMap<String, Drawable>(8);

  public BookmarkListAdapter(Activity context, BookmarkCategory cat)
  {
    mContext = context;
    mCategory = cat;
  }

  public void startLocationUpdate()
  {
    LocationService.INSTANCE.startUpdate(this);
  }

  public void stopLocationUpdate()
  {
    LocationService.INSTANCE.stopUpdate(this);
  }

  @Override
  public int getViewTypeCount()
  {
    return 3; // bookmark + track + section
  }

  final static int TYPE_TRACK = 0;
  final static int TYPE_BMK = 1;
  final static int TYPE_SECTION = 2;

  @Override
  public int getItemViewType(int position)
  {
    final int bmkPos = getBookmarksSectionPosition();
    final int trackPos = getTracksSectionPosition();

    if (position == bmkPos || position == trackPos)
      return TYPE_SECTION;

    if (position > bmkPos && !isSectionEmpty(SECTION_BMKS))
      return TYPE_BMK;
    else if (position > trackPos && !isSectionEmpty(SECTION_TRACKS))
      return TYPE_TRACK;

    throw new IllegalArgumentException("Position not found: " + position);
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    final int type = getItemViewType(position);

    if (type == TYPE_SECTION)
    {
      View sectionView = null;
      TextView sectionName = null;

      if (convertView == null)
      {
        sectionView = LayoutInflater.from(mContext).inflate(R.layout.list_separator_base, null);
        sectionName = (TextView) sectionView.findViewById(R.id.text);
        sectionView.setTag(sectionName);
      }
      else
      {
        sectionView = convertView;
        sectionName = (TextView) sectionView.getTag();
      }

      final int sectionIndex = getSectionForPosition(position);
      sectionName.setText(getSections().get(sectionIndex));
      return sectionView;
    }

    if (convertView == null)
    {
      final int id = (type == TYPE_BMK) ? R.layout.list_item_bookmark : R.layout.list_item_track;
      convertView = LayoutInflater.from(mContext).inflate(id, null);
      convertView.setTag(new PinHolder(convertView));
    }

    final PinHolder holder = (PinHolder) convertView.getTag();
    if (type == TYPE_BMK)
      holder.set((Bookmark) getItem(position));
    else
      holder.set((Track) getItem(position));

    return convertView;
  }

  @Override
  public int getCount()
  {
    return mCategory.getSize()
        + (isSectionEmpty(SECTION_TRACKS) ? 0 : 1)
        + (isSectionEmpty(SECTION_BMKS) ? 0 : 1);
  }

  @Override
  public Object getItem(int position)
  {
    if (getItemViewType(position) == TYPE_TRACK)
      return mCategory.getTrack(position - 1);
    else
      return mCategory.getBookmark(position - 1
          - (isSectionEmpty(SECTION_TRACKS) ? 0 : mCategory.getTracksCount() + 1));
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public void onLocationUpdated(final Location l)
  {
    notifyDataSetChanged();
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    // We don't show any arrows for bookmarks any more.
  }

  @Override
  public void onLocationError(int errorCode)
  {
  }

  private class PinHolder
  {
    ImageView icon;
    TextView name;
    TextView distance;

    public PinHolder(View convertView)
    {
      icon = (ImageView) convertView.findViewById(R.id.pi_pin_color);
      name = (TextView) convertView.findViewById(R.id.pi_name);
      distance = (TextView) convertView.findViewById(R.id.pi_distance);
    }

    void setName(Bookmark bmk)
    {
      name.setText(bmk.getName());
    }

    void setName(Track trk)
    {
      name.setText(trk.getName());
    }

    void setDistance(Bookmark bmk)
    {
      final Location loc = LocationService.INSTANCE.getLastLocation();
      if (loc != null)
      {
        final DistanceAndAzimut daa = bmk.getDistanceAndAzimut(loc.getLatitude(), loc.getLongitude(), 0.0);
        distance.setText(daa.getDistance());
      }
      else
        distance.setText(null);
    }

    void setDistance(Track trk)
    {
      distance.setText(mContext.getString(R.string.length) + " " + trk.getLengthString());
    }

    void setIcon(Bookmark bmk)
    {
      final String key = bmk.getIcon().getType();
      Drawable circle = null;

      if (!mBmkToCircle.containsKey(key))
      {
        final Resources res = mContext.getResources();
        final int circleSize = (int) (res.getDimension(R.dimen.circle_size) + .5);
        circle = UiUtils.drawCircleForPin(key, circleSize, res);
        mBmkToCircle.put(key, circle);
      }
      else
        circle = mBmkToCircle.get(key);

      icon.setImageDrawable(circle);
    }

    void setIcon(Track trk)
    {
      final Resources res = mContext.getResources();
      final int circleSize = (int) (res.getDimension(R.dimen.circle_size) + .5);
      // colors could be different, so don't use cache
      final Drawable circle = UiUtils.drawCircle(trk.getColor(), circleSize, res);
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

  private final static int SECTION_TRACKS = 0;
  private final static int SECTION_BMKS = 1;

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

    return mCategory.getTracksCount()
        + (isSectionEmpty(SECTION_TRACKS) ? 0 : 1);
  }

  private List<String> getSections()
  {
    final List<String> sections = new ArrayList<String>();
    sections.add(mContext.getString(R.string.tracks));
    sections.add(mContext.getString(R.string.bookmarks));
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
      return mCategory.getTracksCount() == 0;
    if (section == SECTION_BMKS)
      return mCategory.getBookmarksCount() == 0;

    throw new IllegalArgumentException("There is no section with index " + section);
  }
}
