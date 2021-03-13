package com.mapswithme.maps.intent;

import androidx.annotation.NonNull;

import com.mapswithme.maps.MwmActivity;

import java.io.Serializable;

public interface MapTask extends Serializable
{
  boolean run(@NonNull MwmActivity target);
}
