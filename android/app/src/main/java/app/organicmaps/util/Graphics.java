package app.organicmaps.util;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.widget.TextView;
import androidx.annotation.AttrRes;
import androidx.annotation.DrawableRes;
import androidx.appcompat.content.res.AppCompatResources;
import app.organicmaps.R;

public final class Graphics
{
  public static void tint(TextView view)
  {
    tint(view, R.attr.iconTint);
  }

  public static void tint(TextView view, @AttrRes int tintAttr)
  {
    final Drawable[] dlist = view.getCompoundDrawablesRelative();
    for (int i = 0; i < dlist.length; i++)
      dlist[i] = tint(view.getContext(), dlist[i], tintAttr);

    view.setCompoundDrawablesRelativeWithIntrinsicBounds(dlist[0], dlist[1], dlist[2], dlist[3]);
  }

  public static Drawable tint(Context context, @DrawableRes int resId)
  {
    return tint(context, resId, R.attr.iconTint);
  }

  public static Drawable tint(Context context, @DrawableRes int resId, @AttrRes int tintAttr)
  {
    return tint(context, AppCompatResources.getDrawable(context, resId), tintAttr);
  }

  public static Drawable tint(Context context, Drawable drawable)
  {
    return tint(context, drawable, R.attr.iconTint);
  }

  public static Drawable tint(Context context, Drawable drawable, @AttrRes int tintAttr)
  {
    return app.organicmaps.sdk.util.Graphics.tint(drawable, ThemeUtils.getColor(context, tintAttr));
  }

  private Graphics() {}
}
