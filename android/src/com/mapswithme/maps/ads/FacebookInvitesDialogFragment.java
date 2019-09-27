package com.mapswithme.maps.ads;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;

import com.facebook.FacebookSdk;
import com.facebook.share.model.AppInviteContent;
import com.facebook.share.widget.AppInviteDialog;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.dialog.DialogUtils;
import com.mapswithme.util.statistics.Statistics;

public class FacebookInvitesDialogFragment extends BaseMwmDialogFragment
{
  private static final String INVITE_APP_URL = "https://fb.me/958251974218933";
  private static final String INVITE_IMAGE = "http://maps.me/images/fb_app_invite_banner.png";

  private boolean mHasInvited;

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    final AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
    final LayoutInflater inflater = getActivity().getLayoutInflater();

    final View root = inflater.inflate(R.layout.fragment_app_invites_dialog, null);
    return builder.setView(root)
                  .setNegativeButton(R.string.remind_me_later, new DialogInterface.OnClickListener()
                  {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                      Statistics.INSTANCE.trackEvent(Statistics.EventName.FACEBOOK_INVITE_LATER);
                    }
                  })
                  .setPositiveButton(R.string.share, new DialogInterface.OnClickListener()
                  {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                      mHasInvited = true;
                      showAppInviteDialog();
                      Statistics.INSTANCE.trackEvent(Statistics.EventName.FACEBOOK_INVITE_INVITED);
                    }
                  }).create();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    if (mHasInvited)
      dismiss();
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    super.onCancel(dialog);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.FACEBOOK_INVITE_LATER);
  }

  private void showAppInviteDialog()
  {
    FacebookSdk.sdkInitialize(getActivity());
    AppInviteContent content = new AppInviteContent.Builder()
        .setApplinkUrl(INVITE_APP_URL)
        .setPreviewImageUrl(INVITE_IMAGE)
        .build();
    if (AppInviteDialog.canShow())
      AppInviteDialog.show(this, content);
    else
    {
      DialogUtils.showAlertDialog(getActivity(), R.string.email_error_title);
      dismiss();
    }
  }
}
