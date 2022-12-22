package app.organicmaps.car.screens.hacks;

import static java.util.Objects.requireNonNull;

import android.os.Handler;
import android.os.Looper;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.RoutePreviewNavigationTemplate;
import androidx.lifecycle.LifecycleOwner;

import app.organicmaps.car.screens.base.BaseScreen;

/**
 * This class is used to provide a working solution for next the two actions:
 * <ul>
 *   <li> {@code popToRoot();}
 *   <li> {@code push(new Screen());}
 * </ul>
 * <p>
 * When you run the following code:
 * <pre>
 * {@code
 *   ScreenManager.popToRoot();
 *   ScreenManager.push(new Screen());
 * }
 * </pre>
 * the first {@code popToRoot} action won't be applied and a new {@link androidx.car.app.Screen} will be pushed to the top of the stack.
 * <p>
 * Actually, the {@code popToRoot} action <b>will</b> be applied and the screen stack will be cleared.
 * It will contain only two screens: the root and the newly pushed.
 * But the steps counter won't be reset after {@code popToRoot}.
 * It will be increased by one as if we just push a new screen without {@code popToRoot} action.
 * <p>
 * To decrease a step counter, it is required to display a previous screen.
 * For example we're on step 4 and we're making a {@code popToRoot} action.
 * The step counter will be decreased to 1 after {@code RootScreen.onStart()/onResume()} call.
 * <p>
 * How it works?
 * <p>
 * In {@link #onStart(LifecycleOwner)} we call {@code getScreenManager().popToRoot();}
 * <p>
 * The screen ({@link PopToRootHack} class) should begin to destroy - all the lifecycle methods should be called step by step.
 * <p>
 * In {@link #onStop(LifecycleOwner)} we post {@code getScreenManager().push(mScreenToPush);} to the main thread.
 * This mean when the main thread will be free, it will execute our code - it will push a new screen.
 * This will happen when {@link PopToRootHack} will be destroyed and the root screen will be displayed.
 */
public final class PopToRootHack extends BaseScreen
{
  private static final Handler mHandler = new Handler(Looper.getMainLooper());
  private static final RoutePreviewNavigationTemplate mTemplate = new RoutePreviewNavigationTemplate.Builder().setLoading(true).build();

  @NonNull
  private final BaseScreen mScreenToPush;

  private PopToRootHack(@NonNull Builder builder)
  {
    super(builder.mCarContext);
    mScreenToPush = requireNonNull(builder.mScreenToPush);
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    getScreenManager().popToRoot();
    return mTemplate;
  }

  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    getScreenManager().popToRoot();
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    mHandler.post(() -> getScreenManager().push(mScreenToPush));
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
