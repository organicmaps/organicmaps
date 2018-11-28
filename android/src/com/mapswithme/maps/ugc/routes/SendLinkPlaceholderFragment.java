package com.mapswithme.maps.ugc.routes;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.ShareCompat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.util.sharing.TargetUtils;
import com.mapswithme.util.statistics.Statistics;

import java.util.Objects;

public class SendLinkPlaceholderFragment extends Fragment
{
  public static final String EXTRA_SHARED_LINK = "shared_link";
  private static final String BODY_STRINGS_SEPARATOR = "\n\n";

  @SuppressWarnings("NullableProblems")
  @NonNull
  private String mSharedLink;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Bundle args = getArguments();
    if (args == null)
      throw new IllegalArgumentException("Please, setup arguments");

    mSharedLink = Objects.requireNonNull(args.getString(EXTRA_SHARED_LINK));
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_ugc_route_send_link, container, false);
    View closeBtn = root.findViewById(R.id.close_btn);
    closeBtn.setOnClickListener(v -> getActivity().finish());
    View sendMeLinkBtn = root.findViewById(R.id.send_me_link_btn);
    sendMeLinkBtn.setOnClickListener(v -> shareLink());
    return root;
  }

  private void shareLink()
  {
    String emailBody = getString(R.string.edit_your_guide_email_body) + BODY_STRINGS_SEPARATOR +
                       mSharedLink;

    ShareCompat.IntentBuilder.from(getActivity())
                             .setType(TargetUtils.TYPE_TEXT_PLAIN)
                             .setSubject(getString(R.string.edit_guide_title))
                             .setText(emailBody)
                             .setChooserTitle(getString(R.string.share))
                             .startChooser();
    Statistics.INSTANCE.trackEvent(Statistics.EventName.BM_EDIT_ON_WEB_CLICK);
  }
}
