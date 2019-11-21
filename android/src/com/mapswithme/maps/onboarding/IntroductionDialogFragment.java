package com.mapswithme.maps.onboarding;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.fragment.app.FragmentManager;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

public class IntroductionDialogFragment extends BaseMwmDialogFragment
{
  private static final String ARG_DEEPLINK = "arg_deeplink";
  private static final String ARG_INTRODUCTION_FACTORY = "arg_introduction_factory";

  public static void show(@NonNull FragmentManager fm, @NonNull String deepLink,
                          @NonNull IntroductionScreenFactory factory)
  {
    Bundle args = new Bundle();
    args.putString(IntroductionDialogFragment.ARG_DEEPLINK, deepLink);
    args.putInt(IntroductionDialogFragment.ARG_INTRODUCTION_FACTORY, factory.ordinal());
    final IntroductionDialogFragment fragment = new IntroductionDialogFragment();
    fragment.setArguments(args);
    fragment.show(fm, IntroductionDialogFragment.class.getName());
    Statistics.INSTANCE.trackEvent(Statistics.EventName.ONBOARDING_DEEPLINK_SCREEN_SHOW,
                                   Statistics.params().add(Statistics.EventParam.TYPE,
                                                           factory.toStatisticValue()));
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    Dialog res = super.onCreateDialog(savedInstanceState);

    View content = View.inflate(getActivity(), R.layout.fragment_welcome, null);
    res.setContentView(content);
    IntroductionScreenFactory factory = getScreenFactory();
    TextView button = content.findViewById(R.id.accept_btn);
    button.setText(factory.getAction());
    button.setOnClickListener(v -> onAcceptClicked());
    ImageView image = content.findViewById(R.id.iv__image);
    image.setImageResource(factory.getImage());
    TextView title = content.findViewById(R.id.tv__title);
    title.setText(factory.getTitle());
    TextView subtitle = content.findViewById(R.id.tv__subtitle1);
    subtitle.setText(factory.getSubtitle());
    UiUtils.hide(content, R.id.decline_btn);

    return res;
  }

  @NonNull
  private IntroductionScreenFactory getScreenFactory()
  {
    Bundle args = getArgumentsOrThrow();
    int dataIndex = args.getInt(ARG_INTRODUCTION_FACTORY);
    return IntroductionScreenFactory.values()[dataIndex];
  }

  private void onAcceptClicked()
  {
    String deepLink = getArgumentsOrThrow().getString(ARG_DEEPLINK);
    if (TextUtils.isEmpty(deepLink))
      throw new AssertionError("Deeplink must non-empty within introduction fragment!");
    IntroductionScreenFactory factory = getScreenFactory();
    factory.createButtonClickListener().onIntroductionButtonClick(requireActivity(), deepLink);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.ONBOARDING_DEEPLINK_SCREEN_ACCEPT,
                                   Statistics.params().add(Statistics.EventParam.TYPE,
                                                           factory.toStatisticValue()));
    dismissAllowingStateLoss();
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    super.onCancel(dialog);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.ONBOARDING_DEEPLINK_SCREEN_DECLINE,
                                   Statistics.params().add(Statistics.EventParam.TYPE,
                                                           getScreenFactory().toStatisticValue()));
  }

  @Override
  protected int getCustomTheme()
  {
    return getFullscreenTheme();
  }
}
