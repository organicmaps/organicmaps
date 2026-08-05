package app.organicmaps.settings;

import android.app.Activity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.widget.SwitchCompat;
import androidx.core.view.ViewCompat;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.sdk.routing.RouteSpeedSettings;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.settings.RoadType;
import app.organicmaps.sdk.settings.UnitLocale;
import app.organicmaps.util.WindowInsetUtils.PaddingInsetsListener;
import com.google.android.material.slider.Slider;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

public class DrivingOptionsFragment extends BaseMwmToolbarFragment
{
  public static final String BUNDLE_ROAD_TYPES = "road_types";
  private static final String BUNDLE_CRUISING_SPEED = "cruising_speed";
  private static final String BUNDLE_WIND_ENABLED = "wind_enabled";
  private static final String BUNDLE_WIND_SPEED = "wind_speed";
  private static final String BUNDLE_WIND_DIRECTION = "wind_direction";
  private static final double KMPH_TO_MPH = 0.621371192;
  @StringRes
  private static final int[] WIND_DIRECTION_LABELS = {
      R.string.route_wind_direction_n,  R.string.route_wind_direction_ne, R.string.route_wind_direction_e,
      R.string.route_wind_direction_se, R.string.route_wind_direction_s,  R.string.route_wind_direction_sw,
      R.string.route_wind_direction_w,  R.string.route_wind_direction_nw};
  @NonNull
  private Set<RoadType> mRoadTypes = Collections.emptySet();
  private View mContent;
  @Nullable
  private RouteSpeedSettings mSettings;
  private double mCruisingSpeedKmph;
  private boolean mWindEnabled;
  private int mWindSpeedMps;
  private int mWindDirectionDegrees;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_driving_options, container, false);
    mSettings = RouteSpeedSettings.nativeGet();
    if (mSettings != null)
    {
      mCruisingSpeedKmph = mSettings.cruisingSpeedKmph;
      mWindEnabled = mSettings.windSpeedMps > 0;
      mWindSpeedMps = mWindEnabled ? mSettings.windSpeedMps : RouteSpeedSettings.DEFAULT_WIND_SPEED_MPS;
      mWindDirectionDegrees = mSettings.windDirectionDegrees;
      if (savedInstanceState != null)
      {
        mCruisingSpeedKmph = savedInstanceState.getDouble(BUNDLE_CRUISING_SPEED, mCruisingSpeedKmph);
        mWindEnabled = savedInstanceState.getBoolean(BUNDLE_WIND_ENABLED, mWindEnabled);
        mWindSpeedMps = savedInstanceState.getInt(BUNDLE_WIND_SPEED, mWindSpeedMps);
        mWindDirectionDegrees = savedInstanceState.getInt(BUNDLE_WIND_DIRECTION, mWindDirectionDegrees);
      }
    }
    initViews(root);
    ViewCompat.setOnApplyWindowInsetsListener(mContent, new PaddingInsetsListener(false, true, true, true));
    mRoadTypes = savedInstanceState != null && savedInstanceState.containsKey(BUNDLE_ROAD_TYPES)
                   ? makeRouteTypes(savedInstanceState)
                   : RoutingOptions.getActiveRoadTypes();
    return root;
  }

  @NonNull
  private Set<RoadType> makeRouteTypes(@NonNull Bundle bundle)
  {
    Set<RoadType> result = new HashSet<>();
    List<Integer> items = Objects.requireNonNull(bundle.getIntegerArrayList(BUNDLE_ROAD_TYPES));
    for (Integer each : items)
    {
      result.add(RoadType.values()[each]);
    }
    return result;
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
    ArrayList<Integer> savedRoadTypes = new ArrayList<>();
    for (RoadType each : mRoadTypes)
    {
      savedRoadTypes.add(each.ordinal());
    }
    outState.putIntegerArrayList(BUNDLE_ROAD_TYPES, savedRoadTypes);
    outState.putDouble(BUNDLE_CRUISING_SPEED, mCruisingSpeedKmph);
    outState.putBoolean(BUNDLE_WIND_ENABLED, mWindEnabled);
    outState.putInt(BUNDLE_WIND_SPEED, mWindSpeedMps);
    outState.putInt(BUNDLE_WIND_DIRECTION, mWindDirectionDegrees);
  }

  private int windSpeedMps()
  {
    return mWindEnabled ? mWindSpeedMps : 0;
  }

  private boolean isRouteSpeedChanged()
  {
    if (mSettings == null)
      return false;
    if (mCruisingSpeedKmph != mSettings.cruisingSpeedKmph || windSpeedMps() != mSettings.windSpeedMps)
      return true;
    return mWindEnabled && mWindDirectionDegrees != mSettings.windDirectionDegrees;
  }

  private boolean areSettingsNotChanged()
  {
    return mRoadTypes.equals(RoutingOptions.getActiveRoadTypes()) && !isRouteSpeedChanged();
  }

  @Override
  public boolean onBackPressed()
  {
    if (areSettingsNotChanged())
    {
      requireActivity().setResult(Activity.RESULT_CANCELED);
    }
    else
    {
      if (isRouteSpeedChanged())
        RouteSpeedSettings.nativeSet(mCruisingSpeedKmph, windSpeedMps(), mWindDirectionDegrees);
      requireActivity().setResult(Activity.RESULT_OK);
    }

    return super.onBackPressed();
  }

  private void initViews(@NonNull View root)
  {
    mContent = root.findViewById(R.id.content);

    initRoadTypeSwitch(root, R.id.avoid_tolls_btn, RoadType.Toll);
    initRoadTypeSwitch(root, R.id.avoid_dirty_roads_btn, RoadType.Dirty);
    initRoadTypeSwitch(root, R.id.avoid_ferries_btn, RoadType.Ferry);
    initRoadTypeSwitch(root, R.id.avoid_motorways_btn, RoadType.Motorway);

    View speedOptions = root.findViewById(R.id.route_speed_options);
    if (mSettings == null)
    {
      speedOptions.setVisibility(View.GONE);
      return;
    }
    speedOptions.setVisibility(View.VISIBLE);

    TextView speedValue = root.findViewById(R.id.route_speed_value);
    Slider speedSlider = root.findViewById(R.id.route_speed_slider);
    speedSlider.setValueFrom((float) mSettings.minSpeedKmph);
    speedSlider.setValueTo((float) mSettings.maxSpeedKmph);
    speedSlider.setStepSize((float) mSettings.speedStepKmph);
    speedSlider.setValue((float) mCruisingSpeedKmph);
    speedValue.setText(formatCruisingSpeed(mCruisingSpeedKmph));
    speedSlider.addOnChangeListener((slider, value, fromUser) -> {
      mCruisingSpeedKmph = value;
      speedValue.setText(formatCruisingSpeed(mCruisingSpeedKmph));
    });

    View windOptions = root.findViewById(R.id.route_wind_options);
    if (!mSettings.isWindSupported())
    {
      windOptions.setVisibility(View.GONE);
      return;
    }
    windOptions.setVisibility(View.VISIBLE);

    View windInputs = root.findViewById(R.id.route_wind_inputs);
    SwitchCompat windEnabled = root.findViewById(R.id.route_wind_enabled);
    windEnabled.setChecked(mWindEnabled);
    windInputs.setVisibility(mWindEnabled ? View.VISIBLE : View.GONE);
    windEnabled.setOnCheckedChangeListener((button, isChecked) -> {
      mWindEnabled = isChecked;
      windInputs.setVisibility(isChecked ? View.VISIBLE : View.GONE);
    });

    TextView windSpeedValue = root.findViewById(R.id.route_wind_speed_value);
    Slider windSpeedSlider = root.findViewById(R.id.route_wind_speed_slider);
    windSpeedSlider.setValueTo(mSettings.maxWindSpeedMps);
    windSpeedSlider.setValue(mWindSpeedMps);
    windSpeedValue.setText(formatWindSpeed(mWindSpeedMps));
    windSpeedSlider.addOnChangeListener((slider, value, fromUser) -> {
      mWindSpeedMps = Math.round(value);
      windSpeedValue.setText(formatWindSpeed(mWindSpeedMps));
    });

    TextView windDirectionValue = root.findViewById(R.id.route_wind_direction_value);
    Slider windDirectionSlider = root.findViewById(R.id.route_wind_direction_slider);
    windDirectionSlider.setValue(mWindDirectionDegrees);
    windDirectionValue.setText(formatWindDirection(mWindDirectionDegrees));
    windDirectionSlider.addOnChangeListener((slider, value, fromUser) -> {
      mWindDirectionDegrees = Math.round(value);
      windDirectionValue.setText(formatWindDirection(mWindDirectionDegrees));
    });
  }

  private static void initRoadTypeSwitch(@NonNull View root, @IdRes int id, @NonNull RoadType roadType)
  {
    SwitchCompat button = root.findViewById(id);
    button.setChecked(RoutingOptions.hasOption(roadType));
    button.setOnCheckedChangeListener((buttonView, isChecked) -> {
      if (isChecked)
        RoutingOptions.addOption(roadType);
      else
        RoutingOptions.removeOption(roadType);
    });
  }

  @NonNull
  private String formatCruisingSpeed(double speedKmph)
  {
    // The native speed formatter rounds to whole units, too coarse for the half a km/h steps of the
    // pedestrian slider.
    boolean imperial = UnitLocale.getUnits() == UnitLocale.UNITS_FOOT;
    @StringRes
    int units = imperial ? R.string.miles_per_hour : R.string.kilometers_per_hour;
    NumberFormat numberFormat = NumberFormat.getNumberInstance();
    numberFormat.setMaximumFractionDigits(1);
    return numberFormat.format(imperial ? speedKmph * KMPH_TO_MPH : speedKmph) + "\u00a0" + getString(units);
  }

  @NonNull
  private String formatWindSpeed(int speedMps)
  {
    if (UnitLocale.getUnits() == UnitLocale.UNITS_FOOT)
      return Math.round(speedMps * 3.6 * KMPH_TO_MPH) + "\u00a0" + getString(R.string.miles_per_hour);
    return speedMps + "\u00a0" + getString(R.string.route_wind_speed_unit_mps);
  }

  @NonNull
  private String formatWindDirection(int directionDegrees)
  {
    int labelIndex = directionDegrees / RouteSpeedSettings.WIND_DIRECTION_STEP_DEGREES;
    return getString(WIND_DIRECTION_LABELS[labelIndex]) + " · " + directionDegrees + "°";
  }
}
