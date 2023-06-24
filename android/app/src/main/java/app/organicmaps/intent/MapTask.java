package app.organicmaps.intent;

import androidx.annotation.NonNull;

import app.organicmaps.MwmActivity;

import java.io.Serializable;

public interface MapTask extends Serializable
{
  boolean run(@NonNull MwmActivity target);
}
