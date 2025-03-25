import SwiftUI

final class TransportOptionsView: UIView {

  private enum Constants {
    static let transportOptionsItemSize = CGSize(width: 40, height: 40)
  }

  private var stackView: UIStackView?
  private var routerTypes: [MWMRouterType] = []
  private var selectedRouterType: MWMRouterType = .vehicle
  private var optionButtons: [CircleImageButton] = []
  private var hostingController: TransportOptionsViewController?

  weak var interactor: NavigationDashboard.Interactor?

  init() {
    super.init(frame: .zero)
    if #available(iOS 15, *) {
      setupSUIView(routerTypes: routerTypes, selectedRouterType: selectedRouterType)
    } else {
      setupDefaultView()
    }
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func set(transportOptions: [MWMRouterType], selectedRouterType: MWMRouterType) {
    let optionsChanged = self.routerTypes != transportOptions
    let selectionChanged = self.selectedRouterType != selectedRouterType
    guard optionsChanged || selectionChanged else { return }

    self.routerTypes = transportOptions
    self.selectedRouterType = selectedRouterType

    if optionsChanged {
      reload()
    } else if selectionChanged {
      updateSelection()
    }
  }

  private func setupDefaultView() {
    guard stackView == nil else { return }
    let stackView = UIStackView()
    stackView.axis = .horizontal
    stackView.alignment = .center
    stackView.distribution = .equalSpacing
    stackView.spacing = 0
    addSubview(stackView)
    stackView.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      stackView.leadingAnchor.constraint(equalTo: leadingAnchor),
      stackView.trailingAnchor.constraint(equalTo: trailingAnchor),
      stackView.topAnchor.constraint(equalTo: topAnchor),
      stackView.bottomAnchor.constraint(equalTo: bottomAnchor)
    ])
    self.stackView = stackView
  }

  @available(iOS 15, *)
  private func setupSUIView(routerTypes: [MWMRouterType], selectedRouterType: MWMRouterType) {
    guard hostingController == nil else { return }
    let controller = TransportOptionsHostingController(options: routerTypes,
                                                       selected: selectedRouterType,
                                                       onSelect: { type in
      if self.selectedRouterType != type {
        self.selectedRouterType = type
        self.interactor?.process(.selectRouterType(type))
      }
    })
    controller.view.translatesAutoresizingMaskIntoConstraints = false
    controller.view.backgroundColor = .clear
    addSubview(controller.view)
    NSLayoutConstraint.activate([
      controller.view.leadingAnchor.constraint(equalTo: leadingAnchor),
      controller.view.trailingAnchor.constraint(equalTo: trailingAnchor),
      controller.view.topAnchor.constraint(equalTo: topAnchor),
      controller.view.bottomAnchor.constraint(equalTo: bottomAnchor)
    ])
    hostingController = controller
  }

  func reload() {
    if #available(iOS 15, *) {
      hostingController?.update(transportOptions: routerTypes, selectedRouterType: selectedRouterType)
    } else {
      guard let stackView else { return }
      stackView.arrangedSubviews.forEach { $0.removeFromSuperview() }
      optionButtons.removeAll()
      for (index, routerType) in routerTypes.enumerated() {
        let button = CircleImageButton(frame: CGRect(origin: .zero, size: Constants.transportOptionsItemSize))
        let isSelected = routerType == selectedRouterType
        button.setImage(routerType.image(for: isSelected), style: isSelected ? GlobalStyleSheet.blue : .black)
        button.tag = index
        button.addTarget(self, action: #selector(didSelectRouter(_:)), for: .touchUpInside)
        stackView.addArrangedSubview(button)
        optionButtons.append(button)
      }
    }
  }

  private func updateSelection() {
    if #available(iOS 15, *) {
      hostingController?.update(transportOptions: routerTypes, selectedRouterType: selectedRouterType)
    } else {
      for (index, routerType) in routerTypes.enumerated() {
        let isSelected = routerType == selectedRouterType
        optionButtons[index].setImage(routerType.image(for: isSelected), style: isSelected ? GlobalStyleSheet.blue : .black)
      }
    }
  }

  @objc private func didSelectRouter(_ sender: UIButton) {
    let index = sender.tag
    guard index < routerTypes.count else { return }
    let routerType = routerTypes[index]
    guard selectedRouterType != routerType else { return }
    selectedRouterType = routerType
    updateSelection()
    interactor?.process(.selectRouterType(routerType))
  }
}
