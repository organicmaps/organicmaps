package app.organicmaps.car.screens;

import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.MessageTemplate;
import androidx.car.app.model.ParkedOnlyOnClickListener;
import androidx.car.app.model.Template;

import app.organicmaps.R;
import app.organicmaps.car.screens.base.BaseScreen;
import app.organicmaps.car.util.Colors;

public class ErrorScreen extends BaseScreen
{
  @StringRes
  private final int mErrorMessage;

  private final boolean mIsCloseable;

  private ErrorScreen(@NonNull Builder builder)
  {
    super(builder.mCarContext);
    mErrorMessage = builder.mErrorMessage;
    mIsCloseable = builder.mIsCloseable;
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    final MessageTemplate.Builder builder = new MessageTemplate.Builder(getCarContext().getString(mErrorMessage));
    builder.setHeaderAction(Action.APP_ICON);
    builder.setTitle(getCarContext().getString(R.string.app_name));
    if (mIsCloseable)
    {
      builder.addAction(new Action.Builder()
          .setBackgroundColor(Colors.BUTTON_ACCEPT)
          .setTitle(getCarContext().getString(R.string.close))
          .setOnClickListener(ParkedOnlyOnClickListener.create(this::finish)).build()
      );
    }

    return builder.build();
  }

  public static class Builder
  {
    @NonNull
    private final CarContext mCarContext;

    @StringRes
    private int mErrorMessage;

    private boolean mIsCloseable;

    public Builder(@NonNull CarContext carContext)
    {
      mCarContext = carContext;
    }

    public Builder setErrorMessage(@StringRes int errorMessage)
    {
      mErrorMessage = errorMessage;
      return this;
    }

    public Builder setCloseable(boolean isCloseable)
    {
      mIsCloseable = isCloseable;
      return this;
    }

    public ErrorScreen build()
    {
      return new ErrorScreen(this);
    }
  }
}
