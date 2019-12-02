package com.mapswithme.maps.permissions;

import android.app.Dialog;
import android.os.Bundle;
import androidx.annotation.DrawableRes;
import androidx.annotation.IdRes;
import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import android.view.View;
import android.view.Window;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.PermissionsUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

abstract class BasePermissionsDialogFragment extends BaseMwmDialogFragment
    implements View.OnClickListener
{
  private static final String TAG = BasePermissionsDialogFragment.class.getName();
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String ARG_REQUEST_CODE = "arg_request_code";

  private int mRequestCode;

  @SuppressWarnings("TryWithIdenticalCatches")
  @Nullable
  public static DialogFragment show(@NonNull FragmentActivity activity, int requestCode,
                                    @NonNull Class<? extends BaseMwmDialogFragment> dialogClass)
  {
    final FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return null;

    Fragment f = fm.findFragmentByTag(dialogClass.getName());
    if (f != null)
      return (DialogFragment) f;

    BaseMwmDialogFragment dialog = null;
    try
    {
      dialog = dialogClass.newInstance();
      final Bundle args = new Bundle();
      args.putInt(ARG_REQUEST_CODE, requestCode);
      dialog.setArguments(args);
      dialog.show(fm, dialogClass.getName());
    }
    catch (java.lang.InstantiationException e)
    {
      LOGGER.e(TAG, "Can't instantiate " + dialogClass.getName() + " fragment", e);
    }
    catch (IllegalAccessException e)
    {
      LOGGER.e(TAG, "Can't instantiate " + dialogClass.getName() + " fragment", e);
    }

    return dialog;
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Bundle args = getArguments();
    if (args != null)
      mRequestCode = args.getInt(ARG_REQUEST_CODE);
  }

  @Override
  protected int getCustomTheme()
  {
    // We can't read actual theme, because permissions are not granted yet.
    return R.style.MwmTheme_DialogFragment_Fullscreen;
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    Dialog res = super.onCreateDialog(savedInstanceState);
    res.requestWindowFeature(Window.FEATURE_NO_TITLE);

    View content = View.inflate(getActivity(), getLayoutRes(), null);
    res.setContentView(content);
    View button = content.findViewById(getFirstActionButton());
    if (button != null)
      button.setOnClickListener(this);
    button = content.findViewById(getContinueActionButton());
    if (button != null)
      button.setOnClickListener(this::onContinueBtnClicked);

    ImageView image = (ImageView) content.findViewById(R.id.iv__image);
    if (image != null)
      image.setImageResource(getImageRes());
    TextView title = (TextView) content.findViewById(R.id.tv__title);
    if (title != null)
      title.setText(getTitleRes());
    TextView subtitle = (TextView) content.findViewById(R.id.tv__subtitle1);
    if (subtitle != null)
      subtitle.setText(getSubtitleRes());

    return res;
  }

  protected void onContinueBtnClicked(View v)
  {
    PermissionsUtils.requestPermissions(requireActivity(), mRequestCode);
  }

  @DrawableRes
  protected int getImageRes()
  {
    return 0;
  }

  @StringRes
  protected int getTitleRes()
  {
    return 0;
  }

  @StringRes
  protected int getSubtitleRes()
  {
    return 0;
  }

  @LayoutRes
  abstract protected int getLayoutRes();

  @IdRes
  protected abstract int getFirstActionButton();

  protected abstract void onFirstActionClick();

  @IdRes
  protected abstract int getContinueActionButton();

  @Override
  public void onClick(@NonNull View v)
  {
    if (v.getId() == getFirstActionButton())
      onFirstActionClick();
  }

  protected int getRequestCode()
  {
    return mRequestCode;
  }
}
