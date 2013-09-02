package com.mapswithme.yopme;

import com.mapswithme.yopme.BackscreenActivity.Mode;

import android.os.Bundle;
import android.app.Activity;
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
  @SuppressWarnings("unused")
  private TextView   mPoiText;


  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_yopme_main);
    setUpView();

    BackscreenActivity.startInMode(getApplicationContext(), Mode.LOCATION);
  }

  private void setUpView()
  {
    mModeGroup = (RadioGroup) findViewById(R.id.mode);
    mSelectPoi = (Button) findViewById(R.id.selectPoi);
    mPoiText   = (TextView) findViewById(R.id.poi);

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
    //TODO add mapswithme invocation
  }

  @Override
  public void onCheckedChanged(RadioGroup group, int checkedId)
  {
    if (checkedId == R.id.modeLocation)
      BackscreenActivity.startInMode(getApplicationContext(), Mode.LOCATION);
    else if (checkedId == R.id.modePoi)
      BackscreenActivity.startInMode(getApplicationContext(), Mode.POI);
    else
      throw new IllegalArgumentException("What?");
  }

}
