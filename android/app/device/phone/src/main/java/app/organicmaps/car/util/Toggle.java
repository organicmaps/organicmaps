package app.organicmaps.car.util;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.car.app.CarContext;
import androidx.car.app.annotations.RequiresCarApi;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.OnClickListener;
import androidx.car.app.model.Row;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.R;

public final class Toggle
{
  @DrawableRes
  private static final int CHECKBOX_ICON = R.drawable.ic_checkbox;
  @DrawableRes
  private static final int CHECKBOX_CHECKED_ICON = R.drawable.ic_checkbox_checked;

  @NonNull
  public static Row create(@NonNull final CarContext context, @StringRes int title,
                           @NonNull final OnClickListener onClickListener, boolean checked)
  {
    final Row.Builder row = new Row.Builder();
    row.setTitle(context.getString(title));
    if (context.getCarAppApiLevel() >= 6)
      row.setToggle(createToggle(onClickListener, checked));
    else
      createCheckbox(row, context, onClickListener, checked);

    return row.build();
  }

  @RequiresCarApi(6)
  @NonNull
  private static androidx.car.app.model.Toggle createToggle(@NonNull final OnClickListener onClickListener,
                                                            boolean checked)
  {
    return new androidx.car.app.model.Toggle.Builder((unused) -> onClickListener.onClick()).setChecked(checked).build();
  }

  private static void createCheckbox(@NonNull final Row.Builder row, @NonNull final CarContext context,
                                     @NonNull final OnClickListener onClickListener, boolean checked)
  {
    row.setOnClickListener(onClickListener);
    row.setImage(
        new CarIcon.Builder(IconCompat.createWithResource(context, checked ? CHECKBOX_CHECKED_ICON : CHECKBOX_ICON))
            .build());
  }

  private Toggle() {}
}
