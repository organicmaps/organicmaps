package app.organicmaps.routing;

import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Typeface;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Pair;
import android.view.View;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.FragmentFactory;
import androidx.fragment.app.FragmentManager;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.downloader.MapManagerHelper;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.routing.ResultCodes;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.util.Utils;
import app.organicmaps.settings.DrivingOptionsActivity;
import app.organicmaps.util.UiUtils;

public class RoutingErrorDialogFragment extends BaseRoutingErrorDialogFragment
{
  private static final String EXTRA_RESULT_CODE = "RouterResultCode";
  private static final String EXTRA_REPORT = "RoutingReport";

  private int mResultCode;
  private String mMessage;
  private boolean mNeedMoreMaps;

  @Override
  void beforeDialogCreated(AlertDialog.Builder builder)
  {
    super.beforeDialogCreated(builder);

    final Context context = requireContext();
    ResultCodesHelper.ResourcesHolder resHolder = ResultCodesHelper.getDialogTitleSubtitle(
        context, MwmApplication.from(context).getLocationHelper(), mResultCode, mMissingMaps.size());
    Pair<String, String> titleMessage = resHolder.getTitleMessage();

    TextView titleView = new TextView(context);
    titleView.setText(titleMessage.first);
    titleView.setPaddingRelative(Utils.dimen(context, R.dimen.margin_base_plus),
                                 Utils.dimen(context, R.dimen.margin_base), Utils.dimen(context, R.dimen.margin_base),
                                 Utils.dimen(context, R.dimen.margin_half));
    titleView.setTextSize(18);
    titleView.setMaxLines(4);
    titleView.setEllipsize(TextUtils.TruncateAt.END);
    titleView.setTypeface(null, Typeface.BOLD);
    builder.setCustomTitle(titleView);

    mMessage = titleMessage.second;
    mNeedMoreMaps = ResultCodesHelper.isMoreMapsNeeded(mResultCode);
    builder.setNegativeButton(mNeedMoreMaps ? R.string.later : resHolder.getCancelBtnResId(), null);
    final String report = requireArguments().getString(EXTRA_REPORT);
    if (report != null)
    {
      builder.setPositiveButton(R.string.report_a_bug,
                                (dialog, which) -> ((MwmActivity) requireActivity()).reportRoutingError(report));
    }
    else if (ResultCodesHelper.isDownloadable(mResultCode, mMissingMaps.size()))
    {
      builder.setPositiveButton(R.string.download, null);
    }
    else if (offersRoutingOptions())
    {
      builder.setNegativeButton(R.string.ok, null);
      builder.setPositiveButton(R.string.driving_options_title,
                                (dialog, which) -> DrivingOptionsActivity.start(requireActivity()));
    }
  }

  private boolean offersRoutingOptions()
  {
    return switch (mResultCode)
    {
      case ResultCodes.START_POINT_NOT_FOUND, ResultCodes.END_POINT_NOT_FOUND, ResultCodes.INTERMEDIATE_POINT_NOT_FOUND,
          ResultCodes.ROUTE_NOT_FOUND ->
        RoutingOptions.hasAnyOptions(RoutingController.get().getLastRouterType());
      default -> false;
    };
  }

  private View addMessage(View frame)
  {
    UiUtils.setTextAndHideIfEmpty(frame.findViewById(R.id.tv__message), mMessage);
    return frame;
  }

  @Override
  public void onDismiss(DialogInterface dialog)
  {
    // Dismissing a missing-maps prompt keeps planning; only a found route can be started.
    if (mNeedMoreMaps && mCancelled)
      mCancelled = false;

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
    MapManagerHelper.warnOn3g(
        requireActivity(), MapManager.nativeGetDownloadSize(mMapsArray, false /* withTerrain */), () -> {
          // The mobile-data confirmation may outlive this dialog fragment.
          if (!isAdded())
            return;

          final FragmentManager manager = getParentFragmentManager();
          if (manager.isStateSaved() || manager.isDestroyed())
            return;

          RoutingMapsDownloadFragment downloader =
              RoutingMapsDownloadFragment.create(manager.getFragmentFactory(), getAppContextOrThrow(), mMapsArray);
          downloader.show(manager, downloader.getClass().getSimpleName());
          mCancelled = false;
          dismiss();
        });
  }

  @Override
  public void onStart()
  {
    super.onStart();

    if (ResultCodesHelper.isDownloadable(mResultCode, mMissingMaps.size()))
    {
      final AlertDialog dlg = (AlertDialog) requireDialog();
      dlg.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v -> startDownload());
    }
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
                                                  int resultCode, @NonNull String[] missingMaps,
                                                  @Nullable String report)
  {
    Bundle args = new Bundle();
    args.putInt(EXTRA_RESULT_CODE, resultCode);
    args.putString(EXTRA_REPORT, report);
    args.putStringArray(EXTRA_MISSING_MAPS, missingMaps);
    RoutingErrorDialogFragment res = (RoutingErrorDialogFragment) factory.instantiate(
        context.getClassLoader(), RoutingErrorDialogFragment.class.getName());
    res.setArguments(args);
    return res;
  }
}
