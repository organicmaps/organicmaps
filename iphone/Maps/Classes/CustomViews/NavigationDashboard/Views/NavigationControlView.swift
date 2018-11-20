@objc(MWMNavigationControlView)
final class NavigationControlView: SolidTouchView, MWMTextToSpeechObserver, MWMTrafficManagerObserver {
  @IBOutlet private weak var distanceLabel: UILabel!
  @IBOutlet private weak var distanceLegendLabel: UILabel!
  @IBOutlet private weak var distanceWithLegendLabel: UILabel!
  @IBOutlet private weak var progressView: UIView!
  @IBOutlet private weak var routingProgress: NSLayoutConstraint!
  @IBOutlet private weak var speedLabel: UILabel!
  @IBOutlet private weak var speedBackground: UIView!
  @IBOutlet private weak var speedLegendLabel: UILabel!
  @IBOutlet private weak var speedWithLegendLabel: UILabel!
  @IBOutlet private weak var timeLabel: UILabel!
  @IBOutlet private weak var timePageControl: UIPageControl!

  @IBOutlet private weak var extendButton: UIButton! {
    didSet {
      setExtendButtonImage()
    }
  }

  @IBOutlet private weak var ttsButton: UIButton! {
    didSet {
      ttsButton.setImage(#imageLiteral(resourceName: "ic_voice_off"), for: .normal)
      ttsButton.setImage(#imageLiteral(resourceName: "ic_voice_on"), for: .selected)
      ttsButton.setImage(#imageLiteral(resourceName: "ic_voice_on"), for: [.selected, .highlighted])
      onTTSStatusUpdated()
    }
  }

  @IBOutlet private weak var trafficButton: UIButton! {
    didSet {
      trafficButton.setImage(#imageLiteral(resourceName: "ic_setting_traffic_off"), for: .normal)
      trafficButton.setImage(#imageLiteral(resourceName: "ic_setting_traffic_on"), for: .selected)
      trafficButton.setImage(#imageLiteral(resourceName: "ic_setting_traffic_on"), for: [.selected, .highlighted])
      onTrafficStateUpdated()
    }
  }

  private lazy var dimBackground: DimBackground = {
    DimBackground(mainView: self, tapAction: { [weak self] in
      self?.diminish()
    })
  }()

  @objc weak var ownerView: UIView!
  @IBOutlet private weak var extendedView: UIView!

  private weak var navigationInfo: MWMNavigationDashboardEntity?

  private var hiddenConstraint: NSLayoutConstraint!
  private var extendedConstraint: NSLayoutConstraint!
  @objc var isVisible = false {
    didSet {
      guard isVisible != oldValue else { return }
      if isVisible {
        addView()
      } else {
        removeView()
      }
      alpha = isVisible ? 0 : 1
      DispatchQueue.main.async {
        self.superview?.animateConstraints {
          self.alpha = self.isVisible ? 1 : 0
          self.hiddenConstraint.isActive = !self.isVisible
        }
      }
    }
  }

  private var isExtended = false {
    willSet {
      guard isExtended != newValue else { return }
      morphExtendButton()
    }
    didSet {
      guard isVisible && superview != nil else { return }
      guard isExtended != oldValue else { return }

      dimBackground.setVisible(isExtended, completion: nil)
      extendedView.isHidden = !isExtended
      superview!.animateConstraints(animations: {
        self.extendedConstraint.isActive = self.isExtended
      })
    }
  }

  private func addView() {
    guard superview != ownerView else { return }
    ownerView.addSubview(self)

    var lAnchor = ownerView.leadingAnchor
    var tAnchor = ownerView.trailingAnchor
    var bAnchor = ownerView.bottomAnchor
    if #available(iOS 11.0, *) {
      let layoutGuide = ownerView.safeAreaLayoutGuide
      lAnchor = layoutGuide.leadingAnchor
      tAnchor = layoutGuide.trailingAnchor
      bAnchor = layoutGuide.bottomAnchor
    }

    leadingAnchor.constraint(equalTo: lAnchor).isActive = true
    trailingAnchor.constraint(equalTo: tAnchor).isActive = true

    hiddenConstraint = topAnchor.constraint(equalTo: ownerView.bottomAnchor)
    hiddenConstraint.isActive = true

    let visibleConstraint = progressView.bottomAnchor.constraint(equalTo: bAnchor)
    visibleConstraint.priority = UILayoutPriority.defaultLow
    visibleConstraint.isActive = true

    extendedConstraint = bottomAnchor.constraint(equalTo: bAnchor)
    extendedConstraint.priority = UILayoutPriority(rawValue: UILayoutPriority.RawValue(Int(UILayoutPriority.defaultHigh.rawValue) - 1))

    ownerView.layoutIfNeeded()
  }

  private func removeView() {
    dimBackground.setVisible(false, completion: {
      self.removeFromSuperview()
    })
  }

  override func mwm_refreshUI() {
    if isVisible {
      super.mwm_refreshUI()
    }
  }

  override func awakeFromNib() {
    super.awakeFromNib()
    translatesAutoresizingMaskIntoConstraints = false

    MWMTextToSpeech.add(self)
    MWMTrafficManager.add(self)
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    let isCompact = traitCollection.verticalSizeClass == .compact
    distanceLabel.isHidden = isCompact
    distanceLegendLabel.isHidden = isCompact
    distanceWithLegendLabel.isHidden = !isCompact
    speedLabel.isHidden = isCompact
    speedLegendLabel.isHidden = isCompact
    speedWithLegendLabel.isHidden = !isCompact

    let pgScale: CGFloat = isCompact ? 0.7 : 1
    timePageControl.transform = CGAffineTransform(scaleX: pgScale, y: pgScale)
  }

  @objc func onNavigationInfoUpdated(_ info: MWMNavigationDashboardEntity) {
    navigationInfo = info
    guard !MWMRouter.isTaxi() else { return }

    let routingNumberAttributes: [NSAttributedStringKey: Any] =
      [
        NSAttributedStringKey.foregroundColor: UIColor.blackPrimaryText(),
        NSAttributedStringKey.font: UIFont.bold24(),
      ]
    let routingLegendAttributes: [NSAttributedStringKey: Any] =
      [
        NSAttributedStringKey.foregroundColor: UIColor.blackSecondaryText(),
        NSAttributedStringKey.font: UIFont.bold14(),
      ]

    if timePageControl.currentPage == 0 {
      timeLabel.text = info.eta
    } else {
      timeLabel.text = info.arrival
    }

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

    let speed = info.speed ?? "0"
    speedLabel.text = speed
    speedLegendLabel.text = info.speedUnits
    let speedWithLegend = NSMutableAttributedString(string: speed, attributes: routingNumberAttributes)
    speedWithLegend.append(NSAttributedString(string: info.speedUnits, attributes: routingLegendAttributes))
    speedWithLegendLabel.attributedText = speedWithLegend

    let speedLimitExceeded = info.isSpeedLimitExceeded
    let textColor = speedLimitExceeded ? UIColor.white() : UIColor.blackPrimaryText()
    speedBackground.backgroundColor = speedLimitExceeded ? UIColor.buttonRed() : UIColor.clear
    speedLabel.textColor = textColor
    speedLegendLabel.textColor = textColor
    speedWithLegendLabel.textColor = textColor

    self.routingProgress.constant = self.progressView.width * info.progress / 100
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

  private func morphExtendButton() {
    guard let imageView = extendButton.imageView else { return }
    let morphImagesCount = 6
    let startValue = isExtended ? morphImagesCount : 1
    let endValue = isExtended ? 0 : morphImagesCount + 1
    let stepValue = isExtended ? -1 : 1
    var morphImages: [UIImage] = []
    let nightMode = UIColor.isNightMode() ? "dark" : "light"
    for i in stride(from: startValue, to: endValue, by: stepValue) {
      let imageName = "ic_menu_\(i)_\(nightMode)"
      morphImages.append(UIImage(named: imageName)!)
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
        self.extendButton.setImage(self.isExtended ? #imageLiteral(resourceName: "ic_menu_down") : #imageLiteral(resourceName: "ic_menu"), for: .normal)
      }
    }
  }

  private func refreshDiminishTimer() {
    let sel = #selector(diminish)
    NSObject.cancelPreviousPerformRequests(withTarget: self, selector: sel, object: self)
    perform(sel, with: self, afterDelay: 5)
  }

  @objc
  private func diminish() {
    isExtended = false
  }

  func onTTSStatusUpdated() {
    guard MWMRouter.isRoutingActive() else { return }
    let isPedestrianRouting = MWMRouter.type() == .pedestrian
    ttsButton.isHidden = isPedestrianRouting || !MWMTextToSpeech.isTTSEnabled()
    if !ttsButton.isHidden {
      ttsButton.isSelected = MWMTextToSpeech.tts().active
    }
    refreshDiminishTimer()
  }

  func onTrafficStateUpdated() {
    guard MWMRouter.isRoutingActive() else { return }
    let isPedestrianRouting = MWMRouter.type() == .pedestrian
    trafficButton.isHidden = isPedestrianRouting
    trafficButton.isSelected = MWMTrafficManager.trafficState() != .disabled
    refreshDiminishTimer()
  }

  override var sideButtonsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return .bottom
  }

  override var widgetsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return alternative(iPhone: .bottom, iPad: []) }
}
