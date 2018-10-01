package com.mapswithme.util;

import android.app.Activity;
import android.content.DialogInterface;
import android.graphics.drawable.Drawable;
import android.support.annotation.DrawableRes;
import android.support.annotation.IdRes;
import android.support.annotation.MenuRes;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;
import android.support.v4.content.ContextCompat;
import android.view.MenuItem;

import com.cocosw.bottomsheet.BottomSheet;
import com.mapswithme.maps.MwmApplication;

public final class BottomSheetHelper
{
  public static class Builder extends BottomSheet.Builder
  {
    public Builder(@NonNull Activity context)
    {
      super(context);
      setOnDismissListener(null);
      if (ThemeUtils.isNightTheme())
        darkTheme();
    }

    @Override
    public BottomSheet build()
    {
      BottomSheet res = super.build();
      return res;
    }

    @SuppressWarnings("NullableProblems")
    @Override
    public Builder setOnDismissListener(final DialogInterface.OnDismissListener listener)
    {
      super.setOnDismissListener(new DialogInterface.OnDismissListener()
      {
        @Override
        public void onDismiss(DialogInterface dialog)
        {
          if (listener != null)
            listener.onDismiss(dialog);
        }
      });

      return this;
    }

    @Override
    public Builder title(CharSequence title)
    {
      super.title(title);
      return this;
    }

    @Override
    public Builder title(@StringRes int title)
    {
      super.title(title);
      return this;
    }

    @Override
    public Builder sheet(@MenuRes int xmlRes)
    {
      super.sheet(xmlRes);
      return this;
    }

    @Override
    public Builder sheet(int id, @NonNull Drawable icon, @NonNull CharSequence text)
    {
      super.sheet(id, icon, text);
      return this;
    }

    @Override
    public Builder sheet(int id, @DrawableRes int iconRes, @StringRes int textRes)
    {
      super.sheet(id, iconRes, textRes);
      return this;
    }

    @Override
    public Builder grid()
    {
      super.grid();
      return this;
    }

    @Override
    public Builder listener(@NonNull MenuItem.OnMenuItemClickListener listener)
    {
      super.listener(listener);
      return this;
    }
  }

  public static void tint(@NonNull BottomSheet bottomSheet)
  {
    for (int i = 0; i < bottomSheet.getMenu().size(); i++)
    {
      MenuItem mi = bottomSheet.getMenu().getItem(i);
      Drawable icon = mi.getIcon();
      if (icon != null)
        mi.setIcon(Graphics.tint(bottomSheet.getContext(), icon));
    }
  }

  @NonNull
  public static MenuItem findItemById(@NonNull BottomSheet bottomSheet, @IdRes int id)
  {
    MenuItem item = bottomSheet.getMenu().findItem(id);

    if (item == null)
      throw new AssertionError("Can not find bottom sheet item with id: " + id);

    return item;
  }

  private BottomSheetHelper()
  {}

  public static Builder create(Activity context)
  {
    return new Builder(context);
  }

  public static Builder create(Activity context, @StringRes int title)
  {
    return create(context).title(title);
  }

  public static Builder create(Activity context, CharSequence title)
  {
    return create(context).title(title);
  }

  public static Builder createGrid(Activity context, @StringRes int title)
  {
    return create(context, title).grid();
  }

  public static Builder sheet(Builder builder, int id, @DrawableRes int iconRes, CharSequence text)
  {
    Drawable icon = ContextCompat.getDrawable(MwmApplication.get(), iconRes);
    return builder.sheet(id, icon, text);
  }
}
