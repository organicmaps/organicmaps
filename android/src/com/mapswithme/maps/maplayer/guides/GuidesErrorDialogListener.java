package com.mapswithme.maps.maplayer.guides;

import androidx.annotation.NonNull;
import com.mapswithme.maps.maplayer.isolines.IsolinesState;

public interface GuidesErrorDialogListener
{
  void onStateChanged(@NonNull GuidesState state);
}
