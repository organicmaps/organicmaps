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
      self.setNeedsLayout()
      self.stepsCollectionViewHeight.constant = self.stepsCollectionView.contentSize.height
      UIView.animate(withDuration: kDefaultAnimationDuration) { self.layoutIfNeeded() }
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
    return alternative(iPhone: .bottom, iPad: [])
  }

  override var visibleAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return alternative(iPhone: .bottom, iPad: [])
  }

  override var widgetsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return alternative(iPhone: .bottom, iPad: [])
  }
}
