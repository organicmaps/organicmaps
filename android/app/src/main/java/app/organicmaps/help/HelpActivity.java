package app.organicmaps.help;

import androidx.fragment.app.Fragment;
import app.organicmaps.base.BaseToolbarActivity;

public class HelpActivity extends BaseToolbarActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return HelpFragment.class;
  }
}
