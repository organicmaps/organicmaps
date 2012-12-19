package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.ArrowImage;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.DistanceAndAthimuth;
import com.mapswithme.maps.bookmarks.data.ParcelablePointD;
import com.mapswithme.maps.location.LocationService;

public class BookmarkListAdapter extends BaseAdapter implements LocationService.Listener
{
  Activity mContext;
  BookmarkCategory mCategory;
  private double m_north = -1;
  private boolean m_hasPosition;
  private ParcelablePointD m_currentPosition;

  public BookmarkListAdapter(Activity context, BookmarkCategory cat)
  {
    mContext = context;
    mCategory = cat;
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    if (convertView == null)
    {
      convertView = LayoutInflater.from(mContext).inflate(R.layout.pin_item, null);
      convertView.setTag(new PinHolder((ArrowImage) convertView.findViewById(R.id.pi_arrow), (ImageView) convertView
          .findViewById(R.id.pi_pin_color), (TextView) convertView.findViewById(R.id.pi_name), (TextView) convertView
          .findViewById(R.id.pi_distance)));
    }
    Bookmark item = mCategory.getBookmark(position);
    PinHolder holder = (PinHolder) convertView.getTag();
    holder.name.setText(item.getName());
    holder.icon.setImageBitmap(item.getIcon().getIcon());
    if (m_hasPosition)
    {
      DistanceAndAthimuth daa = item.getDistanceAndAthimuth(m_currentPosition.x, m_currentPosition.y, m_north);
      holder.distance.setText(daa.getDistance());
      holder.arrow.setAzimut(daa.getAthimuth());
    }
    else
    {
      holder.distance.setText("");
      holder.arrow.setImageDrawable(null);
    }
    Log.d("lat lot", item.getLat() + " " + item.getLon());
    return convertView;
  }

  private static class PinHolder
  {
    ArrowImage arrow;
    ImageView icon;
    TextView name;
    TextView distance;

    public PinHolder(ArrowImage arrow, ImageView icon, TextView name, TextView distance)
    {
      super();
      this.arrow = arrow;
      this.icon = icon;
      this.name = name;
      this.distance = distance;
    }
  }

  @Override
  public int getCount()
  {
    return mCategory.getSize();
  }

  @Override
  public Bookmark getItem(int position)
  {
    return mCategory.getBookmark(position);
  }

  @Override
  public long getItemId(int position)
  {
    return position;
  }

  @Override
  public void onLocationUpdated(long time, double lat, double lon,
                                float accuracy)
  {
    m_currentPosition = new ParcelablePointD(lat, lon);
    m_hasPosition = true;
    notifyDataSetChanged();
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    @SuppressWarnings("deprecation")
    final int orientation = mContext.getWindowManager().getDefaultDisplay().getOrientation();
    final double correction = LocationService.getAngleCorrection(orientation);

    final double north = LocationService.correctAngle(trueNorth, correction);

    // if difference is more than 1 degree
    if (m_north  == -1 || Math.abs(m_north - north) > 0.02)
    {
      m_north = north;
      //Log.d(TAG, "Compass updated, north = " + m_north);

      notifyDataSetChanged();
    }
  }

  @Override
  public void onLocationError(int errorCode)
  {
    m_hasPosition = false;
    notifyDataSetChanged();
  }
}
