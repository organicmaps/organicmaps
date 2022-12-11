package app.organicmaps.car;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.car.app.CarContext;
import androidx.car.app.ScreenManager;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Row;
import androidx.core.graphics.drawable.IconCompat;

import app.organicmaps.R;
import app.organicmaps.routing.RoutingOptions;
import app.organicmaps.settings.RoadType;

public final class UiHelpers
{
  public interface PrefsGetter
  {
    boolean get();
  }

  public interface PrefsSetter
  {
    void set(boolean newValue);
  }

  @Nullable
  private static CarIcon mCheckboxIcon;
  @Nullable
  private static CarIcon mCheckboxSelectedIcon;

  @NonNull
  public static Row createSharedPrefsCheckbox(
      @NonNull CarContext context, @StringRes int titleRes, PrefsGetter getter, PrefsSetter setter)
  {
    final boolean getterValue = getter.get();

    if (mCheckboxIcon == null || mCheckboxSelectedIcon == null)
      initCheckboxIcons(context);
    Row.Builder builder = new Row.Builder();
    builder.setTitle(context.getString(titleRes));
    builder.setOnClickListener(() -> {
      setter.set(!getterValue);
      context.getCarService(ScreenManager.class).getTop().invalidate();
    });
    builder.setImage(getterValue ? mCheckboxSelectedIcon : mCheckboxIcon);
    return builder.build();
  }

  @NonNull
  public static Row createDrivingOptionCheckbox(
      @NonNull CarContext context, RoadType roadType, @StringRes int titleRes)
  {
    if (mCheckboxIcon == null || mCheckboxSelectedIcon == null)
      initCheckboxIcons(context);
    Row.Builder builder = new Row.Builder();
    builder.setTitle(context.getString(titleRes));
    builder.setOnClickListener(() -> {
      if (RoutingOptions.hasOption(roadType))
        RoutingOptions.removeOption(roadType);
      else
        RoutingOptions.addOption(roadType);
      context.getCarService(ScreenManager.class).getTop().invalidate();
    });
    builder.setImage(RoutingOptions.hasOption(roadType) ? mCheckboxSelectedIcon : mCheckboxIcon);
    return builder.build();
  }

  private static void initCheckboxIcons(@NonNull CarContext context)
  {
    mCheckboxIcon = new CarIcon.Builder(IconCompat.createWithResource(context, R.drawable.ic_check_box)).build();
    mCheckboxSelectedIcon = new CarIcon.Builder(IconCompat.createWithResource(context, R.drawable.ic_check_box_checked)).build();
  }
}
