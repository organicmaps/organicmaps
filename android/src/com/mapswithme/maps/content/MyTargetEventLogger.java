package com.mapswithme.maps.content;

import android.app.Application;
import android.support.annotation.NonNull;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.ads.Banner;

class MyTargetEventLogger extends DefaultEventLogger
{
  MyTargetEventLogger(@NonNull Application application)
  {
    super(application);
    Framework.addAdProvider(Banner.Type.TYPE_RB);
  }
}
