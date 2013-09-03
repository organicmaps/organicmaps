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

  private State mState;

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

  @Override
  protected void onPause()
  {
    super.onPause();
    saveToState();
  }


  private void saveToState()
  {
    State.write(this, mState);
  }

  private void restoreFromState()
  {
    mModeGroup.clearCheck();

    final State st = new State();
    if (State.read(this, st))
    {
      mState = st;

      if (st.mMode == Mode.LOCATION)
        mModeGroup.check(R.id.modeLocation);
      else if (st.mMode == Mode.POI)
      {
        mModeGroup.check(R.id.modePoi);
      }
    }
    else
    {
      // Default initialization
      mState = new State();
      mState.mMode = Mode.LOCATION;
      mModeGroup.check(R.id.modeLocation);
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
    if (mState == null)
      return;

    if (checkedId == R.id.modeLocation)
    {
      BackscreenActivity.startInMode(getApplicationContext(), Mode.LOCATION);
      mState.mMode = Mode.LOCATION;
      // TODO: get address from MWM?
      mPoiText.setVisibility(View.GONE);
      mSelectPoi.setEnabled(false);
    }
    else if (checkedId == R.id.modePoi)
    {
      BackscreenActivity.startInMode(getApplicationContext(), Mode.POI);
      mState.mMode = Mode.POI;
      mPoiText.setVisibility(View.VISIBLE);
      mPoiText.setText(mState.mData.getPoint().getName());
      mSelectPoi.setEnabled(true);
    }
    else
      throw new IllegalArgumentException("What?");
  }

}
