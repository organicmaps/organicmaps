@objc(MWMNavigationControlView)
final class NavigationControlView: SolidTouchView, MWMTextToSpeechObserver, MWMTrafficManagerObserver {
  @IBOutlet private weak var distanceLabel: UILabel!
  @IBOutlet private weak var distanceLegendLabel: UILabel!
  @IBOutlet private weak var distanceWithLegendLabel: UILabel!
  @IBOutlet private weak var progressView: UIView!
  @IBOutlet private weak var routingProgress: NSLayoutConstraint!
  @IBOutlet private weak var speedLabel: UILabel!
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
    DimBackground(mainView: self)
  }()

  @objc weak var ownerView: UIView!

  private weak var navigationInfo: MWMNavigationDashboardEntity?

  private var hiddenConstraint: NSLayoutConstraint!
  private var extendedConstraint: NSLayoutConstraint!
  @objc var isVisible = false {
    didSet {
      guard isVisible != oldValue else { return }
      if isVisible {
        addView()
      } else {
        dimBackground.setVisible(false) {}
      }
      DispatchQueue.main.async {
        self.superview?.setNeedsLayout()
        self.hiddenConstraint.isActive = !self.isVisible
        UIView.animate(withDuration: kDefaultAnimationDuration,
                       animations: { self.superview?.layoutIfNeeded() },
                       completion: { _ in
                         if !self.isVisible {
                           self.removeFromSuperview()
                         }
        })
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

      superview?.setNeedsLayout()
      extendedConstraint.isActive = isExtended
      UIView.animate(withDuration: kDefaultAnimationDuration) { self.superview?.layoutIfNeeded() }

      dimBackground.setVisible(isExtended) { [weak self] in
        self?.diminish()
      }
    }
  }

  private func addView() {
    guard superview != ownerView else { return }
    ownerView.addSubview(self)

    NSLayoutConstraint(item: self, attribute: .left, relatedBy: .equal, toItem: ownerView, attribute: .left, multiplier: 1, constant: 0).isActive = true
    NSLayoutConstraint(item: self, attribute: .right, relatedBy: .equal, toItem: ownerView, attribute: .right, multiplier: 1, constant: 0).isActive = true

    hiddenConstraint = NSLayoutConstraint(item: self, attribute: .top, relatedBy: .equal, toItem: ownerView, attribute: .bottom, multiplier: 1, constant: 0)
    hiddenConstraint.priority = UILayoutPriority.defaultHigh
    hiddenConstraint.isActive = true

    let visibleConstraint = NSLayoutConstraint(item: progressView, attribute: .bottom, relatedBy: .equal, toItem: ownerView, attribute: .bottom, multiplier: 1, constant: 0)
    visibleConstraint.priority = UILayoutPriority.defaultLow
    visibleConstraint.isActive = true

    extendedConstraint = NSLayoutConstraint(item: self, attribute: .bottom, relatedBy: .equal, toItem: ownerView, attribute: .bottom, multiplier: 1, constant: 0)
    extendedConstraint.priority = UILayoutPriority(rawValue: UILayoutPriority.RawValue(Int(UILayoutPriority.defaultHigh.rawValue) - 1))
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

    progressView.setNeedsLayout()
    routingProgress.constant = progressView.width * info.progress / 100
    UIView.animate(withDuration: kDefaultAnimationDuration) { [progressView] in
      progressView?.layoutIfNeeded()
    }
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
    trafficButton.isSelected = MWMTrafficManager.state() != .disabled
    refreshDiminishTimer()
  }

  override var sideButtonsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return .bottom
  }
}
