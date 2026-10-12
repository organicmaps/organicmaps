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
    static let gpsColor = UIColor(red: 0x2E / 255, green: 0x9E / 255, blue: 0x48 / 255, alpha: 1)
    static let networkColor = UIColor(red: 0xEF / 255, green: 0x7D / 255, blue: 0x00 / 255, alpha: 1)
    static let manualColor = UIColor(red: 0x24 / 255, green: 0x9C / 255, blue: 0xF2 / 255, alpha: 1)
    static let noneColor = UIColor(red: 0xD3 / 255, green: 0x2F / 255, blue: 0x2F / 255, alpha: 1)
    static let inertialColor = UIColor(red: 0x7B / 255, green: 0x1F / 255, blue: 0xA2 / 255, alpha: 1)
  }

  private static weak var shared: NoGpsControls?
  private static var availableArea = CGRect.zero

  private weak var owner: MapViewController?
  private let statusButton = UIButton(type: .custom)
  private let buttons = UIStackView()
  private let shiftForwardButton = NoGpsControls.makeButton("chevron.up")
  private let shiftStepButton = UIButton(type: .system)
  private let shiftBackButton = NoGpsControls.makeButton("chevron.down")
  private let pauseButton = NoGpsControls.makeButton("pause.fill")
  private let reverseButton = NoGpsControls.makeButton("arrow.uturn.left")
  private let manualButton = NoGpsControls.makeButton("antenna.radiowaves.left.and.right")
  private var statusTop = NSLayoutConstraint()
  private var statusCenterX = NSLayoutConstraint()
  private var buttonsLeading = NSLayoutConstraint()
  private var buttonsBottom = NSLayoutConstraint()
  private var timer: Timer?

  @objc
  init(owner: MapViewController) {
    self.owner = owner
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
    statusButton.titleLabel?.numberOfLines = 2
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
    pauseButton.addTarget(self, action: #selector(togglePause), for: .touchUpInside)
    reverseButton.addTarget(self, action: #selector(reverseDirection), for: .touchUpInside)
    manualButton.addTarget(self, action: #selector(toggleManualMode), for: .touchUpInside)
    shiftForwardButton.accessibilityLabel = L("nogps_shift_forward")
    shiftBackButton.accessibilityLabel = L("nogps_shift_back")
    reverseButton.accessibilityLabel = L("nogps_reverse_direction")
    manualButton.accessibilityLabel = L("nogps_manual_position")

    buttons.axis = .vertical
    buttons.alignment = .center
    buttons.spacing = Constants.spacing
    buttons.translatesAutoresizingMaskIntoConstraints = false
    [shiftForwardButton, shiftStepButton, shiftBackButton, pauseButton, reverseButton, manualButton].forEach {
      buttons.addArrangedSubview($0)
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
    let (text, color) = sourceText(status)
    // The position is not taken from GPS or towers now, but the user has to see whether they work: in the manual mode
    // GPS is not used even when it is back.
    let title = NSMutableAttributedString(string: text, attributes: [.font: UIFont.boldSystemFont(ofSize: 15),
                                                                     .foregroundColor: UIColor.white])
    if status.manualMode || status.source == .inertial {
      title.append(NSAttributedString(string: "\n" + otherSourcesText(status),
                                      attributes: [.font: UIFont.boldSystemFont(ofSize: 12),
                                                   .foregroundColor: UIColor.white]))
    }
    statusButton.setAttributedTitle(title, for: .normal)
    statusButton.backgroundColor = color

    // The buttons moving the position along the road calibrate the speed, they are shown while the user calibrates it.
    shiftForwardButton.isHidden = !(status.movedByHand && status.shiftButtonsShown)
    shiftStepButton.isHidden = shiftForwardButton.isHidden
    shiftBackButton.isHidden = shiftForwardButton.isHidden
    shiftStepButton.setTitle(String(format: L("nogps_meters"), status.shiftStepM), for: .normal)
    // The position waits at a turn until the car leaves it, otherwise it would go to another street.
    shiftForwardButton.alpha = status.shiftForwardBlocked ? Constants.disabledAlpha : 1
    shiftBackButton.alpha = status.shiftBackBlocked ? Constants.disabledAlpha : 1
    reverseButton.isHidden = !status.movedByHand
    // The movement is paused only when it is calculated from the car speed.
    pauseButton.isHidden = !(status.movedByHand && status.inertialEnabled)
    pauseButton.setImage(UIImage(systemName: status.paused ? "play.fill" : "pause.fill"), for: .normal)
    pauseButton.accessibilityLabel = L(status.paused ? "nogps_resume" : "nogps_pause")
    // The icon tells where the position comes from now: from satellites or from the user.
    manualButton.setImage(UIImage(systemName: status.manualMode ? "hand.point.up.left.fill"
                                                                : "antenna.radiowaves.left.and.right"), for: .normal)
    updateLayout()
  }

  private func sourceText(_ status: NoGpsStatus) -> (String, UIColor) {
    switch status.source {
    case .gps:
      return (L("nogps_status_gps"), Constants.gpsColor)
    case .network:
      let key = status.gpsSpoofed ? "nogps_status_network_spoofed" : "nogps_status_network_no_gps"
      return (String(format: L(key), formatAccuracy(status.accuracyM)), Constants.networkColor)
    case .inertial:
      let text = status.paused ? L("nogps_status_paused")
                               : String(format: L("nogps_status_inertial"), formatAccuracy(status.accuracyM))
      return (text, Constants.inertialColor)
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

  private func formatAccuracy(_ meters: Double) -> String {
    if meters < 1000 {
      return String(format: L("nogps_meters"), Int(meters.rounded()))
    }
    return String(format: L("nogps_kilometers"), meters / 1000)
  }

  // MARK: - Actions

  @objc
  private func showSensors() {
    owner?.present(UINavigationController(rootViewController: NoGpsSensorsViewController()), animated: true)
  }

  @objc
  private func toggleManualMode() {
    let enable = !NoGps.isManualMode()
    NoGps.setManualMode(enable)
    Toast.show(withText: L(enable ? "nogps_manual_mode_on" : "nogps_manual_mode_off"))
    refresh()
  }

  @objc
  private func reverseDirection() {
    NoGps.reverseDirection()
  }

  @objc
  private func togglePause() {
    NoGps.togglePause()
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
    let key: String?
    switch event {
    // The buttons show the mode.
    case .manualModeChanged: key = nil
    case .gpsBack: key = "nogps_gps_back_auto"
    case .gpsLost: key = "nogps_gps_lost_auto"
    case .gpsSpoofed: key = "nogps_gps_spoofed"
    case .gpsRestored: key = "nogps_gps_restored"
    case .roadLost: key = "nogps_road_lost"
    case .turnsReversed: key = "nogps_turns_reversed"
    // The box is the only source of the car movement on iOS.
    case .motionSourceStopped: key = "nogps_box_stopped"
    // The car is always on a road.
    case .markNoRoad: key = "nogps_mark_no_road"
    @unknown default: key = nil
    }
    if let key {
      Toast.show(withText: L(key))
    }
    refresh()
  }
}
