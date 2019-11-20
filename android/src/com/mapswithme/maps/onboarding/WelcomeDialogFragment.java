package com.mapswithme.maps.onboarding;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.news.WelcomeScreenBindingType;
import com.mapswithme.util.Counters;
import com.mapswithme.util.SharedPropertiesUtils;
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

  @SuppressWarnings("NullableProblems")
  @NonNull
  private CheckBox mTermOfUseCheckbox;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private CheckBox mPrivacyPolicyCheckbox;

  public static void show(@NonNull FragmentActivity activity)
  {
    create(activity);
    Counters.setFirstStartDialogSeen();
  }

  public static boolean isFirstLaunch(@NonNull FragmentActivity activity)
  {
    if (Counters.getFirstInstallVersion() < BuildConfig.VERSION_CODE)
      return false;

    if (isAgreementDenied(activity))
      return true;

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
    mContentView.findViewById(R.id.privacy_policy_welcome);

    initUserAgreementViews();
    bindWelcomeScreenType();

    return res;
  }

  private void initUserAgreementViews()
  {
    SharedPreferences prefs = MwmApplication.prefs(requireContext());

    mTermOfUseCheckbox = mContentView.findViewById(R.id.term_of_use_welcome_checkbox);
    mTermOfUseCheckbox.setChecked(prefs.getBoolean(SharedPropertiesUtils.USER_AGREEMENT_TERM_OF_USE, false));

    mPrivacyPolicyCheckbox = mContentView.findViewById(R.id.privacy_policy_welcome_checkbox);
    mPrivacyPolicyCheckbox.setChecked(prefs.getBoolean(SharedPropertiesUtils.USER_AGREEMENT_PRIVACY_POLICY, false));

    mTermOfUseCheckbox.setOnCheckedChangeListener(
        (buttonView, isChecked) -> onTermsOfUseViewChanged(isChecked));
    mPrivacyPolicyCheckbox.setOnCheckedChangeListener(
        (buttonView, isChecked) -> onPrivacyPolicyViewChanged(isChecked));

    UiUtils.linkifyPolicyView(mContentView, R.id.privacy_policy_welcome,
                              R.string.sign_agree_pp_gdpr, Framework.nativeGetPrivacyPolicyLink());

    UiUtils.linkifyPolicyView(mContentView, R.id.term_of_use_welcome,
                              R.string.sign_agree_tof_gdpr, Framework.nativeGetTermsOfUseLink());
  }

  private void onPrivacyPolicyViewChanged(boolean isChecked)
  {
    onCheckedValueChanged(isChecked, mTermOfUseCheckbox.isChecked(),
                          SharedPropertiesUtils.USER_AGREEMENT_PRIVACY_POLICY);
  }

  private void onTermsOfUseViewChanged(boolean isChecked)
  {
    onCheckedValueChanged(isChecked, mPrivacyPolicyCheckbox.isChecked(),
                          SharedPropertiesUtils.USER_AGREEMENT_TERM_OF_USE);
  }

  private void onCheckedValueChanged(boolean isChecked,
                                     boolean isAnotherConditionChecked,
                                     @NonNull String key)

  {
    applyPreferenceChanges(key, isChecked);
    boolean isAgreementGranted = isChecked && isAnotherConditionChecked;
    if (!isAgreementGranted)
      return;

    if (mListener != null)
      mListener.onPolicyAgreementApplied();
    dismiss();
  }

  private void bindWelcomeScreenType()
  {
    boolean hasBindingType = mWelcomeScreenBindingType != null;
    UiUtils.showIf(hasBindingType, mContentView, R.id.button_container);

    boolean hasDeclineBtn = hasBindingType
                            && mWelcomeScreenBindingType.hasDeclinedButton();
    TextView declineBtn = mContentView.findViewById(R.id.decline_btn);
    UiUtils.showIf(hasDeclineBtn, declineBtn);

    View userAgreementBlock = mContentView.findViewById(R.id.user_agreement_block);
    UiUtils.hideIf(hasBindingType, userAgreementBlock);

    if (hasDeclineBtn)
      declineBtn.setText(mWelcomeScreenBindingType.getDeclinedButtonResId());

    if (!hasBindingType)
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

  private void applyPreferenceChanges(@NonNull String key, boolean value)
  {
    SharedPreferences.Editor editor = MwmApplication.prefs(requireContext()).edit();
    editor.putBoolean(key, value).apply();
  }

  private static boolean isAgreementDenied(@NonNull Context context)
  {
    SharedPreferences prefs = MwmApplication.prefs(context);
    return !prefs.getBoolean(SharedPropertiesUtils.USER_AGREEMENT_TERM_OF_USE, false)
           || !prefs.getBoolean(SharedPropertiesUtils.USER_AGREEMENT_PRIVACY_POLICY, false);

  }

  public interface PolicyAgreementListener
  {
    void onPolicyAgreementApplied();
  }
}
