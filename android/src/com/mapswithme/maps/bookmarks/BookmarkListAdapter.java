package com.mapswithme.maps.bookmarks;

import android.app.Activity;
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
  private double mNorth = -1;
  private boolean mHasPosition;
  private ParcelablePointD mCurrentPosition;
  private DataChangedListener mDataChangedListener;

  public BookmarkListAdapter(Activity context, BookmarkCategory cat, DataChangedListener dcl)
  {
    mContext = context;
    mCategory = cat;
    mDataChangedListener = dcl;
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
    if (mHasPosition)
    {
      DistanceAndAthimuth daa = item.getDistanceAndAthimuth(mCurrentPosition.x, mCurrentPosition.y, mNorth);
      holder.distance.setText(daa.getDistance());
      holder.arrow.setAzimut(daa.getAthimuth());
    }
    else
    {
      holder.distance.setText("");
      holder.arrow.setImageDrawable(null);
    }
    //Log.d("lat lot", item.getLat() + " " + item.getLon());
    return convertView;
  }

  @Override
  public void notifyDataSetChanged()
  {
    super.notifyDataSetChanged();
    if (mDataChangedListener != null)
    {
      mDataChangedListener.onDataChanged(isEmpty() ? View.VISIBLE : View.GONE);
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
    mCurrentPosition = new ParcelablePointD(lat, lon);
    mHasPosition = true;
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
    if (mNorth  == -1 || Math.abs(mNorth - north) > 0.02)
    {
      mNorth = north;
      //Log.d(TAG, "Compass updated, north = " + m_north);

      notifyDataSetChanged();
    }
  }

  @Override
  public void onLocationError(int errorCode)
  {
    /// @TODO handle it in location service.
    mHasPosition = false;
    notifyDataSetChanged();
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

  public interface DataChangedListener
  {
    void onDataChanged(int vis);
  }
}
