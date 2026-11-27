final class TransportTransitStepsView: SolidTouchView {
  private enum Constants {
    static let contentInsets = UIEdgeInsets(top: 8, left: 0, bottom: -8, right: 0)
  }

  private var stepsCollectionView: TransportTransitStepsCollectionView!
  private var stepsCollectionViewHeight: NSLayoutConstraint!

  var onContentHeightChanged: (() -> Void)?

  init() {
    super.init(frame: .zero)
    setupViews()
    layout()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupViews() {
    let layout = TransportTransitFlowLayout()
    stepsCollectionView = TransportTransitStepsCollectionView(layout: layout)
    stepsCollectionView.backgroundColor = .clear
    stepsCollectionView.isScrollEnabled = false
    stepsCollectionView.onContentHeightChanged = { [weak self] height in
      guard let self else { return }
      self.stepsCollectionViewHeight.constant = height
      self.onContentHeightChanged?()
    }
    stepsCollectionView.isAccessibilityElement = true
    stepsCollectionView.accessibilityTraits = .staticText
  }

  private func layout() {
    addSubview(stepsCollectionView)
    stepsCollectionView.translatesAutoresizingMaskIntoConstraints = false

    stepsCollectionViewHeight = stepsCollectionView.heightAnchor.constraint(equalToConstant: 0)

    NSLayoutConstraint.activate([
      stepsCollectionView.topAnchor.constraint(equalTo: topAnchor, constant: Constants.contentInsets.top),
      stepsCollectionView.leadingAnchor.constraint(equalTo: leadingAnchor, constant: Constants.contentInsets.left),
      stepsCollectionView.trailingAnchor.constraint(equalTo: trailingAnchor, constant: Constants.contentInsets.right),
      // A hidden arranged view collapses to zero height, including its insets.
      stepsCollectionView.bottomAnchor.constraint(equalTo: bottomAnchor, constant: Constants.contentInsets.bottom)
        .withPriority(UILayoutPriority(999)),
      stepsCollectionViewHeight,
    ])
  }

  func setNavigationInfo(_ navigationInfo: MWMNavigationDashboardEntity?) {
    let steps = navigationInfo?.transitSteps ?? []
    stepsCollectionView.steps = steps
    isHidden = steps.isEmpty
    stepsCollectionView.accessibilityLabel = steps.isEmpty ? nil
      : ListFormatter.localizedString(byJoining: steps.map(\.accessibilityDescription))
  }
}

private extension MWMRouterTransitStepInfo {
  var accessibilityDescription: String {
    let distanceText = [distance ?? "", distanceUnits ?? ""].filter { !$0.isEmpty }.joined(separator: " ")
    let title: String
    switch type {
    case .intermediatePoint:
      title = String(format: L("routing_accessibility_stop_number"), intermediateIndex + 1)
    case .pedestrian:
      title = L("routing_accessibility_walking")
    case .subway:
      title = L("button_layer_subway")
    case .train:
      title = L("routing_accessibility_train")
    case .lightRail:
      title = L("routing_accessibility_light_rail")
    case .monorail:
      title = L("routing_accessibility_monorail")
    case .tram:
      title = L("routing_accessibility_tram")
    case .bus:
      title = L("routing_accessibility_bus")
    case .ferry:
      title = L("routing_accessibility_ferry")
    case .cableTram:
      title = L("routing_accessibility_cable_tram")
    case .aerialLift:
      title = L("routing_accessibility_aerial_lift")
    case .funicular:
      title = L("routing_accessibility_funicular")
    case .airService:
      title = L("routing_accessibility_air_service")
    case .waterService:
      title = L("routing_accessibility_water_service")
    case .ruler:
      title = L("placepage_distance")
    }
    return [title, number ?? "", distanceText].filter { !$0.isEmpty }.joined(separator: " ")
  }
}
