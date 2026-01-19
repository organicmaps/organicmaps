import UIKit

final class TransportOptionsView: UIView {
  private var segmentedControl = UISegmentedControl()
  private var routerTypes: [MWMRouterType] = []
  private var selectedRouterType: MWMRouterType = .vehicle

  weak var interactor: NavigationDashboard.Interactor?

  init() {
    super.init(frame: .zero)
    setupSegmentedControlView()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func set(transportOptions: [MWMRouterType], selectedRouterType: MWMRouterType) {
    let optionsChanged = routerTypes != transportOptions
    let selectionChanged = self.selectedRouterType != selectedRouterType
    guard optionsChanged || selectionChanged else { return }

    routerTypes = transportOptions
    self.selectedRouterType = selectedRouterType

    if optionsChanged {
      reload()
    } else if selectionChanged {
      updateSelection()
    }
  }

  private func setupSegmentedControlView() {
    segmentedControl.translatesAutoresizingMaskIntoConstraints = false
    segmentedControl.addTarget(self, action: #selector(didChangeSegment(_:)), for: .valueChanged)
    addSubview(segmentedControl)
    NSLayoutConstraint.activate([
      segmentedControl.leadingAnchor.constraint(equalTo: leadingAnchor),
      segmentedControl.trailingAnchor.constraint(equalTo: trailingAnchor),
      segmentedControl.topAnchor.constraint(equalTo: topAnchor),
      segmentedControl.bottomAnchor.constraint(equalTo: bottomAnchor),
    ])
  }

  func reload() {
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
    guard index < routerTypes.count else { return }
    let routerType = routerTypes[index]
    guard selectedRouterType != routerType else { return }
    selectedRouterType = routerType
    interactor?.process(.selectRouterType(routerType))
  }
}
