package app.organicmaps.display;

import androidx.annotation.NonNull;

public interface DisplayChangedListener
{
  void onDisplayChanged(@NonNull final DisplayType newDisplayType);
}
