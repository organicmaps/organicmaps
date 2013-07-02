package com.mapswithme.maps;

import android.location.Location;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.bookmarks.data.ParcelablePointD;
import com.mapswithme.maps.location.LocationService;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.StubLogger;

public class SimpleNavigationFragment extends Fragment implements LocationService.Listener
{

  private Logger mLogger = StubLogger.get();//SimpleLogger.get(SimpleNavigationFragment.class.getSimpleName().toString());

  // Data
  private ParcelablePointD mPoint = new ParcelablePointD(0, 0);
  private LocationService mLocationService;
  private double mNorth = -1;
  private boolean mDoListenForLocation = true;

  private ArrowImage mArrow;
  private TextView mDistance;
  private TextView mCoords;
  private View mRoot;

  public void             setPoint(ParcelablePointD point) { mPoint = point; }
  public ParcelablePointD getPoint()                       { return mPoint;  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    mRoot = inflater.inflate(R.layout.fragment_simple_navigation, container, false);
    //Set up views
    mArrow = (ArrowImage) mRoot.findViewById(R.id.arrow);
    mArrow.setDrawCircle(true);
    mDistance = (TextView) mRoot.findViewById(R.id.distance);
    mCoords = (TextView) mRoot.findViewById(R.id.coords);

    setClickers();

    return mRoot;
  }

  public void setListenForLocation(boolean doListen)
  {
    mDoListenForLocation = doListen;
  }

  private void updateViews()
  {
    mLogger.d("Updating view");

    if (mDoListenForLocation)
    {
      final Location lastKnown = mLocationService.getLastKnown();
      mLogger.d("Last known ", lastKnown);
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
        UiUtils.show(mDistance);
      }
      else
      {
        mArrow.clear();
        UiUtils.hide(mDistance);
      }
    }
    else
    {
      UiUtils.hide(mArrow);
      UiUtils.hide(mDistance);
    }

    if (mPoint != null)
    {
      mCoords.setText(UiUtils.formatLatLon(mPoint.y, mPoint.x));
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
  public void onPause()
  {
    if (mLocationService != null)
    {
      mLocationService.stopUpdate(this);
      mLocationService = null;
    }
    super.onPause();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    if (mDoListenForLocation)
    {
      mLocationService = ((MWMApplication)getActivity().getApplication()).getLocationService();
      mLocationService.startUpdate(this);
    }
    updateViews();
  }

  public void show()
  {
    UiUtils.show(mRoot);
  }

  public void hide()
  {
    UiUtils.hide(mRoot);
  }

  private static final int MENU_COPY_DMS  = 1;
  private static final int MENU_COPY_DEGR = 2;
  private static final int MENU_CANCEL    = 999;
  @Override
  public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
  {
    if (v == mCoords)
    {
      final String copyText = getResources().getString(android.R.string.copy);
      menu.add(Menu.NONE, MENU_COPY_DEGR, MENU_COPY_DEGR, String.format("%s %s", copyText, UiUtils.formatLatLon(mPoint.y, mPoint.x)));
      menu.add(Menu.NONE, MENU_COPY_DMS, MENU_COPY_DMS, String.format("%s %s", copyText, UiUtils.formatLatLonToDMS(mPoint.y, mPoint.x)));
      menu.add(Menu.NONE, MENU_CANCEL, MENU_CANCEL, android.R.string.cancel);
    }

    super.onCreateContextMenu(menu, v, menuInfo);
  }

  @Override
  public boolean onContextItemSelected(MenuItem item)
  {
    final int ID = item.getItemId();
    String text = null;
    if (ID == MENU_COPY_DEGR)
    {
      text = UiUtils.formatLatLon(mPoint.y, mPoint.x);
    }
    else if (ID == MENU_COPY_DMS)
    {
      text = UiUtils.formatLatLonToDMS(mPoint.y, mPoint.x);
    }
    else if (ID == MENU_CANCEL)
      return true;
    else
      super.onContextItemSelected(item);

    if (text != null)
    {
      Utils.copyTextToClipboard(getActivity(), text);
      Utils.toastShortcut(getActivity(), getString(R.string.copied_to_clipboard, text));
      return true;
    }

    return false;
  }

  private void showShareCoordinatesMenu()
  {
    registerForContextMenu(mCoords);
    mCoords.showContextMenu();
    unregisterForContextMenu(mCoords);
  }

  private void setClickers()
  {
    mCoords.setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        showShareCoordinatesMenu();
      }
    });
  }
}
