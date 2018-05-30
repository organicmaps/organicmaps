package com.mapswithme.maps.auth;

import android.content.Intent;
import android.support.annotation.Nullable;

public interface TargetFragmentCallback
{
  void onTargetFragmentResult(int resultCode, @Nullable Intent data);
  boolean isTargetAdded();
}
