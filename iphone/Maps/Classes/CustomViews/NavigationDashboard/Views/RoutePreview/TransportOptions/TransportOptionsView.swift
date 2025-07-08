import SwiftUI

final class TransportOptionsView: UIView {

  private enum Constants {
    static let transportOptionsItemSize = CGSize(width: 40, height: 40)
  }

  private var stackView: UIStackView?
  private var transportOptions: [MWMRouterType] = []
  private var selectedRouterType: MWMRouterType = .vehicle
  private var optionButtons: [CircleImageButton] = []
  private var hostingController: UIViewController?

  weak var interactor: NavigationDashboard.Interactor?

  init() {
    super.init(frame: .zero)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func set(transportOptions: [MWMRouterType], selectedRouterType: MWMRouterType) {
    let optionsChanged = self.transportOptions != transportOptions
    let selectionChanged = self.selectedRouterType != selectedRouterType
    guard optionsChanged || selectionChanged else {
      return
    }
    self.transportOptions = transportOptions
    self.selectedRouterType = selectedRouterType

    if #available(iOS 15, *) {
      setupSUIView(transportOptions: transportOptions, selectedRouterType: selectedRouterType)
    } else {
      if stackView == nil {
        setupDefaultView()
      }
      if optionsChanged {
        reload()
      } else if selectionChanged {
        updateSelection()
      }
    }
  }

  private func setupDefaultView() {
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
  private func setupSUIView(transportOptions: [MWMRouterType], selectedRouterType: MWMRouterType) {
    guard hostingController == nil else { return }
    let rootView = TransportOptionsSegmentedControl(
      options: transportOptions,
      selected: selectedRouterType,
      onSelect: { type in
        if self.selectedRouterType != type {
          self.selectedRouterType = type
          self.interactor?.process(.selectRouterType(type))
        }
      }
    )
    let controller = UIHostingController(rootView: rootView)
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
    } else {
      guard let stackView else { return }
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
  }

  private func updateSelection() {
    for (index, routerType) in transportOptions.enumerated() {
      let isSelected = routerType == selectedRouterType
      optionButtons[index].setImage(routerType.image(for: isSelected), style: isSelected ? GlobalStyleSheet.blue : .black)
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

@available(iOS 15, *)
fileprivate struct TransportOptionsSegmentedControl: View {

  private enum Constants {
    static let spacing: CGFloat = 8
    static let imageSize: CGFloat = 20
    static let padding: CGFloat = 6
    static let animationDuration: Double = kDefaultAnimationDuration / 2
  }

  @State var options: [MWMRouterType]
  @State var selected: MWMRouterType
  let onSelect: (MWMRouterType) -> Void

  @Namespace private var animation
  private let selectedOptionId = "selectedOptionId"

  var body: some View {
    HStack(spacing: Constants.spacing) {
      ForEach(options, id: \.self) { type in
        Button {
          UIImpactFeedbackGenerator(style: .medium).impactOccurred()
          withAnimation {
            selected = type
          }
          onSelect(type)
        } label: {
          ZStack {
            if selected == type {
              Capsule()
                .fill(Color(uiColor: .linkBlue()))
                .matchedGeometryEffect(id: selectedOptionId, in: animation)
            }
            Image(uiImage: type.image(for: selected == type))
              .renderingMode(.template)
              .resizable()
              .scaledToFit()
              .frame(height: Constants.imageSize)
              .foregroundColor(selected == type ? .white : Color(uiColor: .blackSecondaryText()))
          }
          .padding(Constants.padding)
          .frame(maxWidth: .infinity)
          .animation(.easeIn(duration: Constants.animationDuration), value: selected)
        }
      }
    }
    .frame(maxWidth: .infinity)
    .background(Color(uiColor: .pressBackground()))
    .clipShape(Capsule())
  }
}
