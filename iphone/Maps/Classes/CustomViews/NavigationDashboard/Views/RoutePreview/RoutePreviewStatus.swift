@objc(MWMRoutePreviewStatus)
final class RoutePreviewStatus: SolidTouchView {
  @IBOutlet private weak var errorBox: UIView!
  @IBOutlet private weak var resultsBox: UIView!
  @IBOutlet private weak var heightBox: UIView!
  @IBOutlet private weak var manageRouteBox: UIView! {
    didSet {
      iPhoneSpecific {
        manageRouteBox.backgroundColor = UIColor.blackOpaque()
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
  weak var ownerView: UIView!

  var isVisible = false {
    didSet {
      alternative(iPhone: {
        guard self.isVisible != oldValue else { return }
        if self.isVisible {
          self.addView()
        }
        DispatchQueue.main.async {
          guard let sv = self.superview else { return }
          sv.setNeedsLayout()
          self.hiddenConstraint.isActive = !self.isVisible
          UIView.animate(withDuration: kDefaultAnimationDuration,
                         animations: { sv.layoutIfNeeded() },
                         completion: { _ in
                           if !self.isVisible {
                             self.removeFromSuperview()
                           }
          })
        }
      },
      iPad: { self.isHidden = !self.isVisible })()
    }
  }

  private func addView() {
    guard superview != ownerView else { return }
    ownerView.addSubview(self)

    NSLayoutConstraint(item: self, attribute: .left, relatedBy: .equal, toItem: ownerView, attribute: .left, multiplier: 1, constant: 0).isActive = true
    NSLayoutConstraint(item: self, attribute: .right, relatedBy: .equal, toItem: ownerView, attribute: .right, multiplier: 1, constant: 0).isActive = true

    hiddenConstraint = NSLayoutConstraint(item: self, attribute: .top, relatedBy: .equal, toItem: ownerView, attribute: .bottom, multiplier: 1, constant: 0)
    hiddenConstraint.priority = UILayoutPriorityDefaultHigh
    hiddenConstraint.isActive = true

    let visibleConstraint = NSLayoutConstraint(item: self, attribute: .bottom, relatedBy: .equal, toItem: ownerView, attribute: .bottom, multiplier: 1, constant: 0)
    visibleConstraint.priority = UILayoutPriorityDefaultLow
    visibleConstraint.isActive = true
  }

  private func updateHeight() {
    DispatchQueue.main.async {
      self.setNeedsLayout()
      self.errorBoxBottom.isActive = !self.errorBox.isHidden
      self.resultsBoxBottom.isActive = !self.resultsBox.isHidden
      self.heightBoxBottom.isActive = !self.heightBox.isHidden
      self.heightBoxBottomManageRouteBoxTop.isActive = !self.heightBox.isHidden
      self.taxiBoxBottom.isActive = !self.taxiBox.isHidden
      self.manageRouteBoxBottom.isActive = !self.manageRouteBox.isHidden
      UIView.animate(withDuration: kDefaultAnimationDuration) { self.layoutIfNeeded() }
    }
  }

  private func configManageRouteButton(_ button: UIButton) {
    button.titleLabel?.font = UIFont.medium14()
    button.setTitle(L("planning_route_manage_route"), for: .normal)
    button.tintColor = UIColor.blackSecondaryText()
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

  func stateHidden() {
    isVisible = false
  }

  func statePrepare() {
    isVisible = false
  }

  func stateError(message: String) {
    isVisible = true
    errorBox.isHidden = false
    resultsBox.isHidden = true
    heightBox.isHidden = true
    taxiBox.isHidden = true
    manageRouteBox.isHidden = true

    errorLabel.text = message

    updateHeight()
  }

  func stateReady() {
    isVisible = true
    errorBox.isHidden = true

    if MWMRouter.isTaxi() {
      taxiBox.isHidden = false
      resultsBox.isHidden = true
      heightBox.isHidden = true
    } else {
      taxiBox.isHidden = true
      resultsBox.isHidden = false
      if MWMRouter.hasRouteAltitude() {
        heightBox.isHidden = false
        MWMRouter.routeAltitudeImage(for: heightProfileImage.frame.size,
                                     completion: { image, elevation in
                                       self.heightProfileImage.image = image
                                       self.heightProfileElevationHeight?.text = elevation
        })
      } else {
        heightBox.isHidden = true
      }
    }
    updateManageRouteVisibility()
    updateHeight()
  }

  func stateNavigation() {
    isVisible = false
  }

  func onNavigationInfoUpdated(_ info: MWMNavigationDashboardEntity) {
    resultLabel.attributedText = info.estimate
    arriveLabel?.text = String(coreFormat: L("routing_arrive"), arguments: [info.arrival])
  }

  override var sideButtonsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return alternative(iPhone: .bottom, iPad: [])
  }

  override var visibleAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return alternative(iPhone: .bottom, iPad: [])
  }
}
