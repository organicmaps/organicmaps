package app.organicmaps.sdk.widgets.speedlimit;

import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.Objects;

public class SpeedLimitViewManager
{
  @Nullable
  private SpeedLimitView mSpeedLimitView;

  private int mSpeedLimit;
  private int mCurrentSpeed;
  private boolean mAlert;
  private boolean mVisible;

  public SpeedLimitViewManager()
  {
    mSpeedLimit = 0;
    mCurrentSpeed = 0;
    mAlert = false;
    mVisible = false;
  }

  public void setSpeedLimitView(@NonNull SpeedLimitView speedLimitView)
  {
    mSpeedLimitView = Objects.requireNonNull(speedLimitView);
    updateState();
  }

  public void setSpeed(int speedLimit, int currentSpeed)
  {
    mSpeedLimit = speedLimit;
    mCurrentSpeed = currentSpeed;
    mAlert = mCurrentSpeed > mSpeedLimit;
    mVisible = mSpeedLimit > 0;
    updateState();
  }

  private void updateState()
  {
    if (mSpeedLimitView == null)
      return;
    mSpeedLimitView.setValue(mSpeedLimit);
    mSpeedLimitView.setMode(mAlert ? SpeedLimitView.Mode.Alert : SpeedLimitView.Mode.Normal);
    mSpeedLimitView.setVisibility(mVisible ? View.VISIBLE : View.INVISIBLE);
  }
}
