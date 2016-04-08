package com.mapswithme.maps.routing;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.DialogInterface;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v7.app.AlertDialog;
import android.util.Pair;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.ExpandableListAdapter;
import android.widget.ExpandableListView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.DisabledChildSimpleExpandableListAdapter;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.downloader.CountryItem;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;

public class RoutingErrorDialogFragment extends BaseMwmDialogFragment
{
  private static final String EXTRA_RESULT_CODE = "ResultCode";
  private static final String EXTRA_MISSING_MAPS = "MissingMaps";

  private static final String GROUP_NAME = "GroupName";
  private static final String GROUP_SIZE = "GroupSize";
  private static final String COUNTRY_NAME = "CountryName";

  private final List<CountryItem> mMissingMaps = new ArrayList<>();
  private int mResultCode;

  public interface Listener
  {
    boolean onDownload();
    void onOk();
  }

  private Listener mListener;

  public void setListener(Listener listener)
  {
    mListener = listener;
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    parseArguments();
    final Pair<String, String> titleMessage = ResultCodesHelper.getDialogTitleSubtitle(mResultCode, mMissingMaps.size());
    AlertDialog.Builder builder = new AlertDialog.Builder(getActivity())
                                                 .setTitle(titleMessage.first)
                                                 .setCancelable(true);

    if (mMissingMaps.isEmpty())
      return builder.setMessage(titleMessage.second)
                    .setPositiveButton(android.R.string.ok, new Dialog.OnClickListener()
                    {
                      @Override
                      public void onClick(DialogInterface dialog, int which)
                      {
                        if (mListener != null)
                          mListener.onOk();
                      }
                    }).create();

    View view = (mMissingMaps.size() == 1 ? buildSingleMapView(titleMessage.second, mMissingMaps.get(0))
                                          : buildMultipleMapView(titleMessage.second));
    return builder.setView(view)
                  .setNegativeButton(android.R.string.cancel, null)
                  .setPositiveButton(R.string.download, null)
                  .create();
  }

  @Override
  public void onStart()
  {
    super.onStart();

    final AlertDialog dlg = (AlertDialog) getDialog();
    dlg.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(new View.OnClickListener()
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
        {
          mListener.onOk();
          dlg.dismiss();
        }
        else if (mListener.onDownload())
          dlg.dismiss();
      }
    });
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    mListener = null;
  }

  private void parseArguments()
  {
    final Bundle args = getArguments();
    mResultCode = args.getInt(EXTRA_RESULT_CODE);
    String[] maps = args.getStringArray(EXTRA_MISSING_MAPS);
    if (maps == null)
      return;

    for (String map : maps)
      mMissingMaps.add(CountryItem.fill(map));
  }

  private View buildSingleMapView(String message, CountryItem map)
  {
    @SuppressLint("InflateParams")
    final View countryView = View.inflate(getActivity(), R.layout.dialog_download_single_item, null);
    ((TextView) countryView.findViewById(R.id.tv__title)).setText(map.name);
    ((TextView) countryView.findViewById(R.id.tv__message)).setText(message);

    final TextView szView = (TextView) countryView.findViewById(R.id.tv__size);
    szView.setText(MapManager.nativeIsLegacyMode() ? "" : StringUtils.getFileSizeString(map.totalSize - map.size));
    ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) szView.getLayoutParams();
    lp.rightMargin = 0;
    szView.setLayoutParams(lp);

    return countryView;
  }

  private View buildMultipleMapView(String message)
  {
    @SuppressLint("InflateParams")
    final View countriesView = View.inflate(getActivity(), R.layout.dialog_download_multiple_items, null);
    ((TextView) countriesView.findViewById(R.id.tv__message)).setText(message);

    final ExpandableListView listView = (ExpandableListView) countriesView.findViewById(R.id.elv__items);
    listView.setAdapter(buildAdapter());
    listView.setChildDivider(new ColorDrawable(getResources().getColor(android.R.color.transparent)));

    UiUtils.waitLayout(listView, new ViewTreeObserver.OnGlobalLayoutListener()
    {
      @Override
      public void onGlobalLayout()
      {
        final int width = listView.getWidth();
        final int indicatorWidth = UiUtils.dimen(R.dimen.margin_quadruple);
        listView.setIndicatorBounds(width - indicatorWidth, width);
        if (Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.JELLY_BEAN_MR2)
          listView.setIndicatorBounds(width - indicatorWidth, width);
        else
          listView.setIndicatorBoundsRelative(width - indicatorWidth, width);
      }
    });

    return countriesView;
  }

  private ExpandableListAdapter buildAdapter()
  {
    List<Map<String, String>> countries = new ArrayList<>();
    long size = 0;
    boolean legacy = MapManager.nativeIsLegacyMode();

    for (CountryItem item: mMissingMaps)
    {
      Map<String, String> data = new HashMap<>();
      data.put(COUNTRY_NAME, item.name);
      countries.add(data);

      if (!legacy)
        size += (item.totalSize - item.size);
    }

    Map<String, String> group = new HashMap<>();
    group.put(GROUP_NAME, getString(R.string.maps) + " (" + mMissingMaps.size() + ") ");
    group.put(GROUP_SIZE, (legacy ? "" : StringUtils.getFileSizeString(size)));

    List<Map<String, String>> groups = new ArrayList<>();
    groups.add(group);

    List<List<Map<String, String>>> children = new ArrayList<>();
    children.add(countries);

    return new DisabledChildSimpleExpandableListAdapter(getActivity(),
                                                        groups,
                                                        R.layout.item_country_group_dialog_expanded,
                                                        R.layout.item_country_dialog,
                                                        new String[] { GROUP_NAME, GROUP_SIZE },
                                                        new int[] { R.id.tv__title, R.id.tv__size },
                                                        children,
                                                        R.layout.item_country_dialog,
                                                        new String[] { COUNTRY_NAME },
                                                        new int[] { R.id.tv__title }
    );
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
