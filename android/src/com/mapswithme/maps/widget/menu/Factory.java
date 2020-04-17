package com.mapswithme.maps.widget.menu;

import android.app.Activity;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;

public class Factory
{
  @NonNull
  public static MenuController createMainMenuController(@Nullable MenuStateObserver stateObserver)
  {
    return new BottomSheetMenuController(R.id.main_menu_sheet, new MainMenuRenderer(),
                                         stateObserver);
  }
}
