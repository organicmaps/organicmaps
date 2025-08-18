package app.organicmaps.settings;

import android.graphics.Rect;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.LayoutRes;
import androidx.annotation.Nullable;
import app.organicmaps.base.BaseMwmFragment;

abstract class BaseSettingsFragment extends BaseMwmFragment
{
  private View mFrame;

  private final Rect mSavedPaddings = new Rect();

  protected abstract @LayoutRes int getLayoutRes();

  private void savePaddings()
  {
    View parent = (View) mFrame.getParent();
    if (parent != null)
    {
      mSavedPaddings.set(parent.getPaddingStart(), parent.getPaddingTop(), parent.getPaddingEnd(),
                         parent.getPaddingBottom());
    }
  }

  protected void restorePaddings()
  {
    View parent = (View) mFrame.getParent();
    if (parent != null)
    {
      parent.setPaddingRelative(mSavedPaddings.left, mSavedPaddings.top, mSavedPaddings.right, mSavedPaddings.bottom);
    }
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return mFrame = inflater.inflate(getLayoutRes(), container, false);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    savePaddings();
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();

    restorePaddings();
  }
}
