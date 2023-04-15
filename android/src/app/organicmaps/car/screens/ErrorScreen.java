package app.organicmaps.car.screens;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.LongMessageTemplate;
import androidx.car.app.model.Template;

import app.organicmaps.R;
import app.organicmaps.car.screens.base.BaseScreen;

public class ErrorScreen extends BaseScreen
{
  public ErrorScreen(@NonNull CarContext carContext)
  {
    super(carContext);
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    final LongMessageTemplate.Builder builder = new LongMessageTemplate.Builder(getCarContext().getString(R.string.dialog_error_storage_message));
    builder.setTitle(getCarContext().getString(R.string.dialog_error_storage_title));

    return builder.build();
  }
}
