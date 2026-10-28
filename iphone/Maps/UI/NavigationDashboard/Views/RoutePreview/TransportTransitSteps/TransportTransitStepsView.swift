final class TransportTransitStepsView: SolidTouchView {
  private enum Constants {
    static let contentInsets = UIEdgeInsets(top: 8, left: 0, bottom: -8, right: 0)
    static let chevronSpacing: CGFloat = 8
    static let chevronWidth: CGFloat = 12
  }

  private var stepsCollectionView: TransportTransitStepsCollectionView!
  private var stepsCollectionViewHeight: NSLayoutConstraint!
  private let chevronView = UIImageView(image: UIImage(systemName: "chevron.forward"))
  private var chevronWidth: NSLayoutConstraint!
  private var chevronSpacing: NSLayoutConstraint!
  private var steps: [MWMRouterTransitStepInfo] = []

  /// Called with the route's steps when a public transport strip is tapped to open its details.
  var onTap: (([MWMRouterTransitStepInfo]) -> Void)?

  init() {
    super.init(frame: .zero)
    setupViews()
    layout()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    super.init(coder: coder)
    setupViews()
  }

  private func setupViews() {
    let layout = TransportTransitFlowLayout()
    stepsCollectionView = TransportTransitStepsCollectionView(layout: layout)
    stepsCollectionView.backgroundColor = .clear
    // The strip never scrolls; let taps reach the whole tile.
    stepsCollectionView.isUserInteractionEnabled = false

    chevronView.tintColor = .blackSecondaryText
    chevronView.contentMode = .scaleAspectFit

    addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(didTap)))
    accessibilityTraits = .button
  }

  private func layout() {
    addSubview(stepsCollectionView)
    addSubview(chevronView)
    stepsCollectionView.translatesAutoresizingMaskIntoConstraints = false
    chevronView.translatesAutoresizingMaskIntoConstraints = false

    stepsCollectionViewHeight = stepsCollectionView.heightAnchor.constraint(equalToConstant: 0).withPriority(.defaultHigh)
    chevronWidth = chevronView.widthAnchor.constraint(equalToConstant: 0)
    chevronSpacing = stepsCollectionView.trailingAnchor.constraint(equalTo: chevronView.leadingAnchor)

    NSLayoutConstraint.activate([
      stepsCollectionView.topAnchor.constraint(equalTo: topAnchor, constant: Constants.contentInsets.top),
      stepsCollectionView.leadingAnchor.constraint(equalTo: leadingAnchor, constant: Constants.contentInsets.left),
      chevronSpacing,
      stepsCollectionView.bottomAnchor.constraint(equalTo: bottomAnchor, constant: Constants.contentInsets.bottom)
        .withPriority(.defaultHigh),
      stepsCollectionViewHeight,
      chevronView.trailingAnchor.constraint(equalTo: trailingAnchor, constant: Constants.contentInsets.right),
      chevronView.centerYAnchor.constraint(equalTo: stepsCollectionView.centerYAnchor),
      chevronWidth,
    ])
  }

  /// - Parameter isRouteReady: while a route is still being built the steps belong to the previous route, so
  ///   they must not open details for the new route points.
  func setNavigationInfo(_ navigationInfo: MWMNavigationDashboardEntity?, isRouteReady: Bool = false) {
    setSteps(navigationInfo?.transitSteps ?? [], isRouteReady: isRouteReady)
  }

  func setSteps(_ steps: [MWMRouterTransitStepInfo], isRouteReady: Bool) {
    self.steps = steps
    stepsCollectionView.steps = steps
    isHidden = steps.isEmpty
    // Ruler routes reuse the strip for their waypoints, but only public transport has per-leg details.
    let hasDetails = isRouteReady && !steps.isEmpty && !steps.contains { $0.type == .ruler }
    isUserInteractionEnabled = hasDetails
    chevronView.isHidden = !hasDetails
    chevronWidth.constant = hasDetails ? Constants.chevronWidth : 0
    chevronSpacing.constant = hasDetails ? -Constants.chevronSpacing : 0
    accessibilityLabel = hasDetails ? L("transit_details_title") : nil
    updateHeight()
  }

  @objc
  private func didTap() {
    onTap?(steps)
  }

  private func updateHeight() {
    layoutIfNeeded()
    animateConstraints(animations: {
      self.stepsCollectionViewHeight.constant = self.stepsCollectionView.collectionViewLayout.collectionViewContentSize.height
    })
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    layoutIfNeeded()
  }
}
