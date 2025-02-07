package app.organicmaps;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.base.BaseMwmFragmentActivity;
import app.organicmaps.display.DisplayChangedListener;
import app.organicmaps.display.DisplayManager;
import app.organicmaps.display.DisplayType;
import app.tourism.data.prefs.Language;
import app.tourism.data.prefs.UserPreferences;
import app.tourism.utils.LocaleHelper;

public class MapPlaceholderActivity extends BaseMwmFragmentActivity implements DisplayChangedListener
{
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private DisplayManager mDisplayManager;
  private boolean mRemoveDisplayListener = true;

  @Override
  protected void attachBaseContext(Context newBase) {
      Language language = new UserPreferences(newBase).getLanguage();
      if(language != null) {
        String languageCode = language.getCode();
        super.attachBaseContext(LocaleHelper.localeUpdateResources(newBase, languageCode));
      } else {
        super.attachBaseContext(newBase);
      }
  }

  @Override
  protected void onSafeCreate(@Nullable Bundle savedInstanceState)
  {
    super.onSafeCreate(savedInstanceState);
    setContentView(R.layout.activity_map_placeholder);

    mDisplayManager = DisplayManager.from(this);
    mDisplayManager.addListener(DisplayType.Device, this);

    findViewById(R.id.btn_continue).setOnClickListener((unused) -> mDisplayManager.changeDisplay(DisplayType.Device));
  }

  @Override
  public void onDisplayChangedToDevice(@NonNull Runnable onTaskFinishedCallback)
  {
    mRemoveDisplayListener = false;
    startActivity(new Intent(this, MwmActivity.class)
        .putExtra(MwmActivity.EXTRA_UPDATE_THEME, true));
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
