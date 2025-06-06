@objc(MWMBaseRoutePreviewStatus)
final class BaseRoutePreviewStatus: SolidTouchView {
  @IBOutlet private weak var errorBox: UIView!
  @IBOutlet private weak var resultsBox: UIView!
  @IBOutlet private weak var heightBox: UIView!
  @IBOutlet private weak var manageRouteBox: UIView!
  @IBOutlet weak var manageRouteBoxBackground: UIView! {
    didSet {
        manageRouteBoxBackground.setStyle(.blackOpaqueBackground)
    }
  }

  @IBOutlet private weak var errorLabel: UILabel!
  @IBOutlet private weak var resultLabel: UILabel!
  @IBOutlet private weak var arriveLabel: UILabel?
  @IBOutlet private weak var heightProfileImage: UIImageView!
  @IBOutlet private weak var heightProfileElevationHeight: UILabel?
  @IBOutlet private weak var manageRouteButtonRegular: UIButton! {
    didSet {
      configManageRouteButton(manageRouteButtonRegular)
    }
  }

  @IBOutlet private weak var manageRouteButtonCompact: UIButton? {
    didSet {
      configManageRouteButton(manageRouteButtonCompact!)
    }
  }

  @IBOutlet private weak var saveRouteAsTrackButtonRegular: UIButton! {
    didSet {
      configSaveRouteAsTrackButton(saveRouteAsTrackButtonRegular)
    }
  }

  @IBOutlet private weak var saveRouteAsTrackButtonCompact: UIButton! {
    didSet {
      configSaveRouteAsTrackButton(saveRouteAsTrackButtonCompact)
    }
  }

  @IBOutlet private var errorBoxBottom: NSLayoutConstraint!
  @IBOutlet private var resultsBoxBottom: NSLayoutConstraint!
  @IBOutlet private var heightBoxBottom: NSLayoutConstraint!
  @IBOutlet private var manageRouteBoxBottom: NSLayoutConstraint!
  @IBOutlet private var heightBoxBottomManageRouteBoxTop: NSLayoutConstraint!

  @objc weak var ownerView: UIView!

  weak var navigationInfo: MWMNavigationDashboardEntity?

  static let elevationAttributes: [NSAttributedString.Key: Any] =
                                        [
                                          .foregroundColor: UIColor.linkBlue(),
                                          .font: UIFont.medium14()
                                        ]

  var elevation: NSAttributedString? {
    didSet {
      updateResultsLabel()
    }
  }

  private var isVisible = false {
    didSet {
      addView()
      isHidden = !isVisible
    }
  }

  private func addView() {
    guard superview != ownerView else { return }
    ownerView.addSubview(self)

    let lg = ownerView.safeAreaLayoutGuide
    leadingAnchor.constraint(equalTo: lg.leadingAnchor).isActive = true
    trailingAnchor.constraint(equalTo: lg.trailingAnchor).isActive = true
    bottomAnchor.constraint(equalTo: lg.bottomAnchor).isActive = true
    ownerView.layoutIfNeeded()
  }

  private func updateHeight() {
    DispatchQueue.main.async {
      self.animateConstraints(animations: {
        self.errorBoxBottom.isActive = !self.errorBox.isHidden
        self.resultsBoxBottom.isActive = !self.resultsBox.isHidden
        self.heightBoxBottom.isActive = !self.heightBox.isHidden
        self.heightBoxBottomManageRouteBoxTop.isActive = !self.heightBox.isHidden
        self.manageRouteBoxBottom.isActive = !self.manageRouteBox.isHidden
      })
    }
  }

  private func configManageRouteButton(_ button: UIButton) {
    button.setImagePadding(8)
    button.setTitle(L("planning_route_manage_route"), for: .normal)
  }

  private func configSaveRouteAsTrackButton(_ button: UIButton) {
    button.setImagePadding(8)
    button.setTitle(L("save"), for: .normal)
    button.setTitle(L("saved"), for: .disabled)
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateManageRouteVisibility()
    updateHeight()
    updateResultsLabel()
  }

  private func updateManageRouteVisibility() {
    let isCompact = traitCollection.verticalSizeClass == .compact
    manageRouteBox.isHidden = isCompact || resultsBox.isHidden
    manageRouteButtonCompact?.isHidden = !isCompact
    saveRouteAsTrackButtonCompact.isHidden = !isCompact
  }

  @objc func hide() {
    isVisible = false
  }

  @objc func showError(message: String) {
    isVisible = true
    errorBox.isHidden = false
    resultsBox.isHidden = true
    heightBox.isHidden = true
    manageRouteBox.isHidden = true
    errorLabel.text = message
    updateHeight()
  }

  @objc func showReady() {
    isVisible = true
    errorBox.isHidden = true
    resultsBox.isHidden = false
    elevation = nil
    if MWMRouter.hasRouteAltitude() {
      heightBox.isHidden = false
      MWMRouter.routeAltitudeImage(for: heightProfileImage.frame.size,
                                    completion: { image, totalAscent, totalDescent in
                                    self.heightProfileImage.image = image
                                    if let totalAscent = totalAscent, let totalDescent = totalDescent {                                      
                                      self.elevation = NSAttributedString(string: "↗ \(totalAscent) ↘ \(totalDescent)", attributes: BaseRoutePreviewStatus.elevationAttributes)
                                    }
      })
    } else {
      heightBox.isHidden = true
    }
    setRouteAsTrackButtonEnabled(true)
    updateManageRouteVisibility()
    updateHeight()
  }

  @objc func setRouteSaved(_ isSaved: Bool) {
    setRouteAsTrackButtonEnabled(!isSaved)
  }

  private func setRouteAsTrackButtonEnabled(_ isEnabled: Bool) {
    saveRouteAsTrackButtonRegular.isEnabled = isEnabled
    saveRouteAsTrackButtonCompact.isEnabled = isEnabled
  }

  private func updateResultsLabel() {
    guard let info = navigationInfo else { return }

    if let result = info.estimate().mutableCopy() as? NSMutableAttributedString {
      if let elevation = self.elevation {
        result.append(MWMNavigationDashboardEntity.estimateDot())
        result.append(elevation)
      }
      
      resultLabel.attributedText = result
    }
  }

  @objc func onNavigationInfoUpdated(_ info: MWMNavigationDashboardEntity) {
    navigationInfo = info
    updateResultsLabel()
    arriveLabel?.text = String(format: L("routing_arrive"), arguments: [info.arrival])
  }

  override var sideButtonsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return .bottom
  }

  override var visibleAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return .bottom
  }

  override var widgetsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return .bottom
  }
}
