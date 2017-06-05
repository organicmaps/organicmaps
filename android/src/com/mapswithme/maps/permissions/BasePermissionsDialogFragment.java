package com.mapswithme.maps.permissions;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.DrawableRes;
import android.support.annotation.IdRes;
import android.support.annotation.LayoutRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
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
  private static final String ARG_REQUEST_ID = "arg_request_id";

  private int mRequestId;

  @SuppressWarnings("TryWithIdenticalCatches")
  @Nullable
  public static DialogFragment show(@NonNull FragmentActivity activity, int requestId,
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
      args.putInt(ARG_REQUEST_ID, requestId);
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
      mRequestId = args.getInt(ARG_REQUEST_ID);
  }

  @Override
  protected int getCustomTheme()
  {
    return super.getFullscreenTheme();
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
      button.setOnClickListener(this);

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
    {
      onFirstActionClick();
      return;
    }

    if (v.getId() == getContinueActionButton())
    {
      PermissionsUtils.requestPermissions(getActivity(), mRequestId);
    }
  }

  protected int getRequestId()
  {
    return mRequestId;
  }
}
