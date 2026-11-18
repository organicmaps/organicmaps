package app.organicmaps;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.Lifecycle;
import app.organicmaps.base.BaseMwmFragmentActivity;
import app.organicmaps.sdk.MapsReadiness;
import app.organicmaps.sdk.display.DisplayChangedListener;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.display.DisplayType;

public class MapPlaceholderActivity extends BaseMwmFragmentActivity implements DisplayChangedListener
{
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private DisplayManager mDisplayManager;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private MapsReadiness mMapsReadiness;
  private boolean mRemoveDisplayListener = true;
  @NonNull
  private final Runnable mOpenMapWhenReady = this::openMap;

  @Override
  protected boolean requiresMapsReady()
  {
    return false;
  }

  @Override
  protected void onSafeCreate(@Nullable Bundle savedInstanceState)
  {
    super.onSafeCreate(savedInstanceState);
    setContentView(R.layout.activity_map_placeholder);

    final MwmApplication app = MwmApplication.from(this);
    mMapsReadiness = app.getMapsReadiness();
    mDisplayManager = app.getDisplayManager();
    mDisplayManager.addListener(DisplayType.Device, this);

    findViewById(R.id.btn_continue).setOnClickListener(view -> {
      mDisplayManager.changeDisplay(DisplayType.Device);
      updateHandoffUi();
    });
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    updateHandoffUi();
    waitForMapsIfPhoneSelected();
  }

  @Override
  protected void onPause()
  {
    mMapsReadiness.cancel(mOpenMapWhenReady);
    super.onPause();
  }

  @Override
  public void onDisplayChangedToDevice(@NonNull Runnable onTaskFinishedCallback)
  {
    // Keep the phone listener registered until the map activity can take over.
    if (getLifecycle().getCurrentState().isAtLeast(Lifecycle.State.RESUMED))
    {
      updateHandoffUi();
      waitForMapsIfPhoneSelected();
    }
    onTaskFinishedCallback.run();
  }

  @Override
  public void onDisplayChangedToCar(@NonNull Runnable onTaskFinishedCallback)
  {
    if (mDisplayManager.isCarDisplayUsed())
    {
      mMapsReadiness.cancel(mOpenMapWhenReady);
      updateHandoffUi();
    }
    onTaskFinishedCallback.run();
  }

  private void updateHandoffUi()
  {
    final boolean canContinue = !mDisplayManager.isDeviceDisplayUsed();
    final View continueButton = findViewById(R.id.btn_continue);
    if (continueButton.isEnabled() == canContinue)
      return;

    continueButton.setEnabled(canContinue);
    final TextView status = findViewById(R.id.map_placeholder_status);
    status.setText(canContinue ? R.string.car_used_on_the_car_screen : R.string.please_wait);
  }

  private void waitForMapsIfPhoneSelected()
  {
    if (mDisplayManager.isDeviceDisplayUsed())
      mMapsReadiness.runWhenReady(mOpenMapWhenReady);
  }

  private void openMap()
  {
    // Display ownership changes before the handoff callback reaches this activity.
    if (!mDisplayManager.isDeviceDisplayUsed() || isFinishing())
      return;

    mRemoveDisplayListener = false;
    startActivity(new Intent(this, MwmActivity.class).putExtra(MwmActivity.EXTRA_UPDATE_THEME, true));
    finish();
  }

  @Override
  protected void onSafeDestroy()
  {
    super.onSafeDestroy();
    // Removing the listener would return the display to the car; the recreated screen replaces it instead.
    if (mRemoveDisplayListener && !isChangingConfigurations())
      mDisplayManager.removeListener(DisplayType.Device);
  }
}
