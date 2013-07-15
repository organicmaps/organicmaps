package com.mapswithme.test;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import com.mapswithme.maps.MWMActivity;
import com.mapswithme.maps.MWMActivity.OpenUrlTask;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;

public class TestEventsReceiver extends BroadcastReceiver
{


  Logger l = SimpleLogger.get("TestBro");

  @Override
  public void onReceive(Context context, Intent intent)
  {
    l.d(intent.toString());
    final String uri = intent.getStringExtra("uri");
    l.d(uri);

    MWMActivity.OpenUrlTask oTask = new OpenUrlTask(uri);

    intent = new Intent(context, MWMActivity.class);
    intent.putExtra(MWMActivity.EXTRA_TASK, oTask);
  }

}
