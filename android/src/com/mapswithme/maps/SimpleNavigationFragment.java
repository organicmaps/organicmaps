package com.mapswithme.maps;

import android.app.Activity;
import android.location.Location;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.bookmarks.data.ParcelablePointD;
import com.mapswithme.maps.location.LocationService;
import com.mapswithme.util.UiUtils;

public class SimpleNavigationFragment extends Fragment implements LocationService.Listener
{

  // Data
  private ParcelablePointD mPoint = new ParcelablePointD(0, 0);
  private LocationService mLocationService;
  private double mNorth = -1;

  //Views

  // Right
  private ArrowImage mArrow;
  private TextView mDistance;
  // Left
  private TextView mDegrees;
  private TextView mDMS;



  public void             setPoint(ParcelablePointD point) { mPoint = point; }
  public ParcelablePointD getPoint()                       { return mPoint;  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    View view = inflater.inflate(R.layout.fragment_simple_navigation, container, false);
    //Set up views
    mArrow    = (ArrowImage) view.findViewById(R.id.arrow);
    mDistance = (TextView)   view.findViewById(R.id.distance);
    mDegrees  = (TextView)   view.findViewById(R.id.degrees);
    mDMS      = (TextView)   view.findViewById(R.id.dms);

    updateViews();

    return view;
  }

  private void updateViews()
  {
    final Location lastKnown = mLocationService.getLastKnown();
    if (lastKnown != null)
    {
      final DistanceAndAzimut da = Framework.getDistanceAndAzimutFromLatLon
                (mPoint.y, mPoint.x, lastKnown.getLatitude(), lastKnown.getLongitude(), mNorth);

      final double azimut = da.getAthimuth();
      if (azimut >= 0)
        mArrow.setAzimut(da.getAthimuth());
      else
        mArrow.clear();

      mDistance.setText(da.getDistance());
    }
    else
    {
      //TODO set view for no location
    }

    if (mPoint != null && mDegrees != null)
    {
      mDegrees.setText(UiUtils.formatLatLon(mPoint.y, mPoint.x));
      // TODO add latLon to DMS conversion
    }
    else
    {
      // TODO hide?
    }
  }

  @Override
  public void onLocationUpdated(long time, double lat, double lon, float accuracy)
  {
    updateViews();
  }

  @Override
  public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    // copied from BookmarkListAdapter
    // TODO made more general
    double north[] = { magneticNorth, trueNorth };
    mLocationService.correctCompassAngles(getActivity().getWindowManager().getDefaultDisplay(), north);
    final double ret = (north[1] >= 0.0 ? north[1] : north[0]);

    // if difference is more than 1 degree
    if (mNorth == -1 || Math.abs(mNorth - ret) > 0.02)
    {
      mNorth = ret;
      updateViews();
    }
  }

  @Override
  public void onLocationError(int errorCode)
  {
    // TODO show something frightening
  }

  @Override
  public void onAttach(Activity activity)
  {
    super.onAttach(activity);
    mLocationService = ((MWMApplication)activity.getApplication()).getLocationService();
    mLocationService.startUpdate(this);
  }

  @Override
  public void onDetach()
  {
    super.onDetach();
    mLocationService.stopUpdate(this);
    mLocationService = null;
  }

}
