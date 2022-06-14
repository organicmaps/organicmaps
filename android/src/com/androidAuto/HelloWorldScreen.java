package com.androidAuto;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.Screen;
import androidx.car.app.model.Action;
import androidx.car.app.model.ActionStrip;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.NavigationManager;
import androidx.car.app.navigation.model.NavigationTemplate;

import static androidx.car.app.model.Action.BACK;

public class HelloWorldScreen extends Screen
{
  public HelloWorldScreen(@NonNull CarContext carContext)
  {
    super(carContext);
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    NavigationTemplate.Builder builder = new NavigationTemplate.Builder();
    Action back = BACK;

    ActionStrip.Builder actionStripBuilder = new ActionStrip.Builder();
    actionStripBuilder.addAction(back).addAction(new Action.Builder().setTitle("Test").build());
    builder.setActionStrip(actionStripBuilder.build());
    NavigationManager navigationManager = getCarContext().getCarService(NavigationManager.class);
    return builder.build();
  }
}