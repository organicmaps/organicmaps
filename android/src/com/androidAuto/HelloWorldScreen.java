package com.androidAuto;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.Screen;
import androidx.car.app.model.Action;
import androidx.car.app.model.Pane;
import androidx.car.app.model.PaneTemplate;
import androidx.car.app.model.Row;
import androidx.car.app.model.Template;

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
    Row row = new Row.Builder().setTitle("Hello AA Organic Maps!").build();
    return new PaneTemplate.Builder(new Pane.Builder().addRow(row).build())
        .setHeaderAction(Action.APP_ICON)
        .build();
  }
}