package app.organicmaps.base;

import androidx.fragment.app.Fragment;

public class BaseMwmFragment extends Fragment implements OnBackPressListener
{
  @Override
  public boolean onBackPressed()
  {
    return false;
  }
}
