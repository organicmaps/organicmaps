package app.organicmaps.nogps;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.SwitchCompat;
import androidx.core.content.ContextCompat;
import app.organicmaps.BuildConfig;
import app.organicmaps.R;
import app.organicmaps.sdk.location.NoGps;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * Inertial navigation settings and state: the ELM327 adapter, speed, gyroscope zeroing and the car direction.
 */
public class SensorsBottomSheet extends BottomSheetDialogFragment
{
  public static final String TAG = SensorsBottomSheet.class.getSimpleName();
  private static final long UPDATE_INTERVAL_MS = 500;

  private final Handler mHandler = new Handler(Looper.getMainLooper());
  private final Runnable mUpdater = new Runnable() {
    @Override
    public void run()
    {
      update();
      mHandler.postDelayed(this, UPDATE_INTERVAL_MS);
    }
  };

  private final ActivityResultLauncher<String> mBluetoothPermissionLauncher =
      registerForActivityResult(new ActivityResultContracts.RequestPermission(), this::onBluetoothPermissionResult);
  // What to do after the Bluetooth permission is granted.
  @Nullable
  private Runnable mAfterPermission;
  private final ActivityResultLauncher<String[]> mBoxBluetoothLauncher =
      registerForActivityResult(new ActivityResultContracts.RequestMultiplePermissions(), result -> update());
  // The user has been asked while the sheet is shown.
  private boolean mBoxBluetoothAsked;

  private SwitchCompat mSwitch;
  private TextView mAdapter;
  private View mFirmwareRow;
  private TextView mFirmware;
  private TextView mFirmwareButton;
  private View mBoxCalibrationTitle;
  private View mBoxCalibrationHint;
  private TextView mSpeed;
  private TextView mCar;
  private TextView mVoltageMismatch;
  private TextView mScale;
  private TextView mLag;
  private TextView mGyro;
  private TextView mReadiness;
  private TextView mHint;
  private View mChooseAdapter;
  private TextView mCalibrate;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    final View view = inflater.inflate(R.layout.nogps_sensors_sheet, container, false);
    mSwitch = view.findViewById(R.id.nogps_inertial_switch);
    mAdapter = view.findViewById(R.id.nogps_adapter);
    mFirmwareRow = view.findViewById(R.id.nogps_firmware_row);
    mFirmware = view.findViewById(R.id.nogps_firmware);
    mFirmwareButton = view.findViewById(R.id.nogps_firmware_button);
    mFirmwareButton.setOnClickListener(v -> onFirmwareButton());
    mSpeed = view.findViewById(R.id.nogps_speed);
    mCar = view.findViewById(R.id.nogps_car);
    mVoltageMismatch = view.findViewById(R.id.nogps_voltage_mismatch);
    mScale = view.findViewById(R.id.nogps_scale);
    mLag = view.findViewById(R.id.nogps_lag);
    view.findViewById(R.id.nogps_clear_speed).setOnClickListener(v -> confirmClearSpeed());
    mGyro = view.findViewById(R.id.nogps_gyro);
    mBoxCalibrationTitle = view.findViewById(R.id.nogps_box_calibration_title);
    mBoxCalibrationHint = view.findViewById(R.id.nogps_box_calibration_hint);
    mReadiness = view.findViewById(R.id.nogps_readiness);
    mHint = view.findViewById(R.id.nogps_hint);
    mChooseAdapter = view.findViewById(R.id.nogps_choose_adapter);
    mCalibrate = view.findViewById(R.id.nogps_calibrate);

    final RadioGroup source = view.findViewById(R.id.nogps_source);
    final NoGps.Status status = NoGps.getStatus();
    source.check(status.esp32Source ? R.id.nogps_source_esp32_bt : R.id.nogps_source_phone);
    source.setOnCheckedChangeListener((group, checkedId) -> onSourceChosen(checkedId));

    mSwitch.setChecked(status.inertialEnabled);
    mSwitch.setOnCheckedChangeListener((v, isChecked) -> onSwitch(isChecked));
    final SwitchCompat shiftButtons = view.findViewById(R.id.nogps_shift_buttons_switch);
    shiftButtons.setChecked(status.shiftButtonsShown);
    shiftButtons.setOnCheckedChangeListener((v, isChecked) -> NoGps.nativeSetShiftButtonsShown(isChecked));

    final SwitchCompat disableGps = view.findViewById(R.id.nogps_disable_gps_switch);
    disableGps.setChecked(status.gpsDisabled);
    disableGps.setOnCheckedChangeListener((v, isChecked) -> NoGps.nativeSetGpsDisabled(isChecked));
    mChooseAdapter.setOnClickListener(v -> withBluetoothPermission(this::chooseAdapter));
    mCalibrate.setOnClickListener(v -> NoGps.nativeCalibrate());
    return view;
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mHandler.post(mUpdater);
    final NoGps.Status status = NoGps.getStatus();
    // Once: the user may have refused.
    if (status.inertialEnabled && status.esp32Source && !mBoxBluetoothAsked)
      askBoxBluetooth();
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mHandler.removeCallbacks(mUpdater);
  }

  private void onSourceChosen(int radioId)
  {
    if (radioId != R.id.nogps_source_phone)
    {
      NoGps.nativeSetEsp32Source(true);
      if (NoGps.getStatus().inertialEnabled)
        askBoxBluetooth();
      update();
      return;
    }
    withBluetoothPermission(() -> {
      NoGps.nativeSetEsp32Source(false);
      if (NoGps.getStatus().elm327Address.isEmpty() && NoGps.getStatus().inertialEnabled)
        chooseAdapter();
      update();
    });
  }

  private void onSwitch(boolean enabled)
  {
    final NoGps.Status status = NoGps.getStatus();
    if (!enabled || status.esp32Source)
    {
      NoGps.nativeSetInertialNavigationEnabled(enabled);
      if (enabled)
        askBoxBluetooth();
      update();
      return;
    }
    withBluetoothPermission(() -> {
      NoGps.nativeSetInertialNavigationEnabled(true);
      if (NoGps.getStatus().elm327Address.isEmpty())
        chooseAdapter();
      update();
    });
  }

  /**
   * The sensor box is searched and connected over Bluetooth LE. Without the permission its state tells so.
   */
  private void askBoxBluetooth()
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S)
      return;
    final String[] permissions = {Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT};
    for (String permission : permissions)
    {
      if (ContextCompat.checkSelfPermission(requireContext(), permission) == PackageManager.PERMISSION_GRANTED)
        continue;
      mBoxBluetoothAsked = true;
      mBoxBluetoothLauncher.launch(permissions);
      return;
    }
  }

  private void withBluetoothPermission(@NonNull Runnable action)
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S
        || ContextCompat.checkSelfPermission(requireContext(), Manifest.permission.BLUETOOTH_CONNECT)
               == PackageManager.PERMISSION_GRANTED)
    {
      action.run();
      return;
    }
    mAfterPermission = action;
    mBluetoothPermissionLauncher.launch(Manifest.permission.BLUETOOTH_CONNECT);
  }

  private void onBluetoothPermissionResult(boolean granted)
  {
    final Runnable action = mAfterPermission;
    mAfterPermission = null;
    if (granted && action != null)
    {
      action.run();
      return;
    }
    Toast.makeText(requireContext(), R.string.nogps_bt_permission_denied, Toast.LENGTH_LONG).show();
    mSwitch.setChecked(NoGps.getStatus().inertialEnabled);
  }

  @SuppressLint("MissingPermission") // Checked in withBluetoothPermission().
  private void chooseAdapter()
  {
    final BluetoothAdapter bluetooth = BluetoothAdapter.getDefaultAdapter();
    final List<BluetoothDevice> devices =
        bluetooth == null ? new ArrayList<>() : new ArrayList<>(bluetooth.getBondedDevices());
    if (devices.isEmpty())
    {
      Toast.makeText(requireContext(), R.string.nogps_no_paired, Toast.LENGTH_LONG).show();
      return;
    }
    final String[] names = new String[devices.size()];
    for (int i = 0; i < devices.size(); i++)
    {
      final String name = devices.get(i).getName();
      names[i] = (name != null ? name : "?") + "  (" + devices.get(i).getAddress() + ")";
    }
    new MaterialAlertDialogBuilder(requireContext())
        .setTitle(R.string.nogps_choose_adapter)
        .setItems(names, (dialog, which) -> NoGps.nativeSetElm327Address(devices.get(which).getAddress()))
        .show();
  }

  @SuppressLint("MissingPermission")
  @NonNull
  private String adapterName()
  {
    final String address = NoGps.getStatus().elm327Address;
    if (address.isEmpty())
      return getString(R.string.nogps_sensors_adapter_none);
    final BluetoothAdapter bluetooth = BluetoothAdapter.getDefaultAdapter();
    try
    {
      if (bluetooth != null)
      {
        final String name = bluetooth.getRemoteDevice(address).getName();
        if (name != null)
          return name;
      }
    }
    catch (SecurityException e)
    {
      // No Bluetooth permission, show the address.
    }
    return address;
  }

  private void update()
  {
    if (getContext() == null)
      return;
    final NoGps.Status status = NoGps.getStatus();
    final boolean enabled = status.inertialStarted;

    final boolean esp32 = status.esp32Source;
    mHint.setText(esp32 ? R.string.nogps_sensors_hint_box : R.string.nogps_sensors_hint);
    mChooseAdapter.setVisibility(esp32 ? View.GONE : View.VISIBLE);
    mCalibrate.setText(esp32 ? R.string.nogps_sensors_calibrate_box : R.string.nogps_sensors_calibrate);
    if (esp32)
    {
      String box = enabled && !status.deviceName.isEmpty() ? status.deviceName : "ESP32";
      if (enabled)
        box += " · " + getString(boxStateText(status));
      mAdapter.setText(getString(R.string.nogps_sensors_box, box));
    }
    else
    {
      String adapter = adapterName();
      if (enabled)
        adapter += " · " + getString(elmStateText(status.getSourceState()));
      mAdapter.setText(getString(R.string.nogps_sensors_adapter, adapter));
    }

    updateFirmware(enabled && esp32 ? status : null);

    final int speed = enabled ? status.speedKmh : -1;
    mSpeed.setText(getString(R.string.nogps_sensors_speed, speed >= 0 ? getString(R.string.nogps_speed_kmh, speed)
                                                                      : getString(R.string.nogps_unknown)));

    updateCar(enabled && esp32 && status.hasCarInfo ? status : null, speed);

    String scale = getString(
        R.string.nogps_sensors_scale,
        enabled ? String.format(Locale.US, "\u00D7%.2f", status.speedScale) : getString(R.string.nogps_unknown),
        enabled ? status.speedTableRanges : 0);
    mScale.setText(scale);
    // On its own line: next to the button it took four lines of the two the row has room for.
    mLag.setVisibility(enabled ? View.VISIBLE : View.GONE);
    if (enabled)
    {
      mLag.setText(
          getString(status.speedLagMeasured ? R.string.nogps_sensors_lag_measured : R.string.nogps_sensors_lag_usual,
                    String.format(Locale.US, "%.1f", status.speedLagSec)));
    }

    final NoGps.CalibrationState calibration = enabled ? status.getCalibration() : NoGps.CalibrationState.NONE;
    // The gyroscope of the phone is only zeroed. The box is calibrated by the user when it is put into the car:
    // a block of its own tells how.
    mBoxCalibrationTitle.setVisibility(esp32 ? View.VISIBLE : View.GONE);
    mBoxCalibrationHint.setVisibility(esp32 ? View.VISIBLE : View.GONE);
    if (esp32)
    {
      mGyro.setText(getString(R.string.nogps_sensors_box_sensors, boxCalibrationText(status, calibration)));
    }
    else
    {
      final String gyro = switch (calibration)
      {
        case NONE -> getString(R.string.nogps_gyro_none);
        case CALIBRATING -> getString(R.string.nogps_gyro_calibrating, status.calibrationProgress);
        case DONE -> getString(R.string.nogps_gyro_done);
        case FAILED_MOVING -> getString(R.string.nogps_gyro_failed);
        case MOUNT_MOVED -> getString(R.string.nogps_gyro_mount_moved);
      };
      mGyro.setText(getString(R.string.nogps_sensors_gyro, gyro));
    }

    if (!enabled)
    {
      mReadiness.setText("");
      return;
    }
    final List<String> missing = new ArrayList<>();
    if (speed < 0)
      missing.add(getString(R.string.nogps_missing_speed));
    if (calibration != NoGps.CalibrationState.DONE)
      missing.add(getString(esp32 ? R.string.nogps_missing_box_calibration : R.string.nogps_missing_gyro));
    if (!status.hasInertialPosition)
      missing.add(getString(R.string.nogps_missing_position));
    if (missing.isEmpty())
    {
      mReadiness.setText(R.string.nogps_sensors_ready);
      mReadiness.setTextColor(ContextCompat.getColor(requireContext(), R.color.nogps_status_gps));
    }
    else
    {
      mReadiness.setText(getString(R.string.nogps_sensors_not_ready, String.join(", ", missing)));
      mReadiness.setTextColor(ContextCompat.getColor(requireContext(), R.color.nogps_status_none));
    }
  }

  @NonNull
  private String boxCalibrationText(@NonNull NoGps.Status status, @NonNull NoGps.CalibrationState calibration)
  {
    return switch (calibration)
    {
      case NONE -> getString(R.string.nogps_box_not_calibrated);
      case CALIBRATING -> getString(R.string.nogps_box_calibrating, status.calibrationProgress);
      // The box keeps its calibration, but it may stand in another way since it was plugged in.
      case DONE ->
        getString(status.boxCalibrationAdvised ? R.string.nogps_box_calibration_advised
                                               : R.string.nogps_box_calibrated);
      case FAILED_MOVING -> getString(R.string.nogps_box_calibration_failed);
      case MOUNT_MOVED -> getString(R.string.nogps_gyro_mount_moved);
    };
  }

  /**
   * Shows the firmware of the sensor box, the button to send it the one of the application, or how that goes.
   * @param status null if the box is not used.
   */
  private void updateFirmware(@Nullable NoGps.Status status)
  {
    final NoGps.FirmwareUpdate update = status != null ? status.getFirmwareUpdate() : NoGps.FirmwareUpdate.NONE;
    final boolean updating = isUpdating(update);
    if (status == null || (!updating && status.boxFirmware.isEmpty()))
    {
      mFirmwareRow.setVisibility(View.GONE);
      return;
    }
    mFirmwareRow.setVisibility(View.VISIBLE);
    final String state = switch (update)
    {
      case STARTING -> getString(R.string.nogps_firmware_starting);
      case SENDING -> getString(R.string.nogps_firmware_sending, status.firmwareUpdateProgress);
      case VERIFYING -> getString(R.string.nogps_firmware_verifying);
      case RESTARTING -> getString(R.string.nogps_firmware_restarting);
      case NONE, DONE, FAILED -> status.boxFirmware;
    };
    mFirmware.setText(getString(R.string.nogps_sensors_firmware, state));

    // The box works with the firmware the application is tested with: there is nothing to install. A debug
    // build sends it again to try the update itself.
    final boolean install = !status.bundledFirmware.isEmpty()
                         && (BuildConfig.DEBUG || !status.bundledFirmware.equals(status.boxFirmware));
    // A box that has taken the firmware restarts by itself.
    final boolean cancel = updating && update != NoGps.FirmwareUpdate.RESTARTING;
    mFirmwareButton.setVisibility(install || cancel ? View.VISIBLE : View.GONE);
    if (cancel)
      mFirmwareButton.setText(R.string.cancel);
    else if (install)
      mFirmwareButton.setText(getString(R.string.nogps_firmware_install, status.bundledFirmware));
  }

  private static boolean isUpdating(@NonNull NoGps.FirmwareUpdate update)
  {
    return update == NoGps.FirmwareUpdate.STARTING || update == NoGps.FirmwareUpdate.SENDING
        || update == NoGps.FirmwareUpdate.VERIFYING || update == NoGps.FirmwareUpdate.RESTARTING;
  }

  private void onFirmwareButton()
  {
    if (isUpdating(NoGps.getStatus().getFirmwareUpdate()))
      NoGps.nativeCancelFirmwareUpdate();
    else
      NoGps.nativeStartFirmwareUpdate();
    update();
  }

  /**
   * Shows what the sensor box tells about the car: the engine and the voltage measured by the box and by ELM327.
   * @param car the status with the car data, null if the box tells nothing.
   */
  private void updateCar(@Nullable NoGps.Status car, int speedKmh)
  {
    mCar.setVisibility(car != null ? View.VISIBLE : View.GONE);
    mVoltageMismatch.setVisibility(car != null && car.voltageMismatch ? View.VISIBLE : View.GONE);
    if (car == null)
      return;

    // The box measures its own voltage all the time and asks the car for the rest only while it stands, not to
    // delay the speed.
    final String unknown = getString(speedKmh > 0 ? R.string.nogps_not_while_driving : R.string.nogps_unknown);
    final String engine = getString(car.engineRunning < 0   ? R.string.nogps_unknown
                                    : car.engineRunning > 0 ? R.string.nogps_engine_running
                                                            : R.string.nogps_engine_stopped);
    final String rpm = car.rpm >= 0 ? getString(R.string.nogps_rpm, car.rpm) : unknown;
    final String boxVolts = car.boxMillivolts >= 0 ? formatVolts(car.boxMillivolts) : getString(R.string.nogps_unknown);
    final String elmVolts = car.elmMillivolts >= 0 ? formatVolts(car.elmMillivolts) : unknown;
    final String ecuVolts = car.ecuMillivolts >= 0 ? formatVolts(car.ecuMillivolts) : unknown;
    mCar.setText(getString(R.string.nogps_sensors_car, engine, rpm, boxVolts, elmVolts, ecuVolts));
  }

  @NonNull
  private String formatVolts(int millivolts)
  {
    return getString(R.string.nogps_volts, String.format(Locale.US, "%.1f", millivolts / 1000.0));
  }

  private void confirmClearSpeed()
  {
    new MaterialAlertDialogBuilder(requireContext())
        .setTitle(R.string.nogps_clear_speed_title)
        .setMessage(R.string.nogps_clear_speed_message)
        .setPositiveButton(R.string.nogps_clear_speed,
                           (dialog, which) -> {
                             NoGps.nativeClearSpeedCalibration();
                             update();
                           })
        .setNegativeButton(R.string.cancel, null)
        .show();
  }

  private static int boxStateText(@NonNull NoGps.Status status)
  {
    return switch (status.getSourceState())
    {
      case DISCONNECTED -> R.string.nogps_elm_disconnected;
      case CONNECTING -> boxSearchText(status);
      case NO_ADAPTER -> R.string.nogps_elm_no_adapter;
      case OBD_DISABLED -> R.string.nogps_box_obd_disabled;
      case OBD_CONNECTING -> R.string.nogps_box_obd_connecting;
      case OBD_ERROR -> R.string.nogps_elm_error;
      case NO_CAR_DATA -> R.string.nogps_elm_no_car;
      case BOX_SLEEPING -> R.string.nogps_box_sleeping;
      case OBD_SLEEPING -> R.string.nogps_box_obd_sleeping;
      case CONNECTED -> R.string.nogps_elm_connected;
      case UPDATING -> R.string.nogps_box_updating;
    };
  }

  /**
   * The box is not heard yet: what the phone does to reach it.
   */
  private static int boxSearchText(@NonNull NoGps.Status status)
  {
    return switch (status.getBleState())
    {
      case NO_PERMISSION -> R.string.nogps_box_bt_no_permission;
      // OFF: the phone has no Bluetooth LE.
      case OFF, DISABLED -> R.string.nogps_box_bt_disabled;
      case SEARCHING -> R.string.nogps_box_bt_searching;
      case PAIRING_CLOSED -> R.string.nogps_box_bt_pairing_closed;
      case CONNECTING -> R.string.nogps_box_bt_connecting;
      case PAIRING -> R.string.nogps_box_bt_pairing;
      case CONNECTED -> R.string.nogps_box_bt_waiting;
    };
  }

  private static int elmStateText(@NonNull NoGps.SourceState state)
  {
    return switch (state)
    {
      case DISCONNECTED -> R.string.nogps_elm_disconnected;
      case CONNECTING -> R.string.nogps_elm_connecting;
      case NO_ADAPTER, OBD_DISABLED -> R.string.nogps_elm_no_adapter;
      case OBD_CONNECTING -> R.string.nogps_elm_connecting;
      case OBD_ERROR -> R.string.nogps_elm_error;
      // The last one is of the sensor box only.
      case NO_CAR_DATA, BOX_SLEEPING, OBD_SLEEPING, UPDATING -> R.string.nogps_elm_no_car;
      case CONNECTED -> R.string.nogps_elm_connected;
    };
  }
}
