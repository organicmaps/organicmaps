package com.mapswithme.maps.pins;

import com.mapswithme.maps.pins.pins.PinManager;

import android.app.Activity;
import android.os.Bundle;

public abstract class AbstractPinActivity extends Activity
{
  protected PinManager mManager;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mManager = PinManager.getPinManager(getApplicationContext());
  }
}
