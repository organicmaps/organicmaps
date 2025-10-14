package app.organicmaps;

import android.content.Intent;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.base.BaseMwmFragmentActivity;
import app.organicmaps.sdk.display.DisplayChangedListener;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.display.DisplayType;

public class MapPlaceholderActivity extends BaseMwmFragmentActivity implements DisplayChangedListener
{
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private DisplayManager mDisplayManager;
  private boolean mRemoveDisplayListener = true;

  @Override
  protected void onSafeCreate(@Nullable Bundle savedInstanceState)
  {
    super.onSafeCreate(savedInstanceState);
    setContentView(R.layout.activity_map_placeholder);

    mDisplayManager = MwmApplication.from(this).getDisplayManager();
    mDisplayManager.addListener(DisplayType.Device, this);

    findViewById(R.id.btn_continue).setOnClickListener((unused) -> mDisplayManager.changeDisplay(DisplayType.Device));
  }

  @Override
  public void onDisplayChangedToDevice(@NonNull Runnable onTaskFinishedCallback)
  {
    mRemoveDisplayListener = false;
    startActivity(new Intent(this, MwmActivity.class).putExtra(MwmActivity.EXTRA_UPDATE_THEME, true));
    finish();
    onTaskFinishedCallback.run();
  }

  @Override
  protected void onSafeDestroy()
  {
    super.onSafeDestroy();
    if (mRemoveDisplayListener)
      mDisplayManager.removeListener(DisplayType.Device);
  }
}
