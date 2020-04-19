package com.mapswithme.maps.widget.menu;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;

public class MenuControllerFactory
{
  @NonNull
  public static MenuController createMainMenuController(@Nullable MenuStateObserver stateObserver,
                                                        @NonNull MainMenuOptionListener listener)
  {
    return new BottomSheetMenuController(R.id.main_menu_sheet, new MainMenuRenderer(listener),
                                         stateObserver);
  }
}
