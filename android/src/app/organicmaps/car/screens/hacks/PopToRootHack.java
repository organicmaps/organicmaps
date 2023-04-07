package app.organicmaps.car.screens.hacks;

import android.os.Handler;
import android.os.Looper;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.RoutePreviewNavigationTemplate;
import androidx.lifecycle.LifecycleOwner;

import app.organicmaps.car.screens.base.BaseScreen;

public final class PopToRootHack extends BaseScreen
{
  private static final Handler mHandler = new Handler(Looper.getMainLooper());
  private static final RoutePreviewNavigationTemplate mTemplate = new RoutePreviewNavigationTemplate.Builder().setLoading(true).build();

  private final BaseScreen mScreenToPush;

  private PopToRootHack(@NonNull Builder builder)
  {
    super(builder.mCarContext);
    mScreenToPush = builder.mScreenToPush;
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    return mTemplate;
  }

  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    mHandler.post(() -> {
      getScreenManager().popToRoot();
      mHandler.post(() -> getScreenManager().push(mScreenToPush));
    });
  }

  /**
   * A builder of {@link PopToRootHack}.
   */
  public static final class Builder
  {
    @NonNull
    private final CarContext mCarContext;
    @Nullable
    private BaseScreen mScreenToPush;

    public Builder(@NonNull final CarContext carContext)
    {
      mCarContext = carContext;
    }

    @NonNull
    public Builder setScreenToPush(@NonNull BaseScreen screenToPush)
    {
      mScreenToPush = screenToPush;
      return this;
    }

    @NonNull
    public PopToRootHack build()
    {
      if (mScreenToPush == null)
        throw new IllegalStateException("You must specify Screen that will be pushed to the ScreenManager after the popToRoot() action");
      return new PopToRootHack(this);
    }
  }
}
