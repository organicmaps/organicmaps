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
  private TextView   mPoiText;

  private State mState = new State();

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_yopme_main);

    setUpView();
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    restoreFromState();
  }

  private void saveState()
  {
    State.write(this, mState);
  }

  private void restoreFromState()
  {
    mModeGroup.check(-1);
    final State st = State.read(this);
    if (st != null)
    {
      mState = st;
      if (st.mMode == Mode.LOCATION)
        mModeGroup.check(R.id.modeLocation);
      else if (st.mMode == Mode.POI)
        mModeGroup.check(R.id.modePoi);
    }
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
    {
      BackscreenActivity.startInMode(getApplicationContext(), Mode.LOCATION);
      mState.setMode(Mode.LOCATION);
      mPoiText.setVisibility(View.GONE);
      //TODO: get location name

      mSelectPoi.setEnabled(false);
      saveState();
    }
    else if (checkedId == R.id.modePoi)
    {
      if (mState.hasPoint())
      {
        BackscreenActivity.startInMode(getApplicationContext(), Mode.POI);
        mPoiText.setText(mState.getPoint().getName());
        mPoiText.setVisibility(View.VISIBLE);
      }
      mState.setMode(Mode.POI);
      mSelectPoi.setEnabled(true);
      saveState();
    }
    else
    {
      mPoiText.setVisibility(View.VISIBLE);
      mPoiText.setText("Please select mode");
      mSelectPoi.setEnabled(false);
    }
  }
}
