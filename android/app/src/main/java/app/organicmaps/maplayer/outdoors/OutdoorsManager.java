package app.organicmaps.maplayer.outdoors;

import android.app.Application;
import android.content.Context;

import androidx.annotation.NonNull;
import app.organicmaps.Framework;
import app.organicmaps.MwmApplication;
import app.organicmaps.util.ThemeSwitcher;
import app.organicmaps.util.ThemeUtils;

public class OutdoorsManager
{
  @NonNull
  private final OnOutdoorsChangedListener mListener;

  public OutdoorsManager(@NonNull Application application)
  {
    mListener = new OnOutdoorsChangedListener(application);
  }

  @NonNull
  public static OutdoorsManager from(@NonNull Context context)
  {
    MwmApplication app = (MwmApplication) context.getApplicationContext();
    return app.getOutdoorsManager();
  }

  public static boolean isEnabled()
  {
    @Framework.MapStyle
    int lightMapStyle = Framework.MAP_STYLE_OUTDOORS_CLEAR;
    int darkMapStyle = Framework.MAP_STYLE_OUTDOORS_DARK;
    return Framework.nativeGetMapStyle() == lightMapStyle || Framework.nativeGetMapStyle() == darkMapStyle;
  }

  public static void setEnabled(boolean isEnabled, Context context)
  {
    if (isEnabled == isEnabled())
      return;
    @Framework.MapStyle
    int newMapStyle;
    if (isEnabled)
    {
      if (ThemeUtils.isDefaultTheme(context))
      {
        newMapStyle = Framework.MAP_STYLE_OUTDOORS_CLEAR;
      }
      else
      {
        newMapStyle = Framework.MAP_STYLE_OUTDOORS_DARK;
      }
    }
    else
    {
      if (ThemeUtils.isDefaultTheme(context))
      {
        newMapStyle = Framework.MAP_STYLE_CLEAR;
      }
      else
      {
        newMapStyle = Framework.MAP_STYLE_DARK;
      }
    }
    ThemeSwitcher.SetMapStyle(newMapStyle);
  }
}
