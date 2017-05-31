package com.mapswithme.maps.news;

import android.app.Dialog;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.view.Window;
import android.widget.TextView;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.Counters;

public class WelcomeDialogFragment extends BaseMwmDialogFragment implements View.OnClickListener
{
  @Nullable
  private BaseNewsFragment.NewsDialogListener mListener;

  public static boolean showOn(@NonNull FragmentActivity activity,
                               @Nullable BaseNewsFragment.NewsDialogListener listener)
  {
    if (Counters.getFirstInstallVersion() < BuildConfig.VERSION_CODE)
      return false;

    FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return false;

    if (Counters.isFirstStartDialogSeen() &&
        !recreate(activity))
      return false;

    create(activity, listener);

    Counters.setFirstStartDialogSeen();
    return true;
  }

  private static void create(@NonNull FragmentActivity activity,
                             @Nullable BaseNewsFragment.NewsDialogListener listener)
  {
    final WelcomeDialogFragment fragment = new WelcomeDialogFragment();
    fragment.mListener = listener;
    activity.getSupportFragmentManager()
            .beginTransaction()
            .add(fragment, WelcomeDialogFragment.class.getName())
            .commitAllowingStateLoss();
  }

  private static boolean recreate(FragmentActivity activity)
  {
    FragmentManager fm = activity.getSupportFragmentManager();
    Fragment f = fm.findFragmentByTag(WelcomeDialogFragment.class.getName());
    if (f == null)
      return false;

    // If we're here, it means that the user has rotated the screen.
    // We use different dialog themes for landscape and portrait modes on tablets,
    // so the fragment should be recreated to be displayed correctly.
    fm.beginTransaction().remove(f).commitAllowingStateLoss();
    fm.executePendingTransactions();
    return true;
  }

  @Override
  protected int getCustomTheme()
  {
    return getFullscreenTheme();
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    LocationHelper.INSTANCE.onEnteredIntoFirstRun();

    Dialog res = super.onCreateDialog(savedInstanceState);
    res.requestWindowFeature(Window.FEATURE_NO_TITLE);
    res.setCancelable(false);

    View content = View.inflate(getActivity(), R.layout.fragment_welcome, null);
    res.setContentView(content);
    content.findViewById(R.id.btn__continue).setOnClickListener(this);
    TextView terms = (TextView) content.findViewById(R.id.tv__terms_and_privacy);
    terms.setText(Html.fromHtml(getString(R.string.onboarding_welcome_second_subtitle)));
    terms.setMovementMethod(LinkMovementMethod.getInstance());

    return res;
  }

  @Override
  public void onClick(View v)
  {
    if (v.getId() != R.id.btn__continue)
      return;

    if (mListener != null)
      mListener.onDialogDone();
    dismiss();
  }
}
