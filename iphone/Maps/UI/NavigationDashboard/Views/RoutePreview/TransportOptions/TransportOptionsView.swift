import UIKit

final class TransportOptionsView: UIView {
  /// The picker's presentation: the standard picker fills the row with all router types; a
  /// followed track reduces it to the followable profiles and adds the direction chip.
  enum Mode: Equatable {
    case standard
    case followTrack(isReversed: Bool)
  }

  private enum Constants {
    static let velocityThreshold: CGFloat = 5
    static let directionSpacing: CGFloat = 8
    static let directionHeight: CGFloat = 32
    static let directionContentInsets = UIEdgeInsets(top: 0, left: 12, bottom: 0, right: 12)
    // Language-neutral: the track is followed from its start (A) to its end (B) or backwards.
    static let forwardDirectionTitle = "A→B"
    static let backwardDirectionTitle = "B→A"
  }

  private var segmentedControl = UISegmentedControl()
  private let directionButton = UIButton()
  private var routerTypes: [MWMRouterType] = []
  private var selectedRouterType: MWMRouterType = .vehicle
  private var mode: Mode = .standard
  private let panelPanGestureRecognizer = UIPanGestureRecognizer()
  private let impactGenerator = UIImpactFeedbackGenerator(style: .medium)
  private var segmentedControlTrailingConstraint: NSLayoutConstraint!
  private var followTrackConstraints: [NSLayoutConstraint] = []

  weak var interactor: NavigationDashboard.Interactor?
  var onPanGesture: ((UIPanGestureRecognizer) -> Void)?

  init() {
    super.init(frame: .zero)
    setupSegmentedControlView()
    setupDirectionButton()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func set(transportOptions: [MWMRouterType], selectedRouterType: MWMRouterType, mode: Mode) {
    let optionsChanged = routerTypes != transportOptions
    let selectionChanged = self.selectedRouterType != selectedRouterType
    let modeChanged = self.mode != mode
    guard optionsChanged || selectionChanged || modeChanged else { return }

    routerTypes = transportOptions
    self.selectedRouterType = selectedRouterType
    self.mode = mode

    if optionsChanged {
      reload()
    } else if selectionChanged {
      updateSelection()
    }
    if modeChanged {
      applyMode()
    }
  }

  /// A follow rebuild (profile switch or reversal) tears the session down and rebuilds it:
  /// the controls are disabled while one is pending.
  func setSelectionEnabled(_ enabled: Bool, directionEnabled: Bool) {
    segmentedControl.isEnabled = enabled
    directionButton.isEnabled = directionEnabled
  }

  private func setupSegmentedControlView() {
    segmentedControl.translatesAutoresizingMaskIntoConstraints = false
    segmentedControl.addTarget(self, action: #selector(didChangeSegment(_:)), for: .valueChanged)
    // In the follow-track mode nothing pins the trailing edge: on the narrowest layouts the
    // segments give way to the direction chip instead of pushing it out.
    segmentedControl.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)

    panelPanGestureRecognizer.addTarget(self, action: #selector(handlePanGesture(_:)))
    panelPanGestureRecognizer.delegate = self
    panelPanGestureRecognizer.cancelsTouchesInView = false
    panelPanGestureRecognizer.delaysTouchesBegan = true
    addGestureRecognizer(panelPanGestureRecognizer)

    addSubview(segmentedControl)
    segmentedControlTrailingConstraint = segmentedControl.trailingAnchor.constraint(equalTo: trailingAnchor)
    NSLayoutConstraint.activate([
      segmentedControl.leadingAnchor.constraint(equalTo: leadingAnchor),
      segmentedControl.topAnchor.constraint(equalTo: topAnchor),
      segmentedControl.bottomAnchor.constraint(equalTo: bottomAnchor),
      segmentedControlTrailingConstraint,
    ])
  }

  private func setupDirectionButton() {
    directionButton.setStyle(.flatNormalGrayButtonBig)
    directionButton.contentEdgeInsets = Constants.directionContentInsets
    directionButton.addTarget(self, action: #selector(didTapDirectionButton), for: .touchUpInside)
    directionButton.translatesAutoresizingMaskIntoConstraints = false
    directionButton.setContentHuggingPriority(.required, for: .horizontal)
    directionButton.setContentCompressionResistancePriority(.required, for: .horizontal)
    directionButton.accessibilityLabel = L("reverse_track_direction")
    directionButton.isHidden = true

    addSubview(directionButton)
    followTrackConstraints = [
      directionButton.leadingAnchor.constraint(greaterThanOrEqualTo: segmentedControl.trailingAnchor,
                                               constant: Constants.directionSpacing),
      directionButton.trailingAnchor.constraint(equalTo: trailingAnchor),
      directionButton.centerYAnchor.constraint(equalTo: centerYAnchor),
      directionButton.heightAnchor.constraint(equalToConstant: Constants.directionHeight),
    ]
  }

  private func applyMode() {
    switch mode {
    case .standard:
      directionButton.isHidden = true
      NSLayoutConstraint.deactivate(followTrackConstraints)
      segmentedControlTrailingConstraint.isActive = true
    case .followTrack(let isReversed):
      let title = isReversed ? Constants.backwardDirectionTitle : Constants.forwardDirectionTitle
      directionButton.setTitle(title, for: .normal)
      directionButton.accessibilityValue = title
      directionButton.isHidden = false
      segmentedControlTrailingConstraint.isActive = false
      NSLayoutConstraint.activate(followTrackConstraints)
    }
  }

  private func reload() {
    segmentedControl.removeAllSegments()
    for (index, type) in routerTypes.enumerated() {
      let isSelected = type == selectedRouterType
      segmentedControl.insertSegment(with: type.image(for: isSelected), at: index, animated: false)
      if isSelected {
        segmentedControl.selectedSegmentIndex = index
      }
    }
  }

  private func updateSelection() {
    for (index, type) in routerTypes.enumerated() {
      let isSelected = type == selectedRouterType
      segmentedControl.setImage(type.image(for: isSelected), forSegmentAt: index)
      if isSelected {
        segmentedControl.selectedSegmentIndex = index
      }
    }
  }

  @objc private func didChangeSegment(_ sender: UISegmentedControl) {
    let index = sender.selectedSegmentIndex
    guard index >= 0, index < routerTypes.count else { return }
    let routerType = routerTypes[index]
    guard selectedRouterType != routerType else { return }
    switch mode {
    case .standard:
      selectedRouterType = routerType
      interactor?.process(.selectRouterType(routerType))
    case .followTrack:
      // The follow rebuild may be refused (e.g. the followed track has been deleted): keep the
      // control on the confirmed selection and let the state refresh move it on success.
      updateSelection()
      interactor?.process(.selectRouterType(routerType))
    }
  }

  @objc private func didTapDirectionButton() {
    impactGenerator.impactOccurred()
    interactor?.process(.reverseTrackDirectionButtonDidTap)
  }

  @objc private func handlePanGesture(_ gesture: UIPanGestureRecognizer) {
    onPanGesture?(gesture)
  }
}

extension TransportOptionsView: UIGestureRecognizerDelegate {
  override func gestureRecognizerShouldBegin(_: UIGestureRecognizer) -> Bool {
    let velocity = panelPanGestureRecognizer.velocity(in: self)
    return abs(velocity.y) > abs(velocity.x) + Constants.velocityThreshold
  }
}
