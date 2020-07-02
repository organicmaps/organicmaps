@objc(MWMBaseRoutePreviewStatus)
final class BaseRoutePreviewStatus: SolidTouchView {
  @IBOutlet private weak var errorBox: UIView!
  @IBOutlet private weak var resultsBox: UIView!
  @IBOutlet private weak var heightBox: UIView!
  @IBOutlet private weak var manageRouteBox: UIView!
  @IBOutlet weak var manageRouteBoxBackground: UIView! {
    didSet {
      iPhoneSpecific {
        manageRouteBoxBackground.styleName = "BlackOpaqueBackground"
      }
    }
  }

  @IBOutlet private weak var taxiBox: UIView!
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

  @IBOutlet private var errorBoxBottom: NSLayoutConstraint!
  @IBOutlet private var resultsBoxBottom: NSLayoutConstraint!
  @IBOutlet private var heightBoxBottom: NSLayoutConstraint!
  @IBOutlet private var taxiBoxBottom: NSLayoutConstraint!
  @IBOutlet private var manageRouteBoxBottom: NSLayoutConstraint!
  @IBOutlet private var heightBoxBottomManageRouteBoxTop: NSLayoutConstraint!

  private var hiddenConstraint: NSLayoutConstraint!
  @objc weak var ownerView: UIView!

  weak var navigationInfo: MWMNavigationDashboardEntity?

  var elevation: NSAttributedString? {
    didSet {
      updateResultsLabel()
    }
  }

  private var isVisible = false {
    didSet {
      guard isVisible != oldValue else { return }
      if isVisible {
        addView()
      }
      DispatchQueue.main.async {
        guard let sv = self.superview else { return }
        sv.animateConstraints(animations: {
          self.hiddenConstraint.isActive = !self.isVisible
        }, completion: {
          if !self.isVisible {
            self.removeFromSuperview()
          }
        })
      }
    }
  }

  private func addView() {
    guard superview != ownerView else { return }
    ownerView.addSubview(self)

    addConstraints()
  }

  private func addConstraints() {
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
    hiddenConstraint = topAnchor.constraint(equalTo: bAnchor)
    hiddenConstraint.priority = UILayoutPriority.defaultHigh
    hiddenConstraint.isActive = true

    let visibleConstraint = bottomAnchor.constraint(equalTo: bAnchor)
    visibleConstraint.priority = UILayoutPriority.defaultLow
    visibleConstraint.isActive = true

    ownerView.layoutIfNeeded()
  }

  private func updateHeight() {
    DispatchQueue.main.async {
      self.animateConstraints(animations: {
        self.errorBoxBottom.isActive = !self.errorBox.isHidden
        self.resultsBoxBottom.isActive = !self.resultsBox.isHidden
        self.heightBoxBottom.isActive = !self.heightBox.isHidden
        self.heightBoxBottomManageRouteBoxTop.isActive = !self.heightBox.isHidden
        self.taxiBoxBottom.isActive = !self.taxiBox.isHidden
        self.manageRouteBoxBottom.isActive = !self.manageRouteBox.isHidden
      })
    }
  }

  private func configManageRouteButton(_ button: UIButton) {
    button.setTitle(L("planning_route_manage_route"), for: .normal)
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateManageRouteVisibility()
    updateHeight()
  }

  private func updateManageRouteVisibility() {
    let isCompact = traitCollection.verticalSizeClass == .compact
    manageRouteBox.isHidden = isCompact || resultsBox.isHidden
    manageRouteButtonCompact?.isHidden = !isCompact
  }

  @objc func hide() {
    isVisible = false
  }

  @objc func showError(message: String) {
    isVisible = true
    errorBox.isHidden = false
    resultsBox.isHidden = true
    heightBox.isHidden = true
    taxiBox.isHidden = true
    manageRouteBox.isHidden = true

    errorLabel.text = message

    updateHeight()
  }

  @objc func showReady() {
    isVisible = true
    errorBox.isHidden = true

    if MWMRouter.isTaxi() {
      taxiBox.isHidden = false
      resultsBox.isHidden = true
      heightBox.isHidden = true
    } else {
      taxiBox.isHidden = true
      resultsBox.isHidden = false
      elevation = nil
      if MWMRouter.hasRouteAltitude() {
        heightBox.isHidden = false
        MWMRouter.routeAltitudeImage(for: heightProfileImage.frame.size,
                                     completion: { image, elevation in
                                       self.heightProfileImage.image = image
                                       guard let elevation = elevation else { return }
                                       let attributes: [NSAttributedString.Key: Any] =
                                         [
                                           .foregroundColor: UIColor.linkBlue(),
                                           .font: UIFont.medium14()
                                         ]
                                       self.elevation = NSAttributedString(string: "▲▼ \(elevation)", attributes: attributes)
        })
      } else {
        heightBox.isHidden = true
      }
    }
    updateManageRouteVisibility()
    updateHeight()
  }

  private func updateResultsLabel() {
    guard let info = navigationInfo else { return }

    if let result = info.estimate.mutableCopy() as? NSMutableAttributedString {
      if let elevation = self.elevation {
        result.append(info.estimateDot)
        result.append(elevation)
      }
      resultLabel.attributedText = result
    }
  }

  @objc func onNavigationInfoUpdated(_ info: MWMNavigationDashboardEntity) {
    navigationInfo = info
    updateResultsLabel()
    arriveLabel?.text = String(coreFormat: L("routing_arrive"), arguments: [info.arrival])
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
