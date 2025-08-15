package app.organicmaps.sdk.maplayer;

import android.content.Context;
import android.widget.Toast;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.R;
import app.organicmaps.sdk.maplayer.isolines.IsolinesManager;
import app.organicmaps.sdk.maplayer.subway.SubwayManager;
import app.organicmaps.sdk.maplayer.traffic.TrafficManager;

public enum Mode
{
  TRAFFIC {
    @Override
    public boolean isEnabled(@NonNull Context context)
    {
      return !SubwayManager.isEnabled() && TrafficManager.INSTANCE.isEnabled();
    }

    @Override
    public void setEnabled(@NonNull Context context, boolean isEnabled)
    {
      TrafficManager.INSTANCE.setEnabled(isEnabled);
    }
  },

  SUBWAY {
    @Override
    public boolean isEnabled(@NonNull Context context)
    {
      return SubwayManager.isEnabled();
    }

    @Override
    public void setEnabled(@NonNull Context context, boolean isEnabled)
    {
      SubwayManager.setEnabled(isEnabled);
    }
  },

  ISOLINES {
    @Override
    public boolean isEnabled(@NonNull Context context)
    {
      return IsolinesManager.isEnabled();
    }

    @Override
    public void setEnabled(@NonNull Context context, boolean isEnabled)
    {
      IsolinesManager.setEnabled(isEnabled);
    }
  },

  OUTDOORS {
    @Override
    public boolean isEnabled(@NonNull Context context)
    {
      return Framework.nativeIsOutdoorsLayerEnabled();
    }

    @Override
    public void setEnabled(@NonNull Context context, boolean isEnabled)
    {
      Framework.nativeSetOutdoorsLayerEnabled(isEnabled);
      // TODO: ThemeSwitcher is outside sdk package. Properly fix dependencies
      // ThemeSwitcher.INSTANCE.restart(true);
    }
  },

  HIKING {
    @Override
    public boolean isEnabled(@NonNull Context context)
    {
      return Framework.nativeIsHikingLayerEnabled();
    }

    @Override
    public void setEnabled(@NonNull Context context, boolean isEnabled)
    {
      Framework.nativeSetHikingLayerEnabled(isEnabled);
      if (isEnabled)
        showUpdateToastIfNeeded(context);
    }
  },

  CYCLING {
    @Override
    public boolean isEnabled(@NonNull Context context)
    {
      return Framework.nativeIsCyclingLayerEnabled();
    }

    @Override
    public void setEnabled(@NonNull Context context, boolean isEnabled)
    {
      Framework.nativeSetCyclingLayerEnabled(isEnabled);
      if (isEnabled)
        showUpdateToastIfNeeded(context);
    }
  };

  public abstract boolean isEnabled(@NonNull Context context);

  public abstract void setEnabled(@NonNull Context context, boolean isEnabled);

  public void showUpdateToastIfNeeded(@NonNull Context context)
  {
    if (Framework.nativeNeedUpdateForRoutes())
      Toast.makeText(context, R.string.routes_update_maps_text, Toast.LENGTH_SHORT).show();
  }
}
