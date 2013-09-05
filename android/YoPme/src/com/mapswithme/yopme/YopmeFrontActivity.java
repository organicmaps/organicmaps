package com.mapswithme.yopme;

import com.mapswithme.maps.api.MWMPoint;
import com.mapswithme.maps.api.MWMResponse;
import com.mapswithme.maps.api.MapsWithMeApi;
import com.mapswithme.yopme.BackscreenActivity.Mode;

import android.os.Bundle;
import android.app.Activity;
import android.app.PendingIntent;
import android.content.Intent;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.RadioGroup;
import android.widget.RadioGroup.OnCheckedChangeListener;
import android.widget.TextView;

public class YopmeFrontActivity extends Activity
                                implements OnClickListener, OnCheckedChangeListener
{

  private RadioGroup mModeGroup;
  private Button     mSelectPoi;
  private TextView   mPoiText;

  private Mode mMode;
  private final static String KEY_MODE = "key.mode";
  private MWMPoint mPoint;
  private final static String KEY_POINT = "key.point";

  @Override
  protected void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);

    outState.putSerializable(KEY_MODE, mMode);
    outState.putSerializable(KEY_POINT, mPoint);
  }

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_yopme_main);

    setUpView();

    //restore
    if (savedInstanceState != null)
    {
      mMode  = (Mode) savedInstanceState.getSerializable(KEY_MODE);
      mPoint = (MWMPoint) savedInstanceState.getSerializable(KEY_POINT);

     if (Mode.LOCATION == mMode)
       setLocationView();
     else if (Mode.POI == mMode)
     {
       setPoiView();
       mPoiText.setText(mPoint.getName());
     }
    }

    setUpListeners();
  }

  @Override
  protected void onNewIntent(Intent intent)
  {
    super.onNewIntent(intent);

    if (intent.hasExtra(EXTRA_PICK) && intent.getBooleanExtra(EXTRA_PICK, false))
    {
      final MWMResponse response = MWMResponse.extractFromIntent(this, intent);
      if (response.hasPoint())
      {
        mPoint = response.getPoint();
        mPoiText.setText(mPoint.getName());
        BackscreenActivity.startInMode(this, Mode.POI, mPoint);
      }

    }
  }

  private void setUpView()
  {
    mModeGroup = (RadioGroup) findViewById(R.id.mode);
    mSelectPoi = (Button) findViewById(R.id.selectPoi);
    mPoiText   = (TextView) findViewById(R.id.poi);
  }

  private void setUpListeners()
  {
    mModeGroup.setOnCheckedChangeListener(this);
    mSelectPoi.setOnClickListener(this);
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu)
  {
    getMenuInflater().inflate(R.menu.yopme_main, menu);
    return true;
  }

  @Override
  public void onClick(View v)
  {
    MapsWithMeApi.pickPoint(this, "Pick point", getPickPointPendingIntent());
  }

  private final static String EXTRA_PICK = ".pick_point";
  private PendingIntent getPickPointPendingIntent()
  {
    final Intent i = new Intent(this, YopmeFrontActivity.class);
    i.putExtra(EXTRA_PICK, true);
    return PendingIntent.getActivity(this, 0, i, PendingIntent.FLAG_UPDATE_CURRENT);
  }

  @Override
  public void onCheckedChanged(RadioGroup group, int checkedId)
  {
    if (checkedId == R.id.modeLocation)
    {
      BackscreenActivity.startInMode(getApplicationContext(), Mode.LOCATION, null);
      setLocationView();
    }
    else if (checkedId == R.id.modePoi)
      setPoiView();
  }

  private void setPoiView()
  {
    mPoiText.setVisibility(View.VISIBLE);
    mPoiText.setText(null);
    mSelectPoi.setEnabled(true);
  }

  private void setLocationView()
  {
    mPoiText.setVisibility(View.GONE);
    mPoiText.setText(null);
    mSelectPoi.setEnabled(false);
  }
}
