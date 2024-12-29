package app.organicmaps.car.screens;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.Header;
import androidx.car.app.model.MessageTemplate;
import androidx.car.app.model.Template;

import app.organicmaps.R;
import app.organicmaps.car.screens.base.BaseScreen;
import app.organicmaps.car.util.Colors;
import app.organicmaps.util.concurrency.ThreadPool;
import app.organicmaps.util.concurrency.UiThread;

import java.util.concurrent.ExecutorService;

public class ErrorScreen extends BaseScreen
{
  public interface FinishPrecondition
  {
    boolean canFinish();
  }

  @StringRes
  private final int mTitle;
  @StringRes
  private final int mErrorMessage;

  @StringRes
  private final int mPositiveButtonText;
  @Nullable
  private final Runnable mPositiveButtonCallback;

  @StringRes
  private final int mNegativeButtonText;
  @Nullable
  private final Runnable mNegativeButtonCallback;

  @Nullable
  private final FinishPrecondition mFinishPrecondition;
  @Nullable
  private ExecutorService mExecutorService;

  private ErrorScreen(@NonNull Builder builder)
  {
    super(builder.mCarContext);
    mTitle = builder.mTitle == -1 ? R.string.app_name : builder.mTitle;
    mErrorMessage = builder.mErrorMessage;
    mPositiveButtonText = builder.mPositiveButtonText;
    mPositiveButtonCallback = builder.mPositiveButtonCallback;
    mNegativeButtonText = builder.mNegativeButtonText;
    mNegativeButtonCallback = builder.mNegativeButtonCallback;
    mFinishPrecondition = builder.mFinishPrecondition;
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    final MessageTemplate.Builder builder = new MessageTemplate.Builder(getCarContext().getString(mErrorMessage));

    final Header.Builder headerBuilder = new Header.Builder();
    headerBuilder.setStartHeaderAction(Action.APP_ICON);
    headerBuilder.setTitle(getCarContext().getString(mTitle));

    builder.setHeader(headerBuilder.build());
    if (mPositiveButtonText != -1)
    {
      builder.addAction(new Action.Builder()
          .setBackgroundColor(Colors.BUTTON_ACCEPT)
          .setTitle(getCarContext().getString(mPositiveButtonText))
          .setOnClickListener(this::onPositiveButton).build()
      );
    }
    if (mNegativeButtonText != -1)
    {
      builder.addAction(new Action.Builder()
          .setTitle(getCarContext().getString(mNegativeButtonText))
          .setOnClickListener(this::onNegativeButton).build()
      );
    }
    if (mFinishPrecondition != null && mExecutorService == null)
    {
      mExecutorService = ThreadPool.getWorker();
      mExecutorService.execute(this::checkFinishPrecondition);
    }
    return builder.build();
  }

  private void onPositiveButton()
  {
    if (mPositiveButtonCallback != null)
      mPositiveButtonCallback.run();
    finish();
  }

  private void onNegativeButton()
  {
    if (mNegativeButtonCallback != null)
      mNegativeButtonCallback.run();
    finish();
  }

  private void checkFinishPrecondition()
  {
    assert mFinishPrecondition != null;
    assert mExecutorService != null;

    if (mFinishPrecondition.canFinish())
      UiThread.runLater(this::finish);
    else
      mExecutorService.execute(this::checkFinishPrecondition);
  }

  public static class Builder
  {
    @NonNull
    private final CarContext mCarContext;

    @StringRes
    private int mTitle = -1;
    @StringRes
    private int mErrorMessage;

    @StringRes
    private int mPositiveButtonText = -1;
    @Nullable
    private Runnable mPositiveButtonCallback;

    @StringRes
    private int mNegativeButtonText = -1;
    @Nullable
    private Runnable mNegativeButtonCallback;

    @Nullable
    private FinishPrecondition mFinishPrecondition;

    public Builder(@NonNull CarContext carContext)
    {
      mCarContext = carContext;
    }

    public Builder setTitle(@StringRes int title)
    {
      mTitle = title;
      return this;
    }

    public Builder setErrorMessage(@StringRes int errorMessage)
    {
      mErrorMessage = errorMessage;
      return this;
    }

    public Builder setPositiveButton(@StringRes int text, @Nullable Runnable callback)
    {
      mPositiveButtonText = text;
      mPositiveButtonCallback = callback;
      return this;
    }

    public Builder setNegativeButton(@StringRes int text, @Nullable Runnable callback)
    {
      mNegativeButtonText = text;
      mNegativeButtonCallback = callback;
      return this;
    }

    public Builder setFinishPrecondition(@NonNull FinishPrecondition precondition)
    {
      mFinishPrecondition = precondition;
      return this;
    }

    public ErrorScreen build()
    {
      if (mFinishPrecondition != null && (mPositiveButtonCallback != null || mNegativeButtonCallback != null))
        throw new IllegalStateException("Finish precondition is not compatible with positive or negative button callbacks");
      return new ErrorScreen(this);
    }
  }
}
