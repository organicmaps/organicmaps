package com.mapswithme.util.push;

import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v4.app.JobIntentService;

import com.pushwoosh.internal.utils.NotificationRegistrarHelper;

public class GcmRouterJobIntentService extends JobIntentService
{
  @Override
  protected void onHandleWork(@NonNull Intent intent)
  {
    NotificationRegistrarHelper.handleMessage(intent.getExtras());
  }
}
