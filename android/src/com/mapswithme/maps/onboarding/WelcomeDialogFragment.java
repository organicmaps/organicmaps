package com.mapswithme.maps.onboarding;

import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.news.WelcomeScreenBindingType;
import com.mapswithme.util.Counters;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;

public class WelcomeDialogFragment extends BaseMwmDialogFragment implements View.OnClickListener
{
  private static final String BUNDLE_WELCOME_SCREEN_TYPE = "welcome_screen_type";

  @Nullable
  private PolicyAgreementListener mListener;

  @Nullable
  private WelcomeScreenBindingType mWelcomeScreenBindingType;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mContentView;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private ImageView mImage;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mTitle;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mSubtitle;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mAcceptBtn;

  public static void show(@NonNull FragmentActivity activity)
  {
    create(activity);
    Counters.setFirstStartDialogSeen();
  }

  public static boolean isFirstLaunch(@NonNull FragmentActivity activity)
  {
    if (Counters.getFirstInstallVersion() < BuildConfig.VERSION_CODE)
      return false;

    FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return false;

    return !Counters.isFirstStartDialogSeen();
  }

  private static void create(@NonNull FragmentActivity activity)
  {
    final WelcomeDialogFragment fragment = new WelcomeDialogFragment();
    activity.getSupportFragmentManager()
            .beginTransaction()
            .add(fragment, WelcomeDialogFragment.class.getName())
            .commitAllowingStateLoss();
  }

  public static boolean recreate(@NonNull FragmentActivity activity)
  {
    FragmentManager fm = activity.getSupportFragmentManager();
    Fragment f = fm.findFragmentByTag(WelcomeDialogFragment.class.getName());
    if (f == null)
      return false;

    // If we're here, it means that the user has rotated the screen.
    // We use different dialog themes for landscape and portrait modes on tablets,
    // so the fragment should be recreated to be displayed correctly.
    fm.beginTransaction()
      .remove(f)
      .commitAllowingStateLoss();
    fm.executePendingTransactions();
    return true;
  }

  @Override
  public void onAttach(Activity activity)
  {
    super.onAttach(activity);
    if (activity instanceof BaseNewsFragment.NewsDialogListener)
      mListener = (PolicyAgreementListener) activity;
  }

  @Override
  public void onDetach()
  {
    mListener = null;
    super.onDetach();
  }

  @Override
  protected int getCustomTheme()
  {
    return ThemeUtils.isNightTheme() ? R.style.MwmTheme_DialogFragment_NoFullscreen_Night
                                     : R.style.MwmTheme_DialogFragment_NoFullscreen;
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    Dialog res = super.onCreateDialog(savedInstanceState);
    res.requestWindowFeature(Window.FEATURE_NO_TITLE);
    res.setCancelable(false);

    Bundle args = getArguments();
    mWelcomeScreenBindingType = args != null && args.containsKey(BUNDLE_WELCOME_SCREEN_TYPE)
                                ? makeWelcomeScreenType(args) : null;
    mContentView = View.inflate(getActivity(), R.layout.fragment_welcome, null);
    res.setContentView(mContentView);
    mAcceptBtn = mContentView.findViewById(R.id.accept_btn);
    mAcceptBtn.setOnClickListener(this);
    mImage = mContentView.findViewById(R.id.iv__image);
    mImage.setImageResource(R.drawable.img_welcome);
    mTitle = mContentView.findViewById(R.id.tv__title);
    mTitle.setText(R.string.onboarding_welcome_title);
    mSubtitle = mContentView.findViewById(R.id.tv__subtitle1);
    mSubtitle.setText(R.string.onboarding_welcome_first_subtitle);

    bindWelcomeScreenType();

    return res;
  }

  private void bindWelcomeScreenType()
  {
    boolean hasDeclineBtn = mWelcomeScreenBindingType != null
                            && mWelcomeScreenBindingType.hasDeclinedButton();
    TextView declineBtn = mContentView.findViewById(R.id.decline_btn);
    UiUtils.showIf(hasDeclineBtn, declineBtn);
    if (hasDeclineBtn)
      declineBtn.setText(mWelcomeScreenBindingType.getDeclinedButtonResId());

    if (mWelcomeScreenBindingType == null)
      return;

    mTitle.setText(mWelcomeScreenBindingType.getTitle());
    mImage.setImageResource(mWelcomeScreenBindingType.getImage());
    mAcceptBtn.setText(mWelcomeScreenBindingType.getAcceptButtonResId());
    declineBtn.setOnClickListener(v -> {});
    mSubtitle.setText(mWelcomeScreenBindingType.getSubtitle());
  }

  @NonNull
  private static WelcomeScreenBindingType makeWelcomeScreenType(@NonNull Bundle args)
  {
    return WelcomeScreenBindingType.values()[args.getInt(BUNDLE_WELCOME_SCREEN_TYPE)];
  }

  @Override
  public void onClick(View v)
  {
    if (v.getId() != R.id.accept_btn)
      return;

    if (mListener != null)
      mListener.onPolicyAgreementApplied();
    dismiss();
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    super.onCancel(dialog);
    requireActivity().finish();
  }

  public interface PolicyAgreementListener
  {
    void onPolicyAgreementApplied();
  }
}
