@objc(MWMTransportRoutePreviewStatus)
final class TransportRoutePreviewStatus: SolidTouchView {
  @IBOutlet private weak var etaLabel: UILabel!
  @IBOutlet private weak var stepsCollectionView: TransportTransitStepsCollectionView!
  @IBOutlet private weak var stepsCollectionViewHeight: NSLayoutConstraint!

  private var hiddenConstraint: NSLayoutConstraint!
  @objc weak var ownerView: UIView!

  weak var navigationInfo: MWMNavigationDashboardEntity?

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
  }

  @objc func hide() {
    isVisible = false
  }

  @objc func showReady() {
    isVisible = true
    updateHeight()
  }

  @objc func onNavigationInfoUpdated(_ info: MWMNavigationDashboardEntity) {
    navigationInfo = info
    etaLabel.attributedText = info.estimate
    stepsCollectionView.steps = info.transitSteps
  }

  private func updateHeight() {
    guard stepsCollectionViewHeight.constant != stepsCollectionView.contentSize.height else { return }
    DispatchQueue.main.async {
      self.animateConstraints(animations: {
        self.stepsCollectionViewHeight.constant = self.stepsCollectionView.contentSize.height
      })
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    updateHeight()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateHeight()
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
