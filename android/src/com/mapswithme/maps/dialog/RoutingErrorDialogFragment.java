package com.mapswithme.maps.dialog;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.DialogInterface;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.app.AlertDialog;
import android.util.Pair;
import android.view.View;
import android.view.ViewTreeObserver;
import android.widget.ExpandableListAdapter;
import android.widget.ExpandableListView;
import android.widget.TextView;

import com.mapswithme.country.StorageOptions;
import com.mapswithme.maps.MapStorage;
import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.DisabledChildSimpleExpandableListAdapter;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.data.RoutingResultCodesProcessor;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class RoutingErrorDialogFragment extends BaseMwmDialogFragment
{
  public static final String EXTRA_RESULT_CODE = "ResultCode";
  public static final String EXTRA_MISSING_COUNTRIES = "MissingCountries";

  private static final String GROUP_NAME = "GroupName";
  private static final String GROUP_SIZE = "GroupSize";
  private static final String COUNTRY_NAME = "CountryName";

  private MapStorage.Index[] mMissingCountries;
  private int mResultCode;

  public interface RoutingDialogListener
  {
    void onDownload();

    void onCancel();

    void onOk();
  }

  private RoutingDialogListener mListener;

  public RoutingErrorDialogFragment() {}

  public void setListener(RoutingDialogListener listener)
  {
    mListener = listener;
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    parseArguments();
    final Pair<String, String> titleMessage = RoutingResultCodesProcessor.getDialogTitleSubtitle(mResultCode, mMissingCountries);
    AlertDialog.Builder builder = new AlertDialog.Builder(getActivity())
        .setTitle(titleMessage.first)
        .setCancelable(true);
    if (mMissingCountries != null && mMissingCountries.length != 0)
    {
      if (mMissingCountries.length == 1)
        builder.setView(buildSingleMapView(titleMessage.second));
      else
        builder.setView(buildMultipleMapView(titleMessage.second));

      builder
          .setPositiveButton(R.string.download, new Dialog.OnClickListener()
          {
            @Override
            public void onClick(DialogInterface dialog, int which)
            {
              if (mListener != null)
                mListener.onDownload();
            }
          })
          .setNegativeButton(android.R.string.cancel, new Dialog.OnClickListener()
          {
            @Override
            public void onClick(DialogInterface dialog, int which)
            {
              if (mListener != null)
                mListener.onCancel();
            }
          });
    }
    else
    {
      builder.setMessage(titleMessage.second)
          .setPositiveButton(android.R.string.ok, new Dialog.OnClickListener()
          {
            @Override
            public void onClick(DialogInterface dialog, int which)
            {
              if (mListener != null)
                mListener.onOk();
            }
          });
    }

    return builder.create();
  }

  @Override
  public void onDismiss(DialogInterface dialog)
  {
    super.onDismiss(dialog);

    if (mListener != null)
      mListener.onCancel();
  }

  private void parseArguments()
  {
    final Bundle args = getArguments();
    mMissingCountries = (MapStorage.Index[]) args.getSerializable(EXTRA_MISSING_COUNTRIES);
    mResultCode = args.getInt(EXTRA_RESULT_CODE);
  }

  private View buildSingleMapView(String message)
  {
    @SuppressLint("InflateParams") final View countryView = getActivity().getLayoutInflater().inflate(R.layout.dialog_download_single_item, null);
    ((TextView) countryView.findViewById(R.id.tv__title)).setText(MapStorage.INSTANCE.countryName(mMissingCountries[0]));
    final String size = StringUtils.getFileSizeString(MapStorage.INSTANCE.countryRemoteSizeInBytes(mMissingCountries[0], StorageOptions.MAP_OPTION_MAP_AND_CAR_ROUTING));
    UiUtils.setTextAndShow(((TextView) countryView.findViewById(R.id.tv__size)), size);
    UiUtils.setTextAndShow(((TextView) countryView.findViewById(R.id.tv__message)), message);
    return countryView;
  }

  private View buildMultipleMapView(String message)
  {
    @SuppressLint("InflateParams") final View countriesView = getActivity().getLayoutInflater().inflate(R.layout.dialog_download_multiple_items, null);
    UiUtils.setTextAndShow(((TextView) countriesView.findViewById(R.id.tv__message)), message);

    final ExpandableListView listView = (ExpandableListView) countriesView.findViewById(R.id.elv__items);
    listView.setAdapter(buildAdapter());
    listView.setChildDivider(new ColorDrawable(getResources().getColor(android.R.color.transparent)));

    ViewTreeObserver observer = listView.getViewTreeObserver();
    observer.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener()
    {
      @SuppressWarnings("deprecation")
      @Override
      public void onGlobalLayout()
      {
        final int width = listView.getWidth();
        final int indicatorWidth = getResources().getDimensionPixelSize(R.dimen.margin_quadruple);
        listView.setIndicatorBounds(width - indicatorWidth, width);
        if (Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.JELLY_BEAN_MR2)
          listView.setIndicatorBounds(width - indicatorWidth, width);
        else
          listView.setIndicatorBoundsRelative(width - indicatorWidth, width);

        final ViewTreeObserver treeObserver = listView.getViewTreeObserver();
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN)
          treeObserver.removeGlobalOnLayoutListener(this);
        else
          treeObserver.removeOnGlobalLayoutListener(this);
      }
    });

    return countriesView;
  }

  private ExpandableListAdapter buildAdapter()
  {
    final List<Map<String, String>> groupData = new ArrayList<>();

    final Map<String, String> countriesGroup = new HashMap<>();
    countriesGroup.put(GROUP_NAME, getString(R.string.maps) + " (" + mMissingCountries.length + ") ");
    countriesGroup.put(GROUP_SIZE, StringUtils.getFileSizeString(getCountriesSizeInBytes(StorageOptions.MAP_OPTION_MAP_ONLY)));
    groupData.add(countriesGroup);

    final Map<String, String> routesGroup = new HashMap<>();
    routesGroup.put(GROUP_NAME, getString(R.string.dialog_routing_routes_size) + " (" + mMissingCountries.length + ") ");
    routesGroup.put(GROUP_SIZE, StringUtils.getFileSizeString(getCountriesSizeInBytes(StorageOptions.MAP_OPTION_CAR_ROUTING)));
    groupData.add(routesGroup);

    final List<List<Map<String, String>>> childData = new ArrayList<>();

    final List<Map<String, String>> countries = new ArrayList<>();
    for (MapStorage.Index index : mMissingCountries)
    {
      final Map<String, String> countryData = new HashMap<>();
      countryData.put(COUNTRY_NAME, MapStorage.INSTANCE.countryName(index));
      countries.add(countryData);
    }
    childData.add(countries);
    childData.add(countries);

    return new DisabledChildSimpleExpandableListAdapter(getActivity(),
        groupData,
        R.layout.item_country_group_dialog_expanded,
        R.layout.item_country_group_dialog,
        new String[]{GROUP_NAME, GROUP_SIZE},
        new int[]{R.id.tv__title, R.id.tv__size},
        childData,
        R.layout.item_country_dialog,
        new String[]{COUNTRY_NAME},
        new int[]{R.id.tv__title}
    );
  }

  private long getCountriesSizeInBytes(int option)
  {
    long total = 0;
    for (MapStorage.Index index : mMissingCountries)
      total += MapStorage.INSTANCE.countryRemoteSizeInBytes(index, option);
    return total;
  }
}
