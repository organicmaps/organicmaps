package app.organicmaps.widget.menu;

import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.Drawable;
import android.util.SparseArray;
import android.view.View;

import androidx.annotation.AttrRes;
import androidx.annotation.DimenRes;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.core.content.res.ResourcesCompat;
import androidx.core.widget.ImageViewCompat;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import app.organicmaps.R;
import app.organicmaps.location.LocationState;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.UiUtils;

public class MyPositionButton
{
  private static final int FOLLOW_SHIFT = 1;

  @NonNull
  private final FloatingActionButton mButton;
  private static final SparseArray<Drawable> mIcons = new SparseArray<>(); // Location mode -> Button icon

  private final int mFollowPaddingShift;

  public MyPositionButton(@NonNull View button, @NonNull View.OnClickListener listener)
  {
    mButton = (FloatingActionButton) button;
    mButton.setOnClickListener(listener);
    mIcons.clear();
    mFollowPaddingShift = (int) (FOLLOW_SHIFT * button.getResources().getDisplayMetrics().density);
    update(LocationState.nativeGetMode());
  }

  public void update(int mode)
  {
    Drawable image = mIcons.get(mode);
    @AttrRes int colorAttr = R.attr.iconTint;
    @DimenRes int sizeDimen = R.dimen.map_button_icon_size;
    if (mode == LocationState.FOLLOW || mode == LocationState.FOLLOW_AND_ROTATE || mode == LocationState.PENDING_POSITION)
    {
      colorAttr = R.attr.colorAccent;
      if (mode == LocationState.PENDING_POSITION)
        sizeDimen = R.dimen.map_button_size;
      else
        sizeDimen = R.dimen.map_button_arrow_icon_size;
    }
    Resources resources = mButton.getResources();
    Context context = mButton.getContext();
    if (image == null)
    {
      @DrawableRes int drawableRes;
      switch (mode)
      {
        case LocationState.PENDING_POSITION:
          drawableRes = ThemeUtils.getResource(context, R.attr.myPositionButtonAnimation);
          break;
        case LocationState.NOT_FOLLOW_NO_POSITION:
          drawableRes = R.drawable.ic_location_off;
          break;
        case LocationState.NOT_FOLLOW:
          drawableRes = R.drawable.ic_not_follow;
          break;
        case LocationState.FOLLOW:
          drawableRes = R.drawable.ic_follow;
          break;
        case LocationState.FOLLOW_AND_ROTATE:
          drawableRes = R.drawable.ic_follow_and_rotate;
          break;
        default:
          throw new IllegalArgumentException("Invalid button mode: " + mode);
      }
      image = ResourcesCompat.getDrawable(resources, drawableRes, context.getTheme());
      mIcons.put(mode, image);
    }

    mButton.setImageDrawable(image);
    mButton.setMaxImageSize((int) resources.getDimension(sizeDimen));
    ImageViewCompat.setImageTintList(mButton, ColorStateList.valueOf(ThemeUtils.getColor(context, colorAttr)));
    updatePadding(mode);

    if (image instanceof AnimationDrawable)
      ((AnimationDrawable) image).start();
  }

  private void updatePadding(int mode)
  {
    if (mode == LocationState.FOLLOW)
      mButton.setPadding(0, mFollowPaddingShift, mFollowPaddingShift, 0);
    else
      mButton.setPadding(0, 0, 0, 0);
  }

  public void showButton(boolean show)
  {
    UiUtils.showIf(show, mButton);
  }
}
