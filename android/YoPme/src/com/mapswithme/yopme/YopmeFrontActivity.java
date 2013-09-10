package com.mapswithme.yopme;

import com.mapswithme.maps.api.MWMPoint;
import com.mapswithme.maps.api.MWMResponse;
import com.mapswithme.maps.api.MapsWithMeApi;
import com.mapswithme.maps.api.MwmRequest;
import com.mapswithme.yopme.BackscreenActivity.Mode;

import android.os.Bundle;
import android.app.ActionBar;
import android.app.Activity;
import android.app.PendingIntent;
import android.content.Intent;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.PopupMenu;
import android.widget.Toast;
import android.widget.PopupMenu.OnMenuItemClickListener;
import android.widget.TextView;

public class YopmeFrontActivity extends Activity
                                implements OnClickListener
{
  private TextView   mSelectedLocation;
  private View mMenu;

  private Mode mMode;
  private MWMPoint mPoint;
  private final static String KEY_MODE = "key.mode";
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

    final ActionBar actionBar = getActionBar();
    actionBar.setDisplayShowTitleEnabled(false);
    actionBar.setDisplayShowHomeEnabled(false);
    actionBar.setDisplayUseLogoEnabled(false);
    actionBar.setDisplayShowCustomEnabled(true);
    actionBar.setCustomView(R.layout.action_bar_view);

    setUpView();

    //restore
    if (savedInstanceState != null)
    {
      mMode  = (Mode) savedInstanceState.getSerializable(KEY_MODE);
      mPoint = (MWMPoint) savedInstanceState.getSerializable(KEY_POINT);

     if (Mode.LOCATION == mMode)
       setLocationView();
     else if (Mode.POI == mMode)
       mSelectedLocation.setText(mPoint.getName());
    }

    setUpListeners();

    if (isIntroNeeded())
      showIntro();
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
        mSelectedLocation.setText(mPoint.getName());
        BackscreenActivity.startInMode(this, Mode.POI, mPoint);

        Toast.makeText(this, R.string.toast_poi, Toast.LENGTH_LONG).show();
      }

    }
  }

  private void setUpView()
  {
    mSelectedLocation   = (TextView) findViewById(R.id.selectedLocation);
    mMenu = findViewById(R.id.menu);
  }

  private void setUpListeners()
  {
    findViewById(R.id.poi).setOnClickListener(this);
    findViewById(R.id.me).setOnClickListener(this);
    mMenu.setOnClickListener(this);
  }

  @Override
  public void onClick(View v)
  {
    if (R.id.me == v.getId())
    {
      BackscreenActivity.startInMode(getApplicationContext(), Mode.LOCATION, null);
      Toast.makeText(this, R.string.toast_your_location, Toast.LENGTH_LONG).show();
      setLocationView();
    }
    else if (R.id.poi == v.getId())
    {
      final MwmRequest request = new MwmRequest()
                                   .setCustomButtonName(getString(R.string.pick_point_button_name))
                                   .setPendingIntent(getPickPointPendingIntent())
                                   .setTitle(getString(R.string.app_name))
                                   .setPickPointMode(true);
      MapsWithMeApi.sendRequest(this, request);
    }
    else if (R.id.menu == v.getId())
    {
      final PopupMenu popupMenu = new PopupMenu(this, mMenu);
      popupMenu.setOnMenuItemClickListener(new OnMenuItemClickListener()
      {
        @Override
        public boolean onMenuItemClick(MenuItem item)
        {
          if (item.getItemId() == R.id.menu_help)
          {
            startActivity(new Intent(getApplicationContext(), ReferenceActivity.class));
            return true;
          }
          return false;
        }
      });
      popupMenu.inflate(R.menu.yopme_main);
      popupMenu.show();
    }
  }

  private final static String EXTRA_PICK = ".pick_point";
  private PendingIntent getPickPointPendingIntent()
  {
    final Intent i = new Intent(this, YopmeFrontActivity.class);
    i.putExtra(EXTRA_PICK, true);
    return PendingIntent.getActivity(this, 0, i, PendingIntent.FLAG_UPDATE_CURRENT);
  }

  private void setLocationView()
  {
    mSelectedLocation.setText(getString(R.string.poi_label));
  }

  ImageView mIntro;
  private void showIntro()
  {
    mIntro = new ImageView(this);
    mIntro.setImageResource(R.drawable.introduction);
    ((ViewGroup)getWindow().getDecorView()).addView(mIntro);
    mIntro.setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        mIntro.setVisibility(View.GONE);
        markIntroShown();
      }
    });
  }

  private final static String PREFS = "prefs.xml";
  private final static String KEY_INTRO = "intro";

  private boolean isIntroNeeded()
  {
    return getSharedPreferences(PREFS, 0).getBoolean(KEY_INTRO, true);
  }

  private void markIntroShown()
  {
    getSharedPreferences(PREFS, 0).edit().putBoolean(KEY_INTRO, false).apply();
  }
}
