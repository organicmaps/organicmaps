package com.mapswithme.util;

import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.drawable.Drawable;
import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;
import android.support.v4.content.ContextCompat;

import com.cocosw.bottomsheet.BottomSheet;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.widget.ListShadowController;

import java.lang.ref.WeakReference;

public final class BottomSheetHelper
{
  public static class ShadowedBottomSheet extends BottomSheet
  {
    private ListShadowController mShadowController;


    ShadowedBottomSheet(Context context, int theme)
    {
      super(context, theme);
    }

    @Override
    protected void init(Context context)
    {
      super.init(context);
      mShadowController = new ListShadowController(list);
    }

    @Override
    protected void showFullItems()
    {
      super.showFullItems();
      mShadowController.attach();
    }

    @Override
    protected void showShortItems()
    {
      super.showShortItems();
      mShadowController.detach();
    }
  }


  public static class Builder extends BottomSheet.Builder
  {
    public Builder(@NonNull Activity context)
    {
      super(context);
      setOnDismissListener(null);
    }

    @Override
    protected BottomSheet createDialog(Context context, int theme)
    {
      return new ShadowedBottomSheet(context, theme);
    }

    @Override
    public BottomSheet build()
    {
      free();

      BottomSheet res = super.build();
      sRef = new WeakReference<>(res);
      return res;
    }

    @SuppressWarnings("NullableProblems")
    @Override
    public BottomSheet.Builder setOnDismissListener(final DialogInterface.OnDismissListener listener)
    {
      return super.setOnDismissListener(new DialogInterface.OnDismissListener() {
        @Override
        public void onDismiss(DialogInterface dialog)
        {
          free();
          if (listener != null)
            listener.onDismiss(dialog);
        }
      });
    }
  }


  private static WeakReference<BottomSheet> sRef;


  private BottomSheetHelper()
  {}

  public static BottomSheet getReference()
  {
    if (sRef == null)
      return null;

    return sRef.get();
  }

  public static boolean isShowing()
  {
    BottomSheet bs = getReference();
    return (bs != null && bs.isShowing());
  }

  public static void free()
  {
    BottomSheet ref = getReference();
    if (ref != null)
    {
      if (ref.isShowing())
        ref.dismiss();
      sRef = null;
    }
  }

  public static BottomSheet.Builder create(Activity context)
  {
    free();
    return new Builder(context);
  }

  public static BottomSheet.Builder create(Activity context, @StringRes int title)
  {
    return create(context).title(title);
  }

  public static BottomSheet.Builder createGrid(Activity context, @StringRes int title)
  {
    return create(context, title).grid();
  }

  public static BottomSheet.Builder sheet(BottomSheet.Builder builder, int id, @DrawableRes int iconRes, CharSequence text)
  {
    Drawable icon = ContextCompat.getDrawable(MWMApplication.get(), iconRes);
    return builder.sheet(id, icon, text);
  }
}
