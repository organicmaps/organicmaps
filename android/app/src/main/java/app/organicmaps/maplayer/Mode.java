package app.organicmaps.maplayer;

import android.content.Context;

import androidx.annotation.NonNull;

import app.organicmaps.Framework;
import app.organicmaps.maplayer.isolines.IsolinesManager;
import app.organicmaps.maplayer.subway.SubwayManager;
import app.organicmaps.maplayer.traffic.TrafficManager;
import app.organicmaps.util.ThemeSwitcher;

public enum Mode
{
  TRAFFIC
      {
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
  SUBWAY
      {
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

  ISOLINES
      {
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
  OUTDOORS
      {
        @Override
        public boolean isEnabled(@NonNull Context context)
        {
          return Framework.nativeIsOutdoorsLayerEnabled();
        }

        @Override
        public void setEnabled(@NonNull Context context, boolean isEnabled)
        {
          Framework.nativeSetOutdoorsLayerEnabled(isEnabled);
          ThemeSwitcher.INSTANCE.restart(true);
        }
      };
  
  public abstract boolean isEnabled(@NonNull Context context);

  public abstract void setEnabled(@NonNull Context context, boolean isEnabled);

}
