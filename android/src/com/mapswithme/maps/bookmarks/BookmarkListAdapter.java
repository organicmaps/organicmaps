package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.location.Location;
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
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.location.LocationService;


public class BookmarkListAdapter extends BaseAdapter implements LocationService.Listener
{
  private Activity mContext;
  private BookmarkCategory mCategory;
  private double mNorth = -1;
  private LocationService mLocation;
  private DataChangedListener mDataChangedListener;

  public BookmarkListAdapter(Activity context, LocationService location,
                             BookmarkCategory cat, DataChangedListener dcl)
  {
    mContext = context;
    mLocation = location;
    mCategory = cat;
    mDataChangedListener = dcl;
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
  public View getView(int position, View convertView, ViewGroup parent)
  {
    if (convertView == null)
    {
      convertView = LayoutInflater.from(mContext).inflate(R.layout.bookmark_item, null);
      convertView.setTag(new PinHolder(convertView));
    }

    Bookmark item = mCategory.getBookmark(position);
    PinHolder holder = (PinHolder) convertView.getTag();
    holder.name.setText(item.getName());
    holder.icon.setImageBitmap(item.getIcon().getIcon());

    final Location loc = mLocation.getLastKnown();
    if (loc != null)
    {
      DistanceAndAzimut daa = item.getDistanceAndAzimut(loc.getLatitude(), loc.getLongitude(), mNorth);
      holder.distance.setText(daa.getDistance());

      if (daa.getAthimuth() >= 0.0)
        holder.arrow.setAzimut(daa.getAthimuth());
      else
        holder.arrow.clear();
    }
    else
    {
      holder.distance.setText("");
      holder.arrow.clear();
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
  public void onLocationUpdated(final Location l)
  {
    notifyDataSetChanged();
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    double north[] = { magneticNorth, trueNorth };
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

  private static class PinHolder
  {
    ArrowImage arrow;
    ImageView icon;
    TextView name;
    TextView distance;

    public PinHolder(View convertView)
    {
      arrow = (ArrowImage) convertView.findViewById(R.id.pi_arrow);
      icon = (ImageView) convertView.findViewById(R.id.pi_pin_color);
      name = (TextView) convertView.findViewById(R.id.pi_name);
      distance = (TextView) convertView.findViewById(R.id.pi_distance);
    }
  }

  public interface DataChangedListener
  {
    void onDataChanged(int vis);
  }
}
