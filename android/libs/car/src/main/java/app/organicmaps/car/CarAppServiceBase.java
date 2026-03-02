package app.organicmaps.car;

import androidx.car.app.CarAppService;
import app.organicmaps.api.Const;

public abstract class CarAppServiceBase extends CarAppService
{
  public static final String API_CAR_HOST = Const.AUTHORITY + ".car";
  public static final String ACTION_SHOW_NAVIGATION_SCREEN = Const.ACTION_PREFIX + ".SHOW_NAVIGATION_SCREEN";
}
