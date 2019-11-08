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
import com.mapswithme.util.UiUtils;

public class WelcomeDialogFragment extends BaseMwmDialogFragment implements View.OnClickListener
{
  private static final String BUNDLE_WELCOME_SCREEN_TYPE = "welcome_screen_type";

  @Nullable
  private PolicyAgreementListener mListener;

  @Nullable
  private WelcomeScreenBindingType mWelcomeScreenBindingType;

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
    return getFullscreenTheme();
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
    View content = View.inflate(getActivity(), R.layout.fragment_welcome, null);
    res.setContentView(content);
    TextView acceptBtn = content.findViewById(R.id.accept_btn);
    acceptBtn.setOnClickListener(this);
    ImageView image = content.findViewById(R.id.iv__image);
    image.setImageResource(R.drawable.img_welcome);
    TextView title = content.findViewById(R.id.tv__title);
    title.setText(R.string.onboarding_welcome_title);
    TextView subtitle = content.findViewById(R.id.tv__subtitle1);
    subtitle.setText(R.string.onboarding_welcome_first_subtitle);

    bindWelcomeScreenType(content, image, title, subtitle, acceptBtn);

    return res;
  }

  private void bindWelcomeScreenType(@NonNull View content, @NonNull ImageView image,
                                     @NonNull TextView title, @NonNull TextView subtitle,
                                     @NonNull TextView acceptBtn)
  {
    boolean hasDeclineBtn = mWelcomeScreenBindingType != null
                            && mWelcomeScreenBindingType.getDeclineButton() != null;
    TextView declineBtn = content.findViewById(R.id.decline_btn);
    UiUtils.showIf(hasDeclineBtn, declineBtn);
    if (hasDeclineBtn)
      declineBtn.setText(mWelcomeScreenBindingType.getDeclineButton());

    if (mWelcomeScreenBindingType == null)
      return;

    title.setText(mWelcomeScreenBindingType.getTitle());
    image.setImageResource(mWelcomeScreenBindingType.getImage());
    acceptBtn.setText(mWelcomeScreenBindingType.getAcceptButton());
    declineBtn.setOnClickListener(v -> {});

    boolean hasSubtitle = mWelcomeScreenBindingType.getSubtitle() != null;
    UiUtils.showIf(hasSubtitle, subtitle);
    if (hasSubtitle)
      subtitle.setText(mWelcomeScreenBindingType.getSubtitle());
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
