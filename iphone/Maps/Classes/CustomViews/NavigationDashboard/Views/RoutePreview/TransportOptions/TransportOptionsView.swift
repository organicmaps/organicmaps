final class TransportOptionsView: UIView {

  private enum Constants {
    static let transportOptionsItemSize = CGSize(width: 40, height: 40)
  }

  private var stackView: UIStackView!
  private var transportOptions: [MWMRouterType] = []
  private var selectedRouterType: MWMRouterType = .vehicle
  private var optionButtons: [CircleImageButton] = []

  weak var interactor: NavigationDashboard.Interactor?

  init() {
    super.init(frame: .zero)
    setupView()
    layout()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupView() {
    stackView = UIStackView()
    stackView.axis = .horizontal
    stackView.alignment = .center
    stackView.distribution = .equalSpacing
    stackView.spacing = 0
    addSubview(stackView)
  }

  private func layout() {
    stackView.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      stackView.leadingAnchor.constraint(equalTo: leadingAnchor),
      stackView.trailingAnchor.constraint(equalTo: trailingAnchor),
      stackView.topAnchor.constraint(equalTo: topAnchor),
      stackView.bottomAnchor.constraint(equalTo: bottomAnchor)
    ])
  }

  func set(transportOptions: [MWMRouterType], selectedRouterType: MWMRouterType) {
    let optionsChanged = self.transportOptions != transportOptions
    let selectionChanged = self.selectedRouterType != selectedRouterType
    guard optionsChanged || selectionChanged else {
      return
    }
    self.transportOptions = transportOptions
    self.selectedRouterType = selectedRouterType
    if optionsChanged {
      reload()
    } else if selectionChanged {
      updateSelection()
    }
  }

  func reload() {
    stackView.arrangedSubviews.forEach { $0.removeFromSuperview() }
    optionButtons.removeAll()
    for (index, routerType) in transportOptions.enumerated() {
      let button = CircleImageButton(frame: CGRect(origin: .zero, size: Constants.transportOptionsItemSize))
      let isSelected = routerType == selectedRouterType
      button.setImage(routerType.image(for: isSelected), style: isSelected ? GlobalStyleSheet.blue : .black)
      button.tag = index
      button.addTarget(self, action: #selector(optionButtonTapped(_:)), for: .touchUpInside)
      stackView.addArrangedSubview(button)
      optionButtons.append(button)
    }
  }

  private func updateSelection() {
    for (index, routerType) in transportOptions.enumerated() {
      let isSelected = routerType == selectedRouterType
      let button = optionButtons[safe: index]
      button?.setImage(routerType.image(for: isSelected), style: isSelected ? GlobalStyleSheet.blue : .black)
    }
  }

  @objc private func optionButtonTapped(_ sender: UIButton) {
    let index = sender.tag
    guard index < transportOptions.count else { return }
    let routerType = transportOptions[index]
    guard selectedRouterType != routerType else { return }
    selectedRouterType = routerType
    updateSelection()
    interactor?.process(.selectRouterType(routerType))
  }
}

private extension Array {
  subscript(safe index: Int) -> Element? {
    return indices.contains(index) ? self[index] : nil
  }
}
