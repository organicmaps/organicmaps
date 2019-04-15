package com.mapswithme.maps.intent;

import android.support.annotation.NonNull;

import com.mapswithme.maps.MwmActivity;

import java.io.Serializable;

public interface MapTask extends Serializable
{
  boolean run(@NonNull MwmActivity target);
}
