package com.mapswithme.maps.widget.menu;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.NoConnectionListener;
import com.mapswithme.maps.search.FilterUtils;

public class MenuControllerFactory
{
  @NonNull
  public static MenuController createMainMenuController(@Nullable MenuStateObserver stateObserver,
                                                        @NonNull MainMenuOptionListener listener,
                                                        @NonNull NoConnectionListener noConnectionListener)
  {
    return new BottomSheetMenuController(R.id.main_menu_sheet, new MainMenuRenderer(listener,
                                                                                    noConnectionListener),
                                         stateObserver);
  }

  @NonNull
  public static MenuController createGuestsRoomsMenuController(
      @NonNull MenuRoomsGuestsListener listener, @NonNull MenuStateObserver stateObserver,
      @NonNull FilterUtils.RoomsGuestsCountProvider provider)
  {
    return new BottomSheetMenuController(R.id.guests_and_rooms_menu_sheet,
                                         new GuestsRoomsMenuRenderer(listener, provider),
                                         stateObserver);
  }
}
