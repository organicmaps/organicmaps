package app.organicmaps.sdk.maplayer.subway;

import android.content.Context;
import androidx.annotation.Keep;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;

interface OnTransitSchemeChangedListener
{
  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  @MainThread
  void onTransitStateChanged(int type);

  class Default implements OnTransitSchemeChangedListener
  {
    @NonNull
    private final Context mContext;

    Default(@NonNull Context context)
    {
      mContext = context;
    }

    @Override
    public void onTransitStateChanged(int index)
    {
      TransitSchemeState state = TransitSchemeState.values()[index];
      state.activate(mContext);
    }
  }
}
