package com.mapswithme.maps.pins;

import com.mapswithme.maps.pins.pins.PinManager;

import android.app.ListActivity;
import android.os.Bundle;

public abstract class AbstractPinListActivity extends ListActivity
{
  protected PinManager mManager;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mManager = PinManager.getPinManager(getApplicationContext());
  }
}
