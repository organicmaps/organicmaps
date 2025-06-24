package app.organicmaps.routing;

import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Typeface;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Pair;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.FragmentFactory;
import androidx.fragment.app.FragmentManager;

import app.organicmaps.R;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.util.UiUtils;

public class RoutingErrorDialogFragment extends BaseRoutingErrorDialogFragment
{
  private static final String EXTRA_RESULT_CODE = "RouterResultCode";

  private int mResultCode;
  private String mMessage;
  private boolean mNeedMoreMaps;

  @Override
  void beforeDialogCreated(AlertDialog.Builder builder)
  {
    super.beforeDialogCreated(builder);

    ResultCodesHelper.ResourcesHolder resHolder =
        ResultCodesHelper.getDialogTitleSubtitle(requireContext(), mResultCode, mMissingMaps.size());
    Pair<String, String> titleMessage = resHolder.getTitleMessage();

    TextView titleView = new TextView(requireContext());
    titleView.setText(titleMessage.first);
    titleView.setPadding(65, 32, 32, 16);
    titleView.setTextSize(18);
    titleView.setMaxLines(4);
    titleView.setEllipsize(TextUtils.TruncateAt.END);
    titleView.setTypeface(null, Typeface.BOLD);
    builder.setCustomTitle(titleView);

    mMessage = titleMessage.second;
    builder.setNegativeButton(resHolder.getCancelBtnResId(), null);
    if (ResultCodesHelper.isDownloadable(mResultCode, mMissingMaps.size()))
      builder.setPositiveButton(R.string.download, null);

    mNeedMoreMaps = ResultCodesHelper.isMoreMapsNeeded(mResultCode);
    if (mNeedMoreMaps)
      builder.setNegativeButton(R.string.later, null);
  }

  private View addMessage(View frame)
  {
    UiUtils.setTextAndHideIfEmpty(frame.findViewById(R.id.tv__message), mMessage);
    return frame;
  }

  @Override
  public void onDismiss(DialogInterface dialog)
  {
    if (mNeedMoreMaps && mCancelled) {
      mCancelled = false;

      /// @todo Actually, should cancel if there is no valid route only.
      // I didn't realize how to distinguish NEED_MORE_MAPS but valid route is present.
      // Should refactor RoutingController states.
      //RoutingController.get().cancel();
    }

    super.onDismiss(dialog);
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

  private void startDownload()
  {
    if (mMissingMaps.isEmpty())
    {
      dismiss();
      return;
    }

    long size = 0;
    for (CountryItem country : mMissingMaps)
    {
      if (country.status != CountryItem.STATUS_PROGRESS &&
          country.status != CountryItem.STATUS_APPLYING)
      {
        size += country.totalSize;
      }
    }

    MapManager.warnOn3g(requireActivity(), size, () -> {
      final FragmentManager manager = requireActivity().getSupportFragmentManager();
      RoutingMapsDownloadFragment downloader = RoutingMapsDownloadFragment
          .create(manager.getFragmentFactory(), getAppContextOrThrow(), mMapsArray);
      downloader.show(manager, downloader.getClass().getSimpleName());
      mCancelled = false;
      dismiss();
    });
  }

  @Override
  public void onStart()
  {
    super.onStart();

    final AlertDialog dlg = (AlertDialog) getDialog();
    Button button = dlg.getButton(AlertDialog.BUTTON_POSITIVE);
    if (button == null)
      return;

    button.setOnClickListener(v -> startDownload());
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    mCancelled = true;
    super.onCancel(dialog);
  }

  @Override
  void parseArguments()
  {
    super.parseArguments();
    mResultCode = getArguments().getInt(EXTRA_RESULT_CODE);
  }

  public static RoutingErrorDialogFragment create(@NonNull FragmentFactory factory, @NonNull Context context,
                                                  int resultCode, @Nullable String[] missingMaps)
  {
    Bundle args = new Bundle();
    args.putInt(EXTRA_RESULT_CODE, resultCode);
    args.putStringArray(EXTRA_MISSING_MAPS, missingMaps);
    RoutingErrorDialogFragment res = (RoutingErrorDialogFragment)
        factory.instantiate(context.getClassLoader(), RoutingErrorDialogFragment.class.getName());
    res.setArguments(args);
    return res;
  }
}
