package app.organicmaps.car.screens;

import android.util.Log;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.Screen;
import androidx.car.app.model.LongMessageTemplate;
import androidx.car.app.model.Template;
import app.organicmaps.R;

public class PermissionDeniedScreen extends Screen
{

  public PermissionDeniedScreen(@NonNull CarContext carContext) { super(carContext); }

  private final String TAG = this.getClass().getSimpleName();

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    Log.d(TAG, "onGetTemplate");
    LongMessageTemplate.Builder builder = new LongMessageTemplate.Builder(getCarContext().getString(R.string.aa_location_permission_denied));
    builder.setTitle(getCarContext().getString(R.string.aa_location_permission_denied_title));

    return builder.build();
  }
}
