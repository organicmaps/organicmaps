package com.mapswithme.maps.onboarding;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;
import android.view.Window;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.news.OnboardingStep;
import com.mapswithme.util.Counters;
import com.mapswithme.util.SharedPropertiesUtils;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

import java.util.Arrays;
import java.util.List;
import java.util.Stack;

public class WelcomeDialogFragment extends BaseMwmDialogFragment implements View.OnClickListener
{
  private static final String ARG_SPECIFIC_STEP = "arg_specific_step";
  private static final String ARG_HAS_MANY_STEPS = "arg_has_many_steps";
  private static final String DEF_STATISTICS_VALUE = "agreement";

  @NonNull
  private final Stack<OnboardingStep> mOnboardingSteps = new Stack<>();

  @Nullable
  private PolicyAgreementListener mPolicyAgreementListener;

  @Nullable
  private OnboardingStepPassedListener mOnboardingStepPassedListener;

  @Nullable
  private OnboardingStep mOnboardinStep;

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
    create(activity, null);
  }

  public static void showOnboardinSteps(@NonNull FragmentActivity activity)
  {
    Bundle args = new Bundle();
    args.putBoolean(ARG_HAS_MANY_STEPS, true);
    create(activity, args);
  }

  public static void showOnboardinStepsStartWith(@NonNull FragmentActivity activity,
                                                 @NonNull OnboardingStep startStep)
  {
    Bundle args = new Bundle();
    args.putBoolean(ARG_HAS_MANY_STEPS, true);
    args.putInt(ARG_SPECIFIC_STEP, startStep.ordinal());
    create(activity, args);
  }

  public static void showOnboardinStep(@NonNull FragmentActivity activity,
                                       @NonNull OnboardingStep step)
  {
    Bundle args = new Bundle();
    args.putInt(ARG_SPECIFIC_STEP, step.ordinal());
    create(activity, args);
  }

  public static boolean isFirstLaunch(@NonNull FragmentActivity activity)
  {
    if (Counters.getFirstInstallVersion(activity.getApplicationContext()) < BuildConfig.VERSION_CODE)
      return false;

    FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return false;

    return !Counters.isFirstStartDialogSeen(activity);
  }

  private static void create(@NonNull FragmentActivity activity, @Nullable Bundle args)
  {
    final WelcomeDialogFragment fragment = new WelcomeDialogFragment();
    fragment.setArguments(args);
    activity.getSupportFragmentManager()
            .beginTransaction()
            .add(fragment, WelcomeDialogFragment.class.getName())
            .commitAllowingStateLoss();
  }

  @Nullable
  public static DialogFragment find(@NonNull FragmentActivity activity)
  {
    final FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return null;

    Fragment f = fm.findFragmentByTag(WelcomeDialogFragment.class.getName());
    return (DialogFragment) f;
  }

  @Override
  public void onAttach(Activity activity)
  {
    super.onAttach(activity);
    if (activity instanceof BaseNewsFragment.NewsDialogListener)
      mPolicyAgreementListener = (PolicyAgreementListener) activity;
    if (activity instanceof OnboardingStepPassedListener)
      mOnboardingStepPassedListener = (OnboardingStepPassedListener) activity;
  }

  @Override
  public void onDetach()
  {
    mPolicyAgreementListener = null;
    super.onDetach();
  }

  @Override
  protected int getCustomTheme()
  {
    return ThemeUtils.isNightTheme(requireContext()) ? R.style.MwmTheme_DialogFragment_NoFullscreen_Night
                                                     : R.style.MwmTheme_DialogFragment_NoFullscreen;
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    Dialog res = super.onCreateDialog(savedInstanceState);
    res.requestWindowFeature(Window.FEATURE_NO_TITLE);
    res.setCancelable(false);

    hanldeOnboardingSteps();

    mContentView = View.inflate(getActivity(), R.layout.fragment_welcome, null);
    res.setContentView(mContentView);
    mAcceptBtn = mContentView.findViewById(R.id.accept_btn);
    mAcceptBtn.setOnClickListener(this);
    mImage = mContentView.findViewById(R.id.iv__image);
    mImage.setImageResource(R.drawable.img_welcome);
    mTitle = mContentView.findViewById(R.id.tv__title);
    List<String> headers = Arrays.asList(getString(R.string.new_onboarding_step1_header),
                                         getString(R.string.new_onboarding_step1_header_2));
    String titleText = TextUtils.join(UiUtils.NEW_STRING_DELIMITER, headers);
    mTitle.setText(titleText);
    mSubtitle = mContentView.findViewById(R.id.tv__subtitle1);
    mSubtitle.setText(R.string.sign_message_gdpr);

    initUserAgreementViews();
    bindWelcomeScreenType();
    if (savedInstanceState == null)
      trackStatisticEvent(Statistics.EventName.ONBOARDING_SCREEN_SHOW);

    return res;
  }

  private void hanldeOnboardingSteps()
  {
    Bundle args = getArguments();
    if (args != null)
    {
      boolean hasManySteps = args.containsKey(ARG_HAS_MANY_STEPS);
      if (hasManySteps)
      {
        mOnboardingSteps.push(OnboardingStep.SHARE_EMOTIONS);
        mOnboardingSteps.push(OnboardingStep.EXPERIENCE);
        mOnboardingSteps.push(OnboardingStep.DREAM_AND_PLAN);
      }

      boolean hasSpecificStep = args.containsKey(ARG_SPECIFIC_STEP);
      if (hasSpecificStep)
        mOnboardinStep =
            OnboardingStep.values()[args.getInt(ARG_SPECIFIC_STEP)];

      if (hasManySteps && hasSpecificStep)
      {
        OnboardingStep step = null;
        while (!mOnboardinStep.equals(step))
        {
          step = mOnboardingSteps.pop();
        }
        mOnboardinStep = step;
        return;
      }

      if (hasManySteps)
        mOnboardinStep = mOnboardingSteps.pop();
    }
  }

  private void initUserAgreementViews()
  {
    mTermOfUseCheckbox = mContentView.findViewById(R.id.term_of_use_welcome_checkbox);
    mTermOfUseCheckbox.setChecked(
        SharedPropertiesUtils.isTermOfUseAgreementConfirmed(requireContext()));

    mPrivacyPolicyCheckbox = mContentView.findViewById(R.id.privacy_policy_welcome_checkbox);
    mPrivacyPolicyCheckbox.setChecked(
        SharedPropertiesUtils.isPrivacyPolicyAgreementConfirmed(requireContext()));

    mTermOfUseCheckbox.setOnCheckedChangeListener(
        (buttonView, isChecked) -> onTermsOfUseViewChanged(isChecked));
    mPrivacyPolicyCheckbox.setOnCheckedChangeListener(
        (buttonView, isChecked) -> onPrivacyPolicyViewChanged(isChecked));

    UiUtils.linkifyView(mContentView, R.id.privacy_policy_welcome,
                        R.string.sign_agree_pp_gdpr, Framework.nativeGetPrivacyPolicyLink());

    UiUtils.linkifyView(mContentView, R.id.term_of_use_welcome,
                        R.string.sign_agree_tof_gdpr, Framework.nativeGetTermsOfUseLink());
  }

  private void onPrivacyPolicyViewChanged(boolean isChecked)
  {
    SharedPropertiesUtils.putPrivacyPolicyAgreement(requireContext(), isChecked);
    onCheckedValueChanged(isChecked, mTermOfUseCheckbox.isChecked());
  }

  private void onTermsOfUseViewChanged(boolean isChecked)
  {
    SharedPropertiesUtils.putTermOfUseAgreement(requireContext(), isChecked);
    onCheckedValueChanged(isChecked, mPrivacyPolicyCheckbox.isChecked());
  }

  private void onCheckedValueChanged(boolean isChecked,
                                     boolean isAnotherConditionChecked)

  {
    boolean isAgreementGranted = isChecked && isAnotherConditionChecked;
    if (!isAgreementGranted)
      return;

    trackStatisticEvent(Statistics.EventName.ONBOARDING_SCREEN_ACCEPT);

    if (mPolicyAgreementListener != null)
      mPolicyAgreementListener.onPolicyAgreementApplied();
    dismissAllowingStateLoss();
  }

  private void bindWelcomeScreenType()
  {
    boolean hasBindingType = mOnboardinStep != null;
    UiUtils.showIf(hasBindingType, mContentView, R.id.button_container);

    boolean hasDeclineBtn = hasBindingType
                            && mOnboardinStep.hasDeclinedButton();
    TextView declineBtn = mContentView.findViewById(R.id.decline_btn);
    UiUtils.showIf(hasDeclineBtn, declineBtn);

    View userAgreementBlock = mContentView.findViewById(R.id.user_agreement_block);
    UiUtils.hideIf(hasBindingType, userAgreementBlock);

    if (hasDeclineBtn)
      declineBtn.setText(mOnboardinStep.getDeclinedButtonResId());

    if (!hasBindingType)
      return;

    mTitle.setText(mOnboardinStep.getTitle());
    mImage.setImageResource(mOnboardinStep.getImage());
    mAcceptBtn.setText(mOnboardinStep.getAcceptButtonResId());
    declineBtn.setOnClickListener(v -> onDeclineBtnClicked());
    mSubtitle.setText(mOnboardinStep.getSubtitle());
  }

  private void onDeclineBtnClicked()
  {
    Counters.setFirstStartDialogSeen(requireContext());
    trackStatisticEvent(Statistics.EventName.ONBOARDING_SCREEN_DECLINE);
    dismissAllowingStateLoss();
  }

  private void trackStatisticEvent(@NonNull String event)
  {
    String value = mOnboardinStep == null ? DEF_STATISTICS_VALUE : mOnboardinStep.toStatisticValue();
    Statistics.ParameterBuilder builder = Statistics
        .params().add(Statistics.EventParam.TYPE, value);
    Statistics.INSTANCE.trackEvent(event, builder);
  }

  @Override
  public void onClick(View v)
  {
    if (v.getId() != R.id.accept_btn)
      return;

    trackStatisticEvent(Statistics.EventName.ONBOARDING_SCREEN_ACCEPT);

    if (!mOnboardingSteps.isEmpty())
    {
      mOnboardinStep = mOnboardingSteps.pop();
      if (mOnboardingStepPassedListener != null)
        mOnboardingStepPassedListener.onOnboardingStepPassed(mOnboardinStep);
      bindWelcomeScreenType();
      return;
    }

    if (mOnboardinStep != null && mOnboardingStepPassedListener != null)
      mOnboardingStepPassedListener.onOnboardingStepPassed(mOnboardinStep);

    Counters.setFirstStartDialogSeen(requireContext());
    dismissAllowingStateLoss();

    if (mOnboardingStepPassedListener != null)
      mOnboardingStepPassedListener.onLastOnboardingStepPassed();
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    super.onCancel(dialog);
    if (!isAgreementDeclined(requireContext()))
      Counters.setFirstStartDialogSeen(requireContext());
    if (mOnboardingStepPassedListener != null)
      mOnboardingStepPassedListener.onOnboardingStepCancelled();
  }

  public static boolean isAgreementDeclined(@NonNull Context context)
  {
    return !SharedPropertiesUtils.isTermOfUseAgreementConfirmed(context)
           || !SharedPropertiesUtils.isPrivacyPolicyAgreementConfirmed(context);

  }

  public interface PolicyAgreementListener
  {
    void onPolicyAgreementApplied();
  }

  public interface OnboardingStepPassedListener
  {
    void onOnboardingStepPassed(@NonNull OnboardingStep step);
    void onLastOnboardingStepPassed();
    void onOnboardingStepCancelled();
  }
}
