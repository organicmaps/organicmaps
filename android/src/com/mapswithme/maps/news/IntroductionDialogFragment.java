package com.mapswithme.maps.news;

import android.app.Dialog;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.FragmentManager;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;

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
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    Dialog res = super.onCreateDialog(savedInstanceState);

    View content = View.inflate(getActivity(), R.layout.fragment_welcome, null);
    res.setContentView(content);
    Bundle args = getArgumentsOrThrow();
    int dataIndex = args.getInt(ARG_INTRODUCTION_FACTORY);
    IntroductionScreenFactory data = IntroductionScreenFactory.values()[dataIndex];
    TextView button = content.findViewById(R.id.btn__continue);
    button.setText(data.getAction());
    button.setOnClickListener(v -> {
      String deepLink = args.getString(ARG_DEEPLINK);
      if (TextUtils.isEmpty(deepLink))
        throw new AssertionError("Deeplink must non-empty within introduction fragment!");
      data.createButtonClickListener().onIntroductionButtonClick(requireActivity(), deepLink);
      dismissAllowingStateLoss();
    });
    ImageView image = content.findViewById(R.id.iv__image);
    image.setImageResource(data.getImage());
    TextView title = content.findViewById(R.id.tv__title);
    title.setText(data.getTitle());
    TextView subtitle = content.findViewById(R.id.tv__subtitle1);
    subtitle.setText(data.getSubtitle());

    return res;
  }

  @Override
  protected int getCustomTheme()
  {
    return getFullscreenTheme();
  }
}
