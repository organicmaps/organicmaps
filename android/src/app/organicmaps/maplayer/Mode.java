package app.organicmaps.maplayer;

import android.content.Context;

import androidx.annotation.NonNull;

import app.organicmaps.maplayer.isolines.IsolinesManager;
import app.organicmaps.maplayer.subway.SubwayManager;
import app.organicmaps.maplayer.traffic.TrafficManager;
import app.organicmaps.util.SharedPropertiesUtils;

public enum Mode
{
  TRAFFIC
      {
        @Override
        public boolean isEnabled(@NonNull Context context)
        {
          return !SubwayManager.from(context).isEnabled()
                 && TrafficManager.INSTANCE.isEnabled();
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
          return SubwayManager.from(context).isEnabled();
        }

        @Override
        public void setEnabled(@NonNull Context context, boolean isEnabled)
        {
          SubwayManager.from(context).setEnabled(isEnabled);
        }
      },

  ISOLINES
      {
        @Override
        public boolean isEnabled(@NonNull Context context)
        {
          return IsolinesManager.from(context).isEnabled();
        }

        @Override
        public void setEnabled(@NonNull Context context, boolean isEnabled)
        {
          IsolinesManager.from(context).setEnabled(isEnabled);
        }
      };
  
  public abstract boolean isEnabled(@NonNull Context context);

  public abstract void setEnabled(@NonNull Context context, boolean isEnabled);

  public boolean isNew(@NonNull Context context)
  {
    return SharedPropertiesUtils.shouldShowNewMarkerForLayerMode(context, this);
  }
}
