package app.organicmaps.widget.placepage;

import android.location.Location;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.Framework;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmDialogFragment;
import app.organicmaps.bookmarks.data.DistanceAndAzimut;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.location.LocationListener;
import app.organicmaps.widget.ArrowView;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;

public class DirectionFragment extends BaseMwmDialogFragment
                            implements LocationListener
{
  private static final String EXTRA_MAP_OBJECT = "MapObject";

  private ArrowView mAvDirection;
  private TextView mTvTitle;
  private TextView mTvSubtitle;
  private TextView mTvDistance;

  private MapObject mMapObject;

  @Override
  protected int getCustomTheme()
  {
    return R.style.MwmTheme_DialogFragment_Fullscreen_Translucent;
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    final View root = inflater.inflate(R.layout.fragment_direction, container, false);
    root.setOnTouchListener((v, event) -> {
      dismiss();
      return false;
    });
    initViews(root);
    if (savedInstanceState != null)
      setMapObject(Utils.getParcelable(savedInstanceState, EXTRA_MAP_OBJECT, MapObject.class));

    return root;
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    outState.putParcelable(EXTRA_MAP_OBJECT, mMapObject);
    super.onSaveInstanceState(outState);
  }

  private void initViews(View root)
  {
    mAvDirection = root.findViewById(R.id.av__direction);
    mTvTitle = root.findViewById(R.id.tv__title);
    mTvSubtitle = root.findViewById(R.id.tv__subtitle);
    mTvDistance = root.findViewById(R.id.tv__straight_distance);

    UiUtils.waitLayout(mTvTitle, new ViewTreeObserver.OnGlobalLayoutListener() {
      @Override
      public void onGlobalLayout()
      {
        final int height = mTvTitle.getHeight();
        final int lineHeight = mTvTitle.getLineHeight();
        mTvTitle.setMaxLines(height / lineHeight);
      }
    });
  }

  public void setMapObject(MapObject object)
  {
    mMapObject = object;
    refreshViews();
  }

  private void refreshViews()
  {
    if (mMapObject != null && isResumed())
    {
      mTvTitle.setText(mMapObject.getTitle());
      mTvSubtitle.setText(mMapObject.getSubtitle());
    }
  }


  @Override
  public void onResume()
  {
    super.onResume();
    LocationHelper.INSTANCE.addListener(this);
    refreshViews();
  }

  @Override
  public void onPause()
  {
    super.onPause();
    LocationHelper.INSTANCE.removeListener(this);
  }

  @Override
  public void onLocationUpdated(@NonNull Location location)
  {
    if (mMapObject != null)
    {
      final DistanceAndAzimut distanceAndAzimuth =
          Framework.nativeGetDistanceAndAzimuthFromLatLon(mMapObject.getLat(), mMapObject.getLon(),
                                                          location.getLatitude(), location.getLongitude(), 0.0);
      mTvDistance.setText(distanceAndAzimuth.getDistance());
    }
  }

  @Override
  public void onCompassUpdated(double north)
  {
    final Location last = LocationHelper.INSTANCE.getSavedLocation();
    if (last == null || mMapObject == null)
      return;

    final DistanceAndAzimut da = Framework.nativeGetDistanceAndAzimuthFromLatLon(
        mMapObject.getLat(), mMapObject.getLon(),
        last.getLatitude(), last.getLongitude(), north);

    if (da.getAzimuth() >= 0)
      mAvDirection.setAzimuth(da.getAzimuth());
  }
}
