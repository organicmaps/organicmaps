package com.mapswithme.maps.maplayer.guides;

import android.view.View;

import androidx.annotation.NonNull;
import com.mapswithme.maps.base.NoConnectionListener;
import com.mapswithme.maps.maplayer.BottomSheetItem;
import com.mapswithme.maps.maplayer.DefaultClickListener;
import com.mapswithme.maps.maplayer.LayersAdapter;
import com.mapswithme.maps.maplayer.Mode;
import com.mapswithme.util.ConnectionState;

public abstract class AbstractGuidesClickListener extends DefaultClickListener
{

  @NonNull
  private final NoConnectionListener mNoConnectionListener;

  public AbstractGuidesClickListener(@NonNull LayersAdapter adapter,
                                     @NonNull NoConnectionListener noConnectionListener)
  {
    super(adapter);
    mNoConnectionListener = noConnectionListener;
  }

  @Override
  public void onItemClick(@NonNull View v, @NonNull BottomSheetItem item)
  {
    Mode mode = item.getMode();
    if (!mode.isEnabled(v.getContext()) && !ConnectionState.INSTANCE.isConnected())
    {
      mNoConnectionListener.onNoConnectionError();
      return;
    }

    super.onItemClick(v, item);
  }
}
