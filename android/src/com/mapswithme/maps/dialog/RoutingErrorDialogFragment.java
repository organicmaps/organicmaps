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
import com.mapswithme.maps.routing.RoutingResultCodesProcessor;
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
  public static final String EXTRA_MISSING_ROUTES = "MissingRoutes";

  private static final String GROUP_NAME = "GroupName";
  private static final String GROUP_SIZE = "GroupSize";
  private static final String COUNTRY_NAME = "CountryName";

  private MapStorage.Index[] mMissingCountries;
  private MapStorage.Index[] mMissingRoutes;
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
    if (hasIndex(mMissingCountries) || hasIndex(mMissingRoutes))
    {
      View view;
      if (hasSingleIndex(mMissingCountries) && !hasIndex(mMissingRoutes))
        view = buildSingleMapView(titleMessage.second, mMissingCountries[0], StorageOptions.MAP_OPTION_MAP_AND_CAR_ROUTING);
      else
        view = buildMultipleMapView(titleMessage.second);

      builder.setView(view);
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
    mMissingRoutes = (MapStorage.Index[]) args.getSerializable(EXTRA_MISSING_ROUTES);
    mResultCode = args.getInt(EXTRA_RESULT_CODE);
  }

  private boolean hasIndex(MapStorage.Index[] indexes)
  {
    return indexes != null && indexes.length != 0;
  }

  private boolean hasSingleIndex(MapStorage.Index[] indexes)
  {
    return indexes != null && indexes.length == 1;
  }

  private View buildSingleMapView(String message, MapStorage.Index index, int option)
  {
    @SuppressLint("InflateParams") final View countryView = getActivity().getLayoutInflater().
        inflate(R.layout.dialog_download_single_item, null);
    ((TextView) countryView.findViewById(R.id.tv__title)).setText(MapStorage.INSTANCE.countryName(index));
    final String size = StringUtils.getFileSizeString(MapStorage.INSTANCE.countryRemoteSizeInBytes(index, option));
    UiUtils.setTextAndShow(((TextView) countryView.findViewById(R.id.tv__size)), size);
    UiUtils.setTextAndShow(((TextView) countryView.findViewById(R.id.tv__message)), message);
    return countryView;
  }

  private View buildMultipleMapView(String message)
  {
    @SuppressLint("InflateParams") final View countriesView = getActivity().getLayoutInflater().
        inflate(R.layout.dialog_download_multiple_items, null);
    UiUtils.setTextAndShow(((TextView) countriesView.findViewById(R.id.tv__message)), message);

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
    final List<Map<String, String>> groupData = new ArrayList<>();
    final List<List<Map<String, String>>> childData = new ArrayList<>();
    List<Map<String, String>> countries = null;
    if (hasIndex(mMissingCountries))
    {
      final Map<String, String> countriesGroup = new HashMap<>();
      countriesGroup.put(GROUP_NAME, getString(R.string.maps) + " (" + mMissingCountries.length + ") ");
      countriesGroup.put(GROUP_SIZE, StringUtils.getFileSizeString(getCountrySizesBytes(mMissingCountries, StorageOptions.MAP_OPTION_MAP_ONLY)));
      groupData.add(countriesGroup);

      countries = getCountryNames(mMissingCountries);
      childData.add(countries);
    }
    if (hasIndex(mMissingRoutes))
    {
      final Map<String, String> routesGroup = new HashMap<>();
      long size = 0;
      int routesCount = mMissingRoutes.length;
      final List<Map<String, String>> routes = getCountryNames(mMissingRoutes);
      if (countries != null)
      {
        routes.addAll(countries);
        size += getCountrySizesBytes(mMissingCountries, StorageOptions.MAP_OPTION_CAR_ROUTING);
        routesCount += mMissingCountries.length;
      }
      size += getCountrySizesBytes(mMissingRoutes, StorageOptions.MAP_OPTION_CAR_ROUTING);

      routesGroup.put(GROUP_NAME, getString(R.string.dialog_routing_routes_size) + " (" + routesCount + ") ");
      routesGroup.put(GROUP_SIZE, StringUtils.getFileSizeString(size));
      groupData.add(routesGroup);

      childData.add(routes);
    }

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

  private List<Map<String, String>> getCountryNames(MapStorage.Index[] indexes)
  {
    final List<Map<String, String>> countries = new ArrayList<>();
    for (MapStorage.Index index : indexes)
    {
      final Map<String, String> countryData = new HashMap<>();
      countryData.put(COUNTRY_NAME, MapStorage.INSTANCE.countryName(index));
      countries.add(countryData);
    }
    return countries;
  }

  private long getCountrySizesBytes(MapStorage.Index[] indexes, int option)
  {
    long total = 0;
    for (MapStorage.Index index : indexes)
      total += MapStorage.INSTANCE.countryRemoteSizeInBytes(index, option);
    return total;
  }
}
