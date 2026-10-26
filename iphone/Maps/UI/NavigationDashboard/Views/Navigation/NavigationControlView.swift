final class NavigationControlView: SolidTouchView {
  private enum Constants {
    static let buttonContentInset: CGFloat = 15
    static var metricFont: UIFont { .bold22.dynamic(maxSize: 26) }
    static var metricLegendFont: UIFont { .bold12.dynamic(maxSize: 16) }
  }

  @IBOutlet private var distanceLabel: UILabel!
  @IBOutlet private var distanceLegendLabel: UILabel!
  @IBOutlet private var distanceWithLegendLabel: UILabel!
  @IBOutlet private var progressView: UIView!
  @IBOutlet private var routingProgress: NSLayoutConstraint!
  @IBOutlet private var speedLabel: UILabel!
  @IBOutlet private var speedBackground: UIView!
  @IBOutlet private var speedLegendLabel: UILabel!
  @IBOutlet private var speedWithLegendLabel: UILabel!
  @IBOutlet private var timeLabel: UILabel!
  @IBOutlet private var timePageControl: UIPageControl!
  @IBOutlet private var toggleInfoButton: UIButton!
  @IBOutlet private var extendedView: UIView!

  @IBOutlet private var extendButton: UIButton! {
    didSet {
      setExtendButtonImage()
    }
  }

  @IBOutlet private var settingsButton: MWMButton!
  @IBOutlet private var stopButton: UIButton!

  @IBOutlet private var ttsButton: MWMButton! {
    didSet {
      ttsButton.setImage(UIImage.icVoiceOff, for: .normal)
      ttsButton.setImage(UIImage.icVoiceOn, for: .selected)
      ttsButton.setImage(UIImage.icVoiceOn, for: [.selected, .highlighted])
      onTTSStatusUpdated()
    }
  }

  @IBOutlet private var trackRecordingButton: MWMButton!

  private lazy var dimBackground: DimBackground = .init(mainView: self, tapAction: { [weak self] in
    self?.diminish()
  })

  weak var ownerView: UIView!
  weak var delegate: RouteNavigationControlsDelegate!

  private weak var navigationInfo: MWMNavigationDashboardEntity?
  private var extendedConstraint: NSLayoutConstraint!
  private var notExtendedConstraint: NSLayoutConstraint!
  private let diminishSelector = #selector(diminish)

  var isVisible = false {
    didSet {
      guard isVisible != oldValue else { return }
      accessibilityElementsHidden = !isVisible
      if isVisible {
        addView()
      } else {
        removeView()
      }
    }
  }

  private var isExtended = false {
    willSet {
      guard isExtended != newValue else { return }
      morphExtendButton()
    }
    didSet {
      guard isVisible, superview != nil, isExtended != oldValue else { return }

      extendButton.accessibilityLabel = L(isExtended ? "navigation_accessibility_hide_controls" : "navigation_accessibility_more_controls")
      dimBackground.setVisible(isExtended, completion: nil)
      extendedView.isHidden = !isExtended
      updateAccessibilityElements()
      superview!.animateConstraints(animations: { [weak self] in
        guard let self else { return }
        if self.isExtended {
          self.notExtendedConstraint.isActive = false
          self.extendedConstraint.isActive = true
        } else {
          self.extendedConstraint.isActive = false
          self.notExtendedConstraint.isActive = true
        }
      })
      if UIAccessibility.isVoiceOverRunning {
        UIAccessibility.post(notification: .layoutChanged, argument: isExtended ? settingsButton : extendButton)
      }
    }
  }

  private var metricsLabels: [UILabel] {
    [distanceLabel, distanceWithLegendLabel, speedLabel, speedWithLegendLabel, timeLabel]
  }

  private var metricsLegendLabels: [UILabel] {
    [distanceLegendLabel, speedLegendLabel]
  }

  private var lastViewportHeight: CGFloat = 0

  override func awakeFromNib() {
    super.awakeFromNib()

    for label in metricsLabels {
      configureMetricLabel(label, font: Constants.metricFont)
    }
    for label in metricsLegendLabels {
      configureMetricLabel(label, font: Constants.metricLegendFont)
    }
    updateLegendSize()

    configureButton(settingsButton)
    configureButton(ttsButton)
    configureButton(trackRecordingButton)
    configureStopButton(stopButton)

    distanceLabel.accessibilityLabel = L("navigation_accessibility_distance_remaining")
    distanceLabel.isAccessibilityElement = true
    distanceWithLegendLabel.accessibilityLabel = distanceLabel.accessibilityLabel
    distanceWithLegendLabel.isAccessibilityElement = true
    distanceLegendLabel.isAccessibilityElement = false
    speedLabel.accessibilityLabel = L("navigation_accessibility_current_speed")
    speedLabel.isAccessibilityElement = true
    speedWithLegendLabel.accessibilityLabel = speedLabel.accessibilityLabel
    speedWithLegendLabel.isAccessibilityElement = true
    speedLegendLabel.isAccessibilityElement = false
    timeLabel.isAccessibilityElement = false
    timePageControl.isAccessibilityElement = false
    extendButton.accessibilityLabel = L("navigation_accessibility_more_controls")
    settingsButton.accessibilityLabel = L("settings")
    ttsButton.accessibilityLabel = L("pref_tts_enable_title")
    stopButton.accessibilityLabel = L("navigation_stop_button")
    updateAccessibilityElements()

    MWMTextToSpeech.add(self)
  }

  private func configureMetricLabel(_ label: UILabel, font: UIFont) {
    label.font = font
    label.adjustsFontForContentSizeCategory = true
    label.clipsToBounds = true
    label.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
    label.configureSingleLineAutoScaling()
  }

  private func configureButton(_ button: UIButton) {
    button.imageView?.contentMode = .scaleAspectFit
    button.contentHorizontalAlignment = .fill
    button.contentVerticalAlignment = .fill
    button.contentEdgeInsets = UIEdgeInsets(top: Constants.buttonContentInset,
                                            left: Constants.buttonContentInset,
                                            bottom: Constants.buttonContentInset,
                                            right: Constants.buttonContentInset)
  }

  private func configureStopButton(_ button: UIButton) {
    button.titleLabel?.adjustsFontForContentSizeCategory = true
    button.titleLabel?.numberOfLines = 1
    button.setContentCompressionResistancePriority(.required, for: .vertical)
    button.contentEdgeInsets = UIEdgeInsets(top: 10, left: 22, bottom: 10, right: 22)
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateLegendSize()
    if let navigationInfo {
      onNavigationInfoUpdated(navigationInfo)
    }
    setNeedsLayout()
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    guard isVisible, superview != nil, frame.height != lastViewportHeight else { return }
    lastViewportHeight = frame.height
    updateVisibleViewportArea()
  }

  private func addView() {
    guard superview != ownerView, !ownerView.subviews.contains(self) else { return }
    ownerView.addSubview(self)

    let lg = ownerView.safeAreaLayoutGuide
    leadingAnchor.constraint(equalTo: lg.leadingAnchor).isActive = true
    trailingAnchor.constraint(equalTo: lg.trailingAnchor).isActive = true

    extendedConstraint = bottomAnchor.constraint(equalTo: ownerView.bottomAnchor)
    extendedConstraint.isActive = false

    notExtendedConstraint = progressView.bottomAnchor.constraint(equalTo: lg.bottomAnchor)
    notExtendedConstraint.isActive = true

    updateVisibleViewportArea()
  }

  private func removeView() {
    // Cancel diminish timer on remove, otherwise performSelector keeps the view alive ~5s after close, blocking deinit.
    cancelDiminishTimer()
    updateVisibleViewportArea(onClose: true)
    dimBackground.setVisible(false, completion: {
      self.removeFromSuperview()
    })
  }

  private func updateVisibleViewportArea(onClose: Bool = false) {
    let bottom = frame.height / 2
    let insets = UIEdgeInsets(top: 0, left: 0, bottom: onClose ? 0 : bottom, right: 0)
    MapViewController.shared()?.updateVisibleAreaInsets(for: self, insets: insets, updatingViewport: true)
  }

  private func updateLegendSize() {
    let isCompact = traitCollection.verticalSizeClass == .compact
    distanceLabel.isHidden = isCompact
    distanceLegendLabel.isHidden = isCompact
    distanceWithLegendLabel.isHidden = !isCompact
    speedLabel.isHidden = isCompact
    speedLegendLabel.isHidden = isCompact
    speedWithLegendLabel.isHidden = !isCompact

    let pgScale: CGFloat = isCompact ? 0.7 : 1
    timePageControl.transform = CGAffineTransform(scaleX: pgScale, y: pgScale)
    updateAccessibilityElements()
  }

  private func updateAccessibilityElements() {
    let isCompact = traitCollection.verticalSizeClass == .compact
    let speed: UILabel = isCompact ? speedWithLegendLabel : speedLabel
    let distance: UILabel = isCompact ? distanceWithLegendLabel : distanceLabel
    var elements: [Any] = [speed, toggleInfoButton, distance, extendButton]
    if isExtended {
      elements.append(settingsButton)
      if !ttsButton.isHidden {
        elements.append(ttsButton)
      }
      elements.append(trackRecordingButton)
      elements.append(stopButton)
    }
    accessibilityElements = elements
  }

  func setTrackRecordingState(_ state: TrackRecordingState) {
    trackRecordingButton.coloring = state == .active ? .red : .black
    trackRecordingButton.accessibilityLabel = state == .active ? L("track_recording_title") : L("start_track_recording")
  }

  func onNavigationInfoUpdated(_ info: MWMNavigationDashboardEntity) {
    navigationInfo = info
    guard isVisible else { return }
    let routingNumberAttributes: [NSAttributedString.Key: Any] =
      [
        NSAttributedString.Key.foregroundColor: UIColor.blackPrimaryText,
        NSAttributedString.Key.font: Constants.metricFont,
      ]
    let routingLegendAttributes: [NSAttributedString.Key: Any] =
      [
        NSAttributedString.Key.foregroundColor: UIColor.blackSecondaryText,
        NSAttributedString.Key.font: Constants.metricLegendFont,
      ]

    if timePageControl.currentPage == 0 {
      timeLabel.text = DurationFormatter.durationString(from: TimeInterval(info.timeToTarget))
      toggleInfoButton.accessibilityLabel = L("navigation_accessibility_time_remaining")
      toggleInfoButton.accessibilityHint = L("navigation_accessibility_show_arrival_time")
    } else {
      timeLabel.text = info.arrival
      toggleInfoButton.accessibilityLabel = L("navigation_accessibility_arrival_time")
      toggleInfoButton.accessibilityHint = L("navigation_accessibility_show_time_remaining")
    }
    toggleInfoButton.accessibilityValue = timeLabel.text

    var distanceWithLegend: NSMutableAttributedString?
    if let targetDistance = info.targetDistance {
      distanceLabel.text = targetDistance
      distanceWithLegend = NSMutableAttributedString(string: targetDistance, attributes: routingNumberAttributes)
    }

    if let targetUnits = info.targetUnits {
      distanceLegendLabel.text = targetUnits
      if let distanceWithLegend = distanceWithLegend {
        distanceWithLegend.append(NSAttributedString(string: targetUnits, attributes: routingLegendAttributes))
        distanceWithLegendLabel.attributedText = distanceWithLegend
      }
    }
    let distanceValue = [info.targetDistance, info.targetUnits].compactMap { $0?.isEmpty == false ? $0 : nil }.joined(separator: " ")
    distanceLabel.accessibilityValue = distanceValue
    distanceWithLegendLabel.accessibilityValue = distanceValue

    var speedMps = 0.0
    if let s = LocationManager.lastLocation()?.speed, s > 0 {
      speedMps = s
    }
    let speedMeasure = Measure(asSpeed: speedMps)
    var speed = speedMeasure.valueAsString

    // @todo Draw speed limit sign similar to the CarPlay implemenation.
    // speedLimitMps >= 0 means known limited speed.
    if info.speedLimitMps >= 0 {
      // Short delimeter to not overlap with timeToTarget longer than an hour.
      let delimeter = info.timeToTarget < 60 * 60 ? " / " : "/"
      let speedLimitMeasure = Measure(asSpeed: info.speedLimitMps)
      // speedLimitMps == 0 means unlimited speed.
      speed += delimeter + (info.speedLimitMps == 0 ? "∞" : speedLimitMeasure.valueAsString)
    }

    speedLabel.text = speed
    speedLegendLabel.text = speedMeasure.unit
    let speedWithLegend = NSMutableAttributedString(string: speed, attributes: routingNumberAttributes)
    speedWithLegend.append(NSAttributedString(string: speedMeasure.unit, attributes: routingLegendAttributes))
    speedWithLegendLabel.attributedText = speedWithLegend

    var speedValue = "\(speedMeasure.valueAsString) \(speedMeasure.unit)"
    if info.speedLimitMps >= 0 {
      let limit = info.speedLimitMps == 0
        ? L("navigation_accessibility_unlimited_speed")
        : "\(Measure(asSpeed: info.speedLimitMps).valueAsString) \(speedMeasure.unit)"
      speedValue += ", " + String(format: L("navigation_accessibility_speed_limit"), limit)
    }
    speedLabel.accessibilityValue = speedValue
    speedWithLegendLabel.accessibilityValue = speedValue

    if MWMRouter.isSpeedCamLimitExceeded() {
      speedLabel.textColor = .whitePrimary
      speedBackground.backgroundColor = .buttonRed
    } else {
      let isSpeedLimitExceeded = info.speedLimitMps > 0 && speedMps > info.speedLimitMps
      speedLabel.textColor = isSpeedLimitExceeded ? .buttonRed : .blackPrimaryText
      speedBackground.backgroundColor = UIColor.clear
    }
    speedLegendLabel.textColor = speedLabel.textColor
    speedWithLegendLabel.textColor = speedLabel.textColor

    routingProgress.constant = progressView.width * info.progress / 100
  }

  @IBAction
  private func toggleInfoAction() {
    if let navigationInfo = navigationInfo {
      timePageControl.currentPage = (timePageControl.currentPage + 1) % timePageControl.numberOfPages
      onNavigationInfoUpdated(navigationInfo)
    }
    refreshDiminishTimer()
  }

  @IBAction
  private func extendAction() {
    isExtended = !isExtended
    refreshDiminishTimer()
  }

  @IBAction
  private func ttsButtonAction(_: Any) {
    delegate.ttsButtonDidTap()
  }

  @IBAction
  private func settingsButtonAction(_: Any) {
    delegate.settingsButtonDidTap()
  }

  @IBAction
  private func stopRoutingButtonAction(_: Any) {
    delegate.stopRoutingButtonDidTap()
  }

  @IBAction
  private func trackRecordingButtonAction(_: Any) {
    delegate.trackRecordingButtonDidTap()
  }

  private func morphExtendButton() {
    guard let imageView = extendButton.imageView else { return }
    let morphImagesCount = 6
    let startValue = isExtended ? morphImagesCount : 1
    let endValue = isExtended ? 0 : morphImagesCount + 1
    let stepValue = isExtended ? -1 : 1
    var morphImages: [UIImage] = []
    for i in stride(from: startValue, to: endValue, by: stepValue) {
      morphImages.append(UIImage(named: "ic_menu_\(i)")!)
    }
    imageView.animationImages = morphImages
    imageView.animationRepeatCount = 1
    imageView.image = morphImages.last
    imageView.startAnimating()
    setExtendButtonImage()
  }

  private func setExtendButtonImage() {
    DispatchQueue.main.async {
      guard let imageView = self.extendButton.imageView else { return }
      if imageView.isAnimating {
        self.setExtendButtonImage()
      } else {
        self.extendButton.setImage(UIImage(resource: self.isExtended ? .icMenuDown : .icMenu), for: .normal)
      }
    }
  }

  private func refreshDiminishTimer() {
    cancelDiminishTimer()
    guard !UIAccessibility.isVoiceOverRunning else { return }
    perform(diminishSelector, with: self, afterDelay: 5)
  }

  private func cancelDiminishTimer() {
    NSObject.cancelPreviousPerformRequests(withTarget: self, selector: diminishSelector, object: self)
  }

  @objc
  private func diminish() {
    isExtended = false
  }

  override func applyTheme() {
    super.applyTheme()
    onTTSStatusUpdated()
    setTrackRecordingState(TrackRecordingManager.shared.recordingState)
  }

  override var sideButtonsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    .bottom
  }

  override var widgetsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    alternative(iPhone: .bottom, iPad: [])
  }

  override var trackRecordingButtonAreaAffectDirections: MWMAvailableAreaAffectDirections {
    .bottom
  }
}

extension NavigationControlView: MWMTextToSpeechObserver {
  func onTTSStatusUpdated() {
    guard MWMRouter.isRoutingActive() else { return }
    ttsButton.isHidden = !MWMTextToSpeech.isTTSEnabled()
    if !ttsButton.isHidden {
      ttsButton.isSelected = MWMTextToSpeech.tts().active
      ttsButton.accessibilityValue = L(ttsButton.isSelected ? "on" : "off")
    }
    if isVisible {
      updateAccessibilityElements()
    }
    refreshDiminishTimer()
  }
}
