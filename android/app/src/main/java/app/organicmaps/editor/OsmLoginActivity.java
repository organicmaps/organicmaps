package app.organicmaps.editor;

import androidx.fragment.app.Fragment;

import app.organicmaps.base.BaseMwmFragmentActivity;

public class OsmLoginActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return OsmLoginFragment.class;
  }
}
