import UIKit

final class TransportOptionsView: UIView {
  /// The picker's presentation: the standard picker fills the row with all router types; a
  /// followed track reduces it to the followable profiles.
  enum Mode: Equatable {
    case standard
    case followTrack
  }

  private enum Constants {
    static let velocityThreshold: CGFloat = 5
  }

  private var segmentedControl = UISegmentedControl()
  private var routerTypes: [MWMRouterType] = []
  private var selectedRouterType: MWMRouterType = .vehicle
  private var mode: Mode = .standard
  private let panelPanGestureRecognizer = UIPanGestureRecognizer()

  weak var interactor: NavigationDashboard.Interactor?
  var onPanGesture: ((UIPanGestureRecognizer) -> Void)?

  init() {
    super.init(frame: .zero)
    setupSegmentedControlView()
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
  }

  /// Profile controls are disabled while a preview rebuild is pending.
  func setSelectionEnabled(_ enabled: Bool) {
    segmentedControl.isEnabled = enabled
  }

  private func setupSegmentedControlView() {
    segmentedControl.translatesAutoresizingMaskIntoConstraints = false
    segmentedControl.addTarget(self, action: #selector(didChangeSegment(_:)), for: .valueChanged)

    panelPanGestureRecognizer.addTarget(self, action: #selector(handlePanGesture(_:)))
    panelPanGestureRecognizer.delegate = self
    panelPanGestureRecognizer.cancelsTouchesInView = false
    panelPanGestureRecognizer.delaysTouchesBegan = true
    addGestureRecognizer(panelPanGestureRecognizer)

    addSubview(segmentedControl)
    NSLayoutConstraint.activate([
      segmentedControl.leadingAnchor.constraint(equalTo: leadingAnchor),
      segmentedControl.topAnchor.constraint(equalTo: topAnchor),
      segmentedControl.bottomAnchor.constraint(equalTo: bottomAnchor),
      segmentedControl.trailingAnchor.constraint(equalTo: trailingAnchor),
    ])
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
