/// The controls of the navigation without GPS on the map: where the position comes from, the manual mode and the
/// buttons correcting the position of the car. They are laid out in the free area of the side buttons.
@objc
final class NoGpsControls: NSObject {
  private enum Constants {
    static let buttonSize = CGFloat(48)
    static let spacing = CGFloat(8)
    static let sideOffset = CGFloat(10)
    // The scale ruler of the map is below the buttons.
    static let bottomOffset = CGFloat(40)
    static let statusTopOffset = CGFloat(8)
    static let statusMinHeight = CGFloat(36)
    static let statusMaxWidth = CGFloat(260)
    static let refreshInterval: TimeInterval = 1
    static let disabledAlpha = CGFloat(0.4)
    static let blinkKey = "blink"
    static let blinkDuration: CFTimeInterval = 0.4
    static let blinkOpacity = Float(0.2)
    static let gpsColor = UIColor(red: 0x2E / 255, green: 0x9E / 255, blue: 0x48 / 255, alpha: 1)
    static let networkColor = UIColor(red: 0xEF / 255, green: 0x7D / 255, blue: 0x00 / 255, alpha: 1)
    static let manualColor = UIColor(red: 0x24 / 255, green: 0x9C / 255, blue: 0xF2 / 255, alpha: 1)
    static let noneColor = UIColor(red: 0xD3 / 255, green: 0x2F / 255, blue: 0x2F / 255, alpha: 1)
    static let inertialColor = UIColor(red: 0x7B / 255, green: 0x1F / 255, blue: 0xA2 / 255, alpha: 1)
  }

  private weak static var shared: NoGpsControls?
  private static var availableArea = CGRect.zero

  private let statusButton = UIButton(type: .custom)
  private let buttons = UIStackView()
  private let shiftForwardButton = NoGpsControls.makeButton("chevron.up")
  private let shiftStepButton = UIButton(type: .system)
  private let shiftBackButton = NoGpsControls.makeButton("chevron.down")
  private let reverseButton = NoGpsControls.makeButton("arrow.uturn.left")
  private let switchMarkButton = NoGpsControls.makeButton("arrow.left.arrow.right")
  private let manualButton = NoGpsControls.makeButton("hand.point.up.left")
  private var statusTop = NSLayoutConstraint()
  private var statusCenterX = NSLayoutConstraint()
  private var buttonsLeading = NSLayoutConstraint()
  private var buttonsBottom = NSLayoutConstraint()
  private var timer: Timer?
  private weak var calibrationAlert: UIAlertController?
  private weak var firmwareAlert: UIAlertController?

  @objc
  init(owner: MapViewController) {
    super.init()
    Self.shared = self
    setupViews(in: owner.controlsView)
    refresh()
    let timer = Timer(timeInterval: Constants.refreshInterval, repeats: true) { [weak self] _ in self?.refresh() }
    RunLoop.main.add(timer, forMode: .common)
    self.timer = timer
    NotificationCenter.default.addObserver(self, selector: #selector(onEvent), name: NoGps.eventNotification, object: nil)
  }

  deinit {
    timer?.invalidate()
  }

  static func updateAvailableArea(_ frame: CGRect) {
    availableArea = frame
    DispatchQueue.main.async {
      shared?.updateLayout()
    }
  }

  // MARK: - Views

  private static func makeButton(_ symbol: String) -> BottomTabBarButton {
    let button = BottomTabBarButton()
    button.setStyleAndApply(.bottomTabBarButton)
    button.translatesAutoresizingMaskIntoConstraints = false
    button.setImage(UIImage(systemName: symbol), for: .normal)
    NSLayoutConstraint.activate([
      button.widthAnchor.constraint(equalToConstant: Constants.buttonSize),
      button.heightAnchor.constraint(equalToConstant: Constants.buttonSize),
    ])
    return button
  }

  private func setupViews(in superview: UIView) {
    statusButton.translatesAutoresizingMaskIntoConstraints = false
    statusButton.layer.cornerRadius = Constants.statusMinHeight / 2
    statusButton.layer.shadowColor = UIColor.black.cgColor
    statusButton.layer.shadowOpacity = 0.25
    statusButton.layer.shadowOffset = CGSize(width: 0, height: 1)
    statusButton.contentEdgeInsets = UIEdgeInsets(top: 3, left: 14, bottom: 3, right: 14)
    statusButton.titleLabel?.numberOfLines = 3
    statusButton.titleLabel?.textAlignment = .center
    statusButton.titleLabel?.lineBreakMode = .byTruncatingTail
    statusButton.addTarget(self, action: #selector(showSensors), for: .touchUpInside)
    superview.addSubview(statusButton)

    shiftStepButton.translatesAutoresizingMaskIntoConstraints = false
    shiftStepButton.titleLabel?.font = .boldSystemFont(ofSize: 14)
    shiftStepButton.setTitleColor(.blackPrimaryText, for: .normal)
    shiftStepButton.backgroundColor = .white
    shiftStepButton.layer.cornerRadius = Constants.spacing
    shiftStepButton.widthAnchor.constraint(equalToConstant: Constants.buttonSize).isActive = true
    shiftStepButton.addTarget(self, action: #selector(cycleShiftStep), for: .touchUpInside)

    shiftForwardButton.addTarget(self, action: #selector(shiftForward), for: .touchUpInside)
    shiftBackButton.addTarget(self, action: #selector(shiftBack), for: .touchUpInside)
    reverseButton.addTarget(self, action: #selector(reverseDirection), for: .touchUpInside)
    switchMarkButton.addTarget(self, action: #selector(switchMark), for: .touchUpInside)
    manualButton.addTarget(self, action: #selector(toggleMarkByTap), for: .touchUpInside)
    shiftForwardButton.accessibilityLabel = L("nogps_shift_forward")
    shiftBackButton.accessibilityLabel = L("nogps_shift_back")
    reverseButton.accessibilityLabel = L("nogps_reverse_direction")
    switchMarkButton.accessibilityLabel = L("nogps_switch_mark")
    manualButton.accessibilityLabel = L("nogps_manual_position")

    buttons.axis = .vertical
    buttons.alignment = .center
    buttons.spacing = Constants.spacing
    buttons.translatesAutoresizingMaskIntoConstraints = false
    for item in [shiftForwardButton, shiftStepButton, shiftBackButton, reverseButton, switchMarkButton, manualButton] {
      buttons.addArrangedSubview(item)
    }
    superview.addSubview(buttons)

    statusTop = statusButton.topAnchor.constraint(equalTo: superview.topAnchor)
    statusCenterX = statusButton.centerXAnchor.constraint(equalTo: superview.leftAnchor)
    buttonsLeading = buttons.leftAnchor.constraint(equalTo: superview.leftAnchor)
    buttonsBottom = buttons.bottomAnchor.constraint(equalTo: superview.topAnchor)
    NSLayoutConstraint.activate([
      statusTop, statusCenterX, buttonsLeading, buttonsBottom,
      statusButton.heightAnchor.constraint(greaterThanOrEqualToConstant: Constants.statusMinHeight),
      statusButton.widthAnchor.constraint(lessThanOrEqualToConstant: Constants.statusMaxWidth),
    ])
    updateLayout()
  }

  private func updateLayout() {
    let area = Self.availableArea
    guard !area.isEmpty else { return }
    statusTop.constant = area.minY + Constants.statusTopOffset
    statusCenterX.constant = area.midX
    buttonsLeading.constant = area.minX + Constants.sideOffset
    buttonsBottom.constant = area.maxY - Constants.bottomOffset
    // The buttons are hidden when a panel leaves no place for them.
    buttons.alpha = buttons.systemLayoutSizeFitting(UIView.layoutFittingCompressedSize).height
      + Constants.statusMinHeight + Constants.bottomOffset + 2 * Constants.spacing > area.height ? 0 : 1
  }

  // MARK: - State

  private func refresh() {
    let status = NoGps.status()
    var (text, color) = sourceText(status)
    var details = [String]()
    if status.roadLost {
      // The mark stands and waits for the user: a toast about it is easy to miss while driving.
      text = L("nogps_status_road_lost")
      color = Constants.noneColor
      details.append(L("nogps_status_road_lost_hint"))
    } else if status.manualMode || status.source == .inertial {
      // The position is not taken from GPS or towers now, but the user has to see whether they work: in the manual
      // mode GPS is not used even when it is back.
      details.append(otherSourcesText(status))
    }
    // The panel of a followed route tells whether the car speed comes, nothing does without a route.
    if !MWMRouter.isOnRoute(), let link = linkText(status) {
      details.append(link)
    }
    let title = NSMutableAttributedString(string: text, attributes: [.font: UIFont.boldSystemFont(ofSize: 15),
                                                                     .foregroundColor: UIColor.white])
    for line in details {
      title.append(NSAttributedString(string: "\n" + line, attributes: [.font: UIFont.boldSystemFont(ofSize: 12),
                                                                        .foregroundColor: UIColor.white]))
    }
    statusButton.setAttributedTitle(title, for: .normal)
    statusButton.backgroundColor = color
    blinkStatus(status.roadLost)

    // The buttons moving the position along the road calibrate the speed, they are shown while the user calibrates it.
    shiftForwardButton.isHidden = !(status.movedByHand && status.shiftButtonsShown)
    shiftStepButton.isHidden = shiftForwardButton.isHidden
    shiftBackButton.isHidden = shiftForwardButton.isHidden
    shiftStepButton.setTitle(String(format: L("nogps_meters"), status.shiftStepM), for: .normal)
    // The position waits at a turn until the car leaves it, otherwise it would go to another street.
    shiftForwardButton.alpha = status.shiftForwardBlocked ? Constants.disabledAlpha : 1
    shiftBackButton.alpha = status.shiftBackBlocked ? Constants.disabledAlpha : 1
    reverseButton.isHidden = !status.movedByHand
    switchMarkButton.isHidden = !(status.movedByHand && status.otherMarks > 0)
    // The car is marked by hand only while the position is not from GPS. The button is red while the next tap on the
    // map shows where the car is, the tap is taken back when GPS is used again.
    let canMark = status.source != .gps
    if !canMark, status.markByTap {
      NoGps.setMarkByTap(false)
    }
    manualButton.isHidden = !canMark
    manualButton.tintColor = canMark && status.markByTap ? Constants.noneColor : nil
    updateLayout()
  }

  private func sourceText(_ status: NoGpsStatus) -> (String, UIColor) {
    switch status.source {
    case .gps:
      return (L("nogps_status_gps"), Constants.gpsColor)
    case .network:
      let format = L(status.gpsSpoofed ? "nogps_status_network_spoofed" : "nogps_status_network_no_gps")
      return (String(format: format, formatAccuracy(status.accuracyM)), Constants.networkColor)
    case .inertial:
      return (String(format: L("nogps_status_inertial"), formatAccuracy(status.accuracyM)), Constants.inertialColor)
    case .manual:
      let minutes = Int(status.manualAge / 60)
      let text = minutes == 0 ? L("nogps_status_manual_just_now") : String(format: L("nogps_status_manual_minutes"), minutes)
      return (text, Constants.manualColor)
    case .none:
      return (L("nogps_status_none"), Constants.noneColor)
    @unknown default:
      return (L("nogps_status_none"), Constants.noneColor)
    }
  }

  private func otherSourcesText(_ status: NoGpsStatus) -> String {
    if status.workingGpsAccuracyM >= 0 {
      return String(format: L("nogps_status_gps_back"), formatAccuracy(status.workingGpsAccuracyM))
    }
    let gps = L(status.gpsSpoofed ? "nogps_status_gps_spoofed" : "nogps_status_no_gps")
    guard status.workingNetworkAccuracyM >= 0 else { return gps }
    return gps + " · " + String(format: L("nogps_status_towers"), formatAccuracy(status.workingNetworkAccuracyM))
  }

  /// Whether the car speed comes from the sensor box, the only source of it on iOS.
  private func linkText(_ status: NoGpsStatus) -> String? {
    guard status.inertialEnabled, status.inertialStarted else { return nil }
    switch status.sourceState {
    case .connected: return L("nogps_status_box_ok")
    case .disconnected, .connecting, .boxSleeping: return L("nogps_status_box_none")
    case .updating: return L("nogps_box_updating")
    default: return L("nogps_status_box_no_car")
    }
  }

  private func blinkStatus(_ blink: Bool) {
    // The system removes the animation in the background, the next refresh adds it again.
    let layer = statusButton.layer
    guard blink != (layer.animation(forKey: Constants.blinkKey) != nil) else { return }
    guard blink else {
      layer.removeAnimation(forKey: Constants.blinkKey)
      return
    }
    let animation = CABasicAnimation(keyPath: "opacity")
    animation.fromValue = 1
    animation.toValue = Constants.blinkOpacity
    animation.duration = Constants.blinkDuration
    animation.autoreverses = true
    animation.repeatCount = .infinity
    layer.add(animation, forKey: Constants.blinkKey)
  }

  private func formatAccuracy(_ meters: Double) -> String {
    if meters < 1000 {
      return String(format: L("nogps_meters"), Int(meters.rounded()))
    }
    return String(format: L("nogps_kilometers"), meters / 1000)
  }

  // MARK: - Actions

  @objc
  private func showSensors() {
    // The calibration alert may come while the screen is open, under another alert too.
    let top = UIViewController.topViewController()
    var shown: UIViewController? = top
    while let controller = shown {
      if (controller as? UINavigationController)?.viewControllers.first is NoGpsSensorsViewController {
        return
      }
      shown = controller.presentingViewController
    }
    top.present(UINavigationController(rootViewController: NoGpsSensorsViewController()), animated: true)
  }

  /// The sensor box has been plugged in: asks the user to calibrate it, the car should stand on a level place.
  private func offerBoxCalibration() {
    // As on Android, not asked in the background: the car may be driving when the user comes back. The sensors
    // screen still tells to calibrate the box. The box may be powered on again before the user answers.
    guard UIApplication.shared.applicationState == .active, calibrationAlert == nil else { return }
    let alert = UIAlertController(title: L("nogps_box_calibration"),
                                  message: L("nogps_box_calibration_offer"),
                                  preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: L("later"), style: .cancel))
    alert.addAction(UIAlertAction(title: L("nogps_sensors_calibrate_box"), style: .default) { [weak self] _ in
      NoGps.calibrate()
      // It shows how the calibration goes.
      self?.showSensors()
    })
    UIViewController.topViewController().present(alert, animated: true)
    calibrationAlert = alert
  }

  /// The box works with another firmware than the app is tested with: asks the user to install the one of the app.
  private func offerFirmwareUpdate() {
    // As the calibration, not asked in the background: the car may be driving when the user comes back. The sensors
    // screen still offers the firmware.
    let status = NoGps.status()
    guard UIApplication.shared.applicationState == .active, firmwareAlert == nil, !status.bundledFirmware.isEmpty
    else { return }
    let alert = UIAlertController(title: L("nogps_firmware_title"),
                                  message: String(format: L("nogps_firmware_offer"), status.boxFirmware,
                                                  status.bundledFirmware),
                                  preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: L("later"), style: .cancel))
    alert.addAction(UIAlertAction(title: String(format: L("nogps_firmware_install"), status.bundledFirmware),
                                  style: .default) { [weak self] _ in
        NoGps.startFirmwareUpdate()
        // It shows how the update goes.
        self?.showSensors()
      })
    UIViewController.topViewController().present(alert, animated: true)
    firmwareAlert = alert
  }

  private func firmwareErrorText(_ code: String) -> String {
    switch code {
    case "MOVING": return L("nogps_firmware_error_moving")
    case "LOW_VOLTAGE": return L("nogps_firmware_error_voltage")
    case "LINK": return L("nogps_firmware_error_link")
    case "CANCELED", "ABORTED": return L("nogps_firmware_error_canceled")
    case "ROLLBACK": return L("nogps_firmware_error_rollback")
    case "PENDING": return L("nogps_firmware_error_pending")
    default: return String(format: L("nogps_firmware_error_code"), code)
    }
  }

  @objc
  private func toggleMarkByTap() {
    let armed = !NoGps.status().markByTap
    NoGps.setMarkByTap(armed)
    if armed {
      Toast.show(withText: L("nogps_mark_by_tap"))
    }
    refresh()
  }

  @objc
  private func reverseDirection() {
    NoGps.reverseDirection()
  }

  @objc
  private func switchMark() {
    NoGps.switchMark()
    refresh()
  }

  @objc
  private func cycleShiftStep() {
    NoGps.cycleShiftStep()
    refresh()
  }

  @objc
  private func shiftForward() {
    shift(forward: true)
  }

  @objc
  private func shiftBack() {
    shift(forward: false)
  }

  /// Moves the position along the road when it lags behind the car or has run ahead of it.
  private func shift(forward: Bool) {
    let status = NoGps.status()
    let distanceM = forward ? status.shiftStepM : -status.shiftStepM
    // The position stands at a crossing, it is moved that way after the car passes it.
    if forward ? status.shiftForwardBlocked : status.shiftBackBlocked {
      Toast.show(withText: L("nogps_shift_at_turn"))
      return
    }
    let applied = NoGps.shiftPosition(Double(distanceM))
    let after = NoGps.status()
    if applied == 0, forward ? after.shiftForwardBlocked : after.shiftBackBlocked {
      Toast.show(withText: L("nogps_shift_at_turn"))
    } else if Int(abs(applied).rounded()) < abs(distanceM) {
      // The position stops at the closest turn, so the user has to know when it was moved by less.
      let appliedM = Int(abs(applied).rounded())
      Toast.show(withText: appliedM == 0 ? L("nogps_shift_failed") : String(format: L("nogps_shift_limited"), appliedM))
    }
    refresh()
  }

  @objc
  private func onEvent(_ notification: Notification) {
    guard let raw = notification.userInfo?[NoGps.eventKey] as? Int, let event = NoGpsEvent(rawValue: raw) else { return }
    let text: String?
    switch event {
    // The buttons show the mode.
    case .manualModeChanged: text = nil
    case .gpsBack: text = L("nogps_gps_back_auto")
    case .gpsLost: text = L("nogps_gps_lost_auto")
    case .gpsSpoofed: text = L("nogps_gps_spoofed")
    case .gpsRestored: text = L("nogps_gps_restored")
    case .roadLost: text = L("nogps_road_lost")
    case .turnsReversed: text = L("nogps_turns_reversed")
    // The box is the only source of the car movement on iOS.
    case .motionSourceStopped: text = L("nogps_box_stopped")
    // The car is always on a road.
    case .markNoRoad: text = L("nogps_mark_no_road")
    case .notCalibrated:
      if NoGps.status().calibration == .mountMoved {
        text = L("nogps_box_moved_stopped")
      } else {
        text = L("nogps_box_uncalibrated_stopped")
      }
    case .boxCalibrationAdvised:
      offerBoxCalibration()
      text = nil
    case .firmwareUpdateAvailable:
      offerFirmwareUpdate()
      text = nil
    case .firmwareUpdateDone: text = String(format: L("nogps_firmware_done"), NoGps.status().boxFirmware)
    case .firmwareUpdateFailed:
      text = String(format: L("nogps_firmware_failed"), firmwareErrorText(NoGps.status().firmwareUpdateError))
    @unknown default: text = nil
    }
    if let text {
      Toast.show(withText: text)
    }
    refresh()
  }
}
