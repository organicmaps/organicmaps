package app.organicmaps.downloader;

import android.content.Context;

import androidx.fragment.app.Fragment;

import app.organicmaps.base.BaseMwmFragmentActivity;
import app.organicmaps.base.OnBackPressListener;
import app.tourism.data.prefs.Language;
import app.tourism.data.prefs.UserPreferences;
import app.tourism.utils.LocaleHelper;

public class DownloaderActivity extends BaseMwmFragmentActivity
{
  public static final String EXTRA_OPEN_DOWNLOADED = "open downloaded";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return DownloaderFragment.class;
  }

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
  public void onBackPressed()
  {
    OnBackPressListener fragment = (OnBackPressListener)getSupportFragmentManager().findFragmentById(getFragmentContentResId());
    if (!fragment.onBackPressed())
      super.onBackPressed();
  }
}
