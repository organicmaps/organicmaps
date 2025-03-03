package app.organicmaps.base;

import android.app.Application;
import android.content.Context;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StyleRes;
import androidx.fragment.app.DialogFragment;

import app.organicmaps.R;
import app.organicmaps.util.ThemeUtils;

public class BaseMwmDialogFragment extends DialogFragment
{
  protected int getStyle()
  {
    return STYLE_NORMAL;
  }

  protected @StyleRes int getCustomTheme()
  {
    return 0;
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    int style = getStyle();
    int theme = getCustomTheme();
    if (style != STYLE_NORMAL || theme != 0)
      setStyle(style, theme);
  }

  @NonNull
  protected Application getAppContextOrThrow()
  {
    Context context = requireContext();
    if (context == null)
      throw new IllegalStateException("Before call this method make sure that the context exists");
    return (Application) context.getApplicationContext();
  }

}
