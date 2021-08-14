package com.mapswithme.maps.settings;

import android.app.Activity;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;

import androidx.appcompat.app.AlertDialog;

import com.mapswithme.maps.R;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Utils;

public abstract class FeedbackDialog
{
    public static void show(Activity activity)
    {

        DialogInterface.OnClickListener clickListener = new DialogInterface.OnClickListener() {
            private void sendGeneralFeedback() {
                Utils.sendFeedback(activity);
            }

            private void reportBug() {
                Utils.sendBugReport(activity, "Bugreport from user");
            }

            private void telegram() {
                activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.TELEGRAM)));
            }

            private void github() {
                activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.GITHUB)));
            }

            @Override
            public void onClick(DialogInterface dialog, int which) {
                switch (which) {
                    case 0:
                        sendGeneralFeedback();
                        break;

                    case 1:
                        reportBug();
                        break;

                    case 2:
                        telegram();
                        break;

                    case 3:
                        github();
                        break;
                }
            }
        };

        new AlertDialog.Builder(activity)
                .setTitle(activity.getString(R.string.feedback))
                .setNegativeButton(activity.getString(R.string.cancel), null)
                .setItems(new CharSequence[]{
                                activity.getString(R.string.feedback_general),
                                activity.getString(R.string.report_a_bug),
                                activity.getString(R.string.telegram),
                                activity.getString(R.string.github)
                        },
                        clickListener).show();
    }
}
