package app.organicmaps.editor;

import androidx.fragment.app.Fragment;

import app.organicmaps.base.BaseMwmFragmentActivity;

public class ProfileActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return ProfileFragment.class;
  }
}
