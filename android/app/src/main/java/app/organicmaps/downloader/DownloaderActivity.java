package app.organicmaps.downloader;

import androidx.fragment.app.Fragment;
import app.organicmaps.base.BaseMwmFragmentActivity;
import app.organicmaps.base.OnBackPressListener;

public class DownloaderActivity extends BaseMwmFragmentActivity
{
  public static final String EXTRA_OPEN_DOWNLOADED = "open downloaded";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return DownloaderFragment.class;
  }

  @Override
  public void onBackPressed()
  {
    OnBackPressListener fragment =
        (OnBackPressListener) getSupportFragmentManager().findFragmentById(getFragmentContentResId());
    if (!fragment.onBackPressed())
      super.onBackPressed();
  }
}
