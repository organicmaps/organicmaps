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

public class SimpleNavigationFragment extends Fragment implements LocationService.Listener
{

  // Data
  private ParcelablePointD mPoint = new ParcelablePointD(0, 0);
  private LocationService mLocationService;
  private double mNorth = -1;

  //Views
    // Right
  private boolean mShowDynamic = true;
  private ArrowImage mArrow;
  private TextView mDistance;
    // Left
  private boolean mShowStatic = true;
  private TextView mDMS;
    // Containers
  private View mRoot;
  private View mStaticData;
  private View mDynamicData;

  public void             setPoint(ParcelablePointD point) { mPoint = point; }
  public ParcelablePointD getPoint()                       { return mPoint;  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    mRoot = inflater.inflate(R.layout.fragment_simple_navigation, container, false);
    mStaticData = mRoot.findViewById(R.id.staticData);
    mDynamicData = mRoot.findViewById(R.id.dynamicData);
    //Set up views
    mArrow    = (ArrowImage) mRoot.findViewById(R.id.arrow);
    mArrow.setDrawCircle(true);
    mDistance = (TextView)   mRoot.findViewById(R.id.distance);
    mDMS      = (TextView)   mRoot.findViewById(R.id.dms);

    setClickers();

    return mRoot;
  }

  private void updateViews()
  {
    final Location lastKnown = mLocationService.getLastKnown();
    if (lastKnown != null && mShowDynamic)
    {
      UiUtils.show(mDynamicData);
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
      UiUtils.hide(mDynamicData);

    if (mPoint != null && mShowStatic)
    {
      UiUtils.show(mStaticData);
      mDMS.setText(UiUtils.formatLatLonToDMS(mPoint.y, mPoint.x));
    }
    else
      UiUtils.hide(mStaticData);
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
  public void onPause()
  {
    mLocationService.stopUpdate(this);
    mLocationService = null;
    super.onPause();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    mLocationService = ((MWMApplication)getActivity().getApplication()).getLocationService();
    mLocationService.startUpdate(this);
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

  public void showStaticData(boolean show)
  {
    mShowStatic = show;
    if (show)
      UiUtils.show(mStaticData);
    else
      UiUtils.hide(mStaticData);
  }

  public void showDynamicData(boolean show)
  {
    mShowDynamic = show;

    if (show)
      UiUtils.show(mDynamicData);
    else
      UiUtils.hide(mDynamicData);
  }


  private static final int MENU_COPY_DMS  = 1;
  private static final int MENU_COPY_DEGR = 2;
  private static final int MENU_CANCEL    = 999;
  @Override
  public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
  {
    if (v == mStaticData)
    {
      // TODO add localizations
      menu.add(Menu.NONE, MENU_COPY_DEGR, MENU_COPY_DEGR, String.format("Copy %s", UiUtils.formatLatLon(mPoint.y, mPoint.x)));
      menu.add(Menu.NONE, MENU_COPY_DMS, MENU_COPY_DMS, String.format("Copy %s", mDMS.getText().toString()));
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
      text = mDMS.getText().toString();
    }
    else if (ID == MENU_CANCEL)
      return true;
    else
      super.onContextItemSelected(item);

    if (text != null)
    {
      Utils.copyTextToClipboard(getActivity(), text);
      // TODO add localization
      Utils.toastShortcut(getActivity(), "Copied to Clipboard: " + text);
      return true;
    }

    return false;
  }

  private void showShareCoordinatesMenu()
  {
    registerForContextMenu(mStaticData);
    mStaticData.showContextMenu();
    unregisterForContextMenu(mStaticData);
  }

  private void setClickers()
  {
    mStaticData.setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        showShareCoordinatesMenu();
      }
    });
  }
}
