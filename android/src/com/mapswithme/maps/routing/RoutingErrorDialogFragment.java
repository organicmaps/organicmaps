package com.mapswithme.maps.routing;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v7.app.AlertDialog;
import android.util.Pair;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.downloader.CountryItem;
import com.mapswithme.util.UiUtils;

public class RoutingErrorDialogFragment extends BaseRoutingErrorDialogFragment
{
  private static final String EXTRA_RESULT_CODE = "ResultCode";

  private int mResultCode;
  private String mMessage;

  interface Listener
  {
    boolean onDownload();
  }

  private Listener mListener;

  public void setListener(Listener listener)
  {
    mListener = listener;
  }

  @Override
  void beforeDialogCreated(AlertDialog.Builder builder)
  {
    super.beforeDialogCreated(builder);

    Pair<String, String> titleMessage = ResultCodesHelper.getDialogTitleSubtitle(mResultCode, mMissingMaps.size());
    builder.setTitle(titleMessage.first);
    mMessage = titleMessage.second;

    if (ResultCodesHelper.isDownloadable(mResultCode))
      builder.setPositiveButton(R.string.download, null);
  }

  private View addMessage(View frame)
  {
    UiUtils.setTextAndHideIfEmpty((TextView)frame.findViewById(R.id.tv__message), mMessage);
    return frame;
  }

  @Override
  View buildSingleMapView(CountryItem map)
  {
    return addMessage(super.buildSingleMapView(map));
  }

  @Override
  View buildMultipleMapView()
  {
    return addMessage(super.buildMultipleMapView());
  }

  @Override
  public void onStart()
  {
    super.onStart();

    final AlertDialog dlg = (AlertDialog) getDialog();
    Button button = dlg.getButton(AlertDialog.BUTTON_POSITIVE);
    if (button == null)
      return;

    button.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        if (mListener == null)
        {
          dlg.dismiss();
          return;
        }

        if (mMissingMaps.isEmpty())
          dlg.dismiss();
        else if (mListener.onDownload())
          dlg.dismiss();
      }
    });

    button = dlg.getButton(AlertDialog.BUTTON_NEGATIVE);
    button.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        dlg.dismiss();
        RoutingController.get().cancel();
      }
    });
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    mListener = null;
  }

  @Override
  void parseArguments()
  {
    super.parseArguments();
    mResultCode = getArguments().getInt(EXTRA_RESULT_CODE);
  }

  public static RoutingErrorDialogFragment create(int resultCode, @Nullable String[] missingMaps)
  {
    Bundle args = new Bundle();
    args.putInt(EXTRA_RESULT_CODE, resultCode);
    args.putStringArray(EXTRA_MISSING_MAPS, missingMaps);
    RoutingErrorDialogFragment res = (RoutingErrorDialogFragment)Fragment.instantiate(MwmApplication.get(), RoutingErrorDialogFragment.class.getName());
    res.setArguments(args);
    return res;
  }
}
