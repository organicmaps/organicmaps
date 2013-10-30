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


public class BookmarkListAdapter extends BaseAdapter implements LocationService.Listener
{
  private final Activity mContext;
  private final BookmarkCategory mCategory;
  private double mNorth = -1;
  private final LocationService mLocation;

  public BookmarkListAdapter(Activity context, LocationService location,
                             BookmarkCategory cat)
  {
    mContext = context;
    mLocation = location;
    mCategory = cat;
  }

  public void startLocationUpdate()
  {
    mLocation.startUpdate(this);
  }

  public void stopLocationUpdate()
  {
    mLocation.stopUpdate(this);
  }

  @Override
  public int getViewTypeCount()
  {
    return 2; // bookmark + track
  }
  final static int TYPE_TRACK = 0;
  final static int TYPE_BMK   = 1;

  @Override
  public int getItemViewType(int position)
  {
    return position < mCategory.getTracksCount() ? TYPE_TRACK : TYPE_BMK;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    final int type = getItemViewType(position);
    if (convertView == null)
    {
      final int lId = type == TYPE_BMK ? R.layout.list_item_bookmark : R.layout.list_item_track;
      convertView = LayoutInflater.from(mContext).inflate(lId, null);
      convertView.setTag(new PinHolder(convertView));
    }
    final PinHolder holder = (PinHolder) convertView.getTag();

    if (type == TYPE_BMK)
      holder.set(mCategory.getBookmark(position - mCategory.getTracksCount()));
    else
      holder.set(mCategory.getTrack(position));

    return convertView;
  }

  @Override
  public int getCount()
  {
    return mCategory.getSize();
  }

  @Override
  public Object getItem(int position)
  {
    if (getItemViewType(position) == TYPE_TRACK)
      return mCategory.getTrack(position);
    else
      return mCategory.getBookmark(position - mCategory.getTracksCount());
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
    final double north[] = { magneticNorth, trueNorth };
    mLocation.correctCompassAngles(mContext.getWindowManager().getDefaultDisplay(), north);
    final double ret = (north[1] >= 0.0 ? north[1] : north[0]);

    // if difference is more than 1 degree
    if (mNorth == -1 || Math.abs(mNorth - ret) > 0.02)
    {
      mNorth = ret;
      //Log.d(TAG, "Compass updated, north = " + m_north);

      notifyDataSetChanged();
    }
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
      final Location loc = mLocation.getLastKnown();
      if (loc != null)
      {
        final DistanceAndAzimut daa = bmk.getDistanceAndAzimut(loc.getLatitude(), loc.getLongitude(), mNorth);
        distance.setText(daa.getDistance());
      }
      else
        distance.setText(null);
    }

    void setDistance(Track trk)
    {
      distance.setText(trk.getLengthString());
    }

    void setIcon(Bookmark bmk)
    {
      icon.setImageBitmap(bmk.getIcon().getIcon());
    }

    void setIcon(Track trk)
    {
      final Resources res = mContext.getResources();
      final Drawable circle = UiUtils.drawCircle(trk.getColor(), (int) (res.getDimension(R.dimen.icon_size)), res);
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
}
