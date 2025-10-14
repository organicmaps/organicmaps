package app.organicmaps.editor;

import android.app.Activity;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import app.organicmaps.base.BaseMwmFragmentActivity;

public class EditorActivity extends BaseMwmFragmentActivity
{
  public static final String EXTRA_NEW_OBJECT = "ExtraNewMapObject";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return EditorHostFragment.class;
  }

  public static void start(@NonNull Activity activity)
  {
    final Intent intent = new Intent(activity, EditorActivity.class);
    activity.startActivity(intent);
  }
}
