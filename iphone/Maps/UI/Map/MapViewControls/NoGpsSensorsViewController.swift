/// The settings and the state of the inertial navigation by the ESP32 sensor box: the box, the speed, its
/// calibration, the calibration buttons and the test switch disabling GPS.
final class NoGpsSensorsViewController: MWMTableViewController {
  private enum Constants {
    static let refreshInterval: TimeInterval = 0.5
    static let cellId = "NoGpsSensorsCell"
  }

  private enum Row {
    case enabled
    case box
    case calibrate
    case gyro
    case speed
    case car
    case voltageMismatch
    case readiness
    case scale
    case clearSpeed
    case shiftButtons
    case disableGps
  }

  private struct Section {
    let rows: [Row]
    let footer: String?
  }

  private var sections: [Section] = []
  private var status = NoGps.status()
  private var timer: Timer?

  init() {
    super.init(style: .grouped)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    title = L("nogps_sensors_title")
    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(close))
    tableView.register(UITableViewCell.self, forCellReuseIdentifier: Constants.cellId)
    tableView.rowHeight = UITableView.automaticDimension
    tableView.estimatedRowHeight = 44
    refresh()
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    let timer = Timer(timeInterval: Constants.refreshInterval, repeats: true) { [weak self] _ in self?.refresh() }
    RunLoop.main.add(timer, forMode: .common)
    self.timer = timer
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    timer?.invalidate()
    timer = nil
  }

  private func refresh() {
    status = NoGps.status()
    var stateRows: [Row] = [.box, .calibrate, .gyro, .speed]
    if status.inertialStarted, status.hasCarInfo {
      stateRows.append(.car)
      if status.voltageMismatch {
        stateRows.append(.voltageMismatch)
      }
    }
    if status.inertialStarted {
      stateRows.append(.readiness)
    }
    sections = [
      Section(rows: [.enabled], footer: L("nogps_sensors_hint_box")),
      Section(rows: stateRows, footer: nil),
      Section(rows: [.scale, .clearSpeed], footer: nil),
      Section(rows: [.shiftButtons], footer: L("nogps_shift_buttons_hint")),
      Section(rows: [.disableGps], footer: L("nogps_disable_gps_hint")),
    ]
    tableView.reloadData()
  }

  // MARK: - UITableViewDataSource

  override func numberOfSections(in _: UITableView) -> Int {
    sections.count
  }

  override func tableView(_: UITableView, numberOfRowsInSection section: Int) -> Int {
    sections[section].rows.count
  }

  override func tableView(_: UITableView, titleForFooterInSection section: Int) -> String? {
    sections[section].footer
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(withIdentifier: Constants.cellId, for: indexPath)
    var content = cell.defaultContentConfiguration()
    content.textProperties.numberOfLines = 0
    cell.accessoryView = nil
    cell.selectionStyle = .none
    let row = sections[indexPath.section].rows[indexPath.row]
    switch row {
    case .enabled:
      content.text = L("nogps_sensors_enable")
      cell.accessoryView = makeSwitch(isOn: status.inertialEnabled, action: #selector(enabledChanged))
    case .box:
      let name = status.inertialStarted && !status.deviceName.isEmpty ? status.deviceName : "ESP32"
      let box = status.inertialStarted ? name + " · " + L(boxStateKey(status.sourceState)) : name
      content.text = String(format: L("nogps_sensors_box"), box)
    case .calibrate:
      content.text = L("nogps_sensors_calibrate_box")
      content.textProperties.color = status.inertialStarted ? .linkBlue : .blackHintText
      cell.selectionStyle = .default
    case .gyro:
      content.text = String(format: L("nogps_sensors_gyro"), gyroText())
    case .speed:
      let speed = status.inertialStarted ? status.speedKmh : -1
      content.text = String(format: L("nogps_sensors_speed"),
                            speed >= 0 ? String(format: L("nogps_speed_kmh"), speed) : L("nogps_unknown"))
    case .car:
      content.text = carText()
    case .voltageMismatch:
      content.text = L("nogps_voltage_mismatch")
      content.textProperties.color = .buttonRed
    case .readiness:
      let missing = missingParts()
      content.text = missing.isEmpty ? L("nogps_sensors_ready")
                                     : String(format: L("nogps_sensors_not_ready"), missing.joined(separator: ", "))
      content.textProperties.color = missing.isEmpty ? .linkBlue : .buttonRed
    case .scale:
      let scale = status.inertialStarted ? String(format: "×%.2f", status.speedScale) : L("nogps_unknown")
      var text = String(format: L("nogps_sensors_scale"), scale, status.inertialStarted ? status.speedTableRanges : 0)
      if status.inertialStarted {
        let key = status.speedLagMeasured ? "nogps_sensors_lag_measured" : "nogps_sensors_lag_usual"
        text += "\n" + String(format: L(key), String(format: "%.1f", status.speedLagSec))
      }
      content.text = text
    case .clearSpeed:
      content.text = L("nogps_clear_speed")
      content.textProperties.color = .linkBlue
      cell.selectionStyle = .default
    case .shiftButtons:
      content.text = L("nogps_shift_buttons")
      cell.accessoryView = makeSwitch(isOn: status.shiftButtonsShown, action: #selector(shiftButtonsChanged))
    case .disableGps:
      content.text = L("nogps_disable_gps")
      cell.accessoryView = makeSwitch(isOn: status.gpsDisabled, action: #selector(disableGpsChanged))
    }
    cell.contentConfiguration = content
    return cell
  }

  // MARK: - UITableViewDelegate

  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    switch sections[indexPath.section].rows[indexPath.row] {
    case .calibrate:
      NoGps.calibrate()
      refresh()
    case .clearSpeed:
      confirmClearSpeed()
    default:
      break
    }
  }

  // MARK: - Texts

  private func boxStateKey(_ state: NoGpsSourceState) -> String {
    switch state {
    case .disconnected: return "nogps_elm_disconnected"
    case .connecting: return "nogps_box_bt_searching"
    case .noAdapter: return "nogps_elm_no_adapter"
    case .obdDisabled: return "nogps_box_obd_disabled"
    case .obdConnecting: return "nogps_box_obd_connecting"
    case .obdError: return "nogps_elm_error"
    case .noCarData: return "nogps_elm_no_car"
    case .boxSleeping: return "nogps_box_sleeping"
    case .obdSleeping: return "nogps_box_obd_sleeping"
    case .connected: return "nogps_elm_connected"
    @unknown default: return "nogps_unknown"
    }
  }

  private func gyroText() -> String {
    switch status.inertialStarted ? status.calibration : .none {
    case .none: return L("nogps_gyro_none")
    case .calibrating: return String(format: L("nogps_gyro_calibrating"), status.calibrationProgress)
    case .done: return L("nogps_gyro_done")
    case .failedMoving: return L("nogps_gyro_failed")
    case .mountMoved: return L("nogps_gyro_mount_moved")
    @unknown default: return L("nogps_unknown")
    }
  }

  /// What the box tells about the car: the engine and the voltage measured by the box and by ELM327.
  private func carText() -> String {
    // The box measures its own voltage all the time and asks the car for the rest only while it stands, not to delay
    // the speed.
    let unknown = L(status.speedKmh > 0 ? "nogps_not_while_driving" : "nogps_unknown")
    let engine = status.engineRunning < 0 ? L("nogps_unknown")
                                          : L(status.engineRunning > 0 ? "nogps_engine_running" : "nogps_engine_stopped")
    let rpm = status.rpm >= 0 ? String(format: L("nogps_rpm"), status.rpm) : unknown
    let box = status.boxMillivolts >= 0 ? volts(status.boxMillivolts) : L("nogps_unknown")
    let elm = status.elmMillivolts >= 0 ? volts(status.elmMillivolts) : unknown
    let ecu = status.ecuMillivolts >= 0 ? volts(status.ecuMillivolts) : unknown
    return String(format: L("nogps_sensors_car"), engine, rpm, box, elm, ecu)
  }

  private func volts(_ millivolts: Int) -> String {
    String(format: L("nogps_volts"), String(format: "%.1f", Double(millivolts) / 1000))
  }

  private func missingParts() -> [String] {
    var missing: [String] = []
    if status.speedKmh < 0 {
      missing.append(L("nogps_missing_speed"))
    }
    if status.calibration != .done {
      missing.append(L("nogps_missing_gyro"))
    }
    if !status.hasInertialPosition {
      missing.append(L("nogps_missing_position"))
    }
    return missing
  }

  // MARK: - Actions

  private func makeSwitch(isOn: Bool, action: Selector) -> UISwitch {
    let control = UISwitch()
    control.isOn = isOn
    control.addTarget(self, action: action, for: .valueChanged)
    return control
  }

  @objc
  private func enabledChanged(_ sender: UISwitch) {
    NoGps.setInertialNavigationEnabled(sender.isOn)
    refresh()
  }

  @objc
  private func shiftButtonsChanged(_ sender: UISwitch) {
    NoGps.setShiftButtonsShown(sender.isOn)
  }

  @objc
  private func disableGpsChanged(_ sender: UISwitch) {
    NoGps.setGpsDisabled(sender.isOn)
  }

  private func confirmClearSpeed() {
    let alert = UIAlertController(title: L("nogps_clear_speed_title"),
                                  message: L("nogps_clear_speed_message"),
                                  preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: L("cancel"), style: .cancel))
    alert.addAction(UIAlertAction(title: L("nogps_clear_speed"), style: .destructive) { [weak self] _ in
      NoGps.clearSpeedCalibration()
      self?.refresh()
    })
    present(alert, animated: true)
  }

  @objc
  private func close() {
    dismiss(animated: true)
  }
}
