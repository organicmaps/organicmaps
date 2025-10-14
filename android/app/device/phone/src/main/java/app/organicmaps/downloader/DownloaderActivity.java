package app.organicmaps.downloader;

import androidx.fragment.app.Fragment;
import app.organicmaps.base.BaseMwmFragmentActivity;

public class DownloaderActivity extends BaseMwmFragmentActivity
{
  public static final String EXTRA_OPEN_DOWNLOADED = "open downloaded";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return DownloaderFragment.class;
  }
}
