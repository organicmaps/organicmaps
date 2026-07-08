final class ColorGridViewController: UIViewController {
  enum Constants {
    static let columnCount = 4
    static let spacing: CGFloat = 8
    static let layoutMargins = NSDirectionalEdgeInsets(top: 16, leading: 16, bottom: 16, trailing: 16)

    static func preferredContentSize(forItemCount itemCount: Int) -> CGSize {
      let rowCount = Int(ceil(Double(itemCount) / Double(columnCount)))
      let width = layoutMargins.leading + layoutMargins.trailing +
        CGFloat(columnCount) * ColorPickerButton.Constants.buttonSize +
        CGFloat(columnCount - 1) * spacing
      let height = layoutMargins.top + layoutMargins.bottom +
        CGFloat(rowCount) * ColorPickerButton.Constants.buttonSize +
        CGFloat(max(0, rowCount - 1)) * spacing
      return CGSize(width: width, height: height)
    }
  }

  var onDismiss: (() -> Void)?

  private let currentColor: UIColor?
  private let predefinedColors: [PredefinedColor]
  private let onSelectColor: (UIColor) -> Void
  private let onSelectCustomColor: (UIViewController) -> Void
  private let showsCustomColor = !ProcessInfo.processInfo.isiOSAppOnMac
  private let stackView = UIStackView()

  init(currentColor: UIColor?,
       predefinedColors: [PredefinedColor],
       onSelectColor: @escaping (UIColor) -> Void,
       onSelectCustomColor: @escaping (UIViewController) -> Void) {
    self.currentColor = currentColor
    self.predefinedColors = predefinedColors
    self.onSelectColor = onSelectColor
    self.onSelectCustomColor = onSelectCustomColor
    super.init(nibName: nil, bundle: nil)
    preferredContentSize = Constants.preferredContentSize(forItemCount: predefinedColors.count + (showsCustomColor ? 1 : 0))
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    view.backgroundColor = .systemBackground
    configureStackView()
    configureColorButtons()
  }

  private func configureStackView() {
    stackView.axis = .vertical
    stackView.alignment = .fill
    stackView.distribution = .fillEqually
    stackView.spacing = Constants.spacing
    stackView.translatesAutoresizingMaskIntoConstraints = false

    view.addSubview(stackView)
    NSLayoutConstraint.activate([
      stackView.leadingAnchor.constraint(equalTo: view.layoutMarginsGuide.leadingAnchor),
      stackView.trailingAnchor.constraint(equalTo: view.layoutMarginsGuide.trailingAnchor),
      stackView.topAnchor.constraint(equalTo: view.layoutMarginsGuide.topAnchor),
      stackView.bottomAnchor.constraint(equalTo: view.layoutMarginsGuide.bottomAnchor),
    ])
    view.directionalLayoutMargins = Constants.layoutMargins
  }

  private func configureColorButtons() {
    var buttons = predefinedColors.map { button(for: $0) }
    if showsCustomColor {
      buttons.append(customColorButton())
    }

    for rowItems in buttons.chunked(by: Constants.columnCount) {
      let rowView = UIStackView()
      rowView.axis = .horizontal
      rowView.alignment = .fill
      rowView.distribution = .fillEqually
      rowView.spacing = Constants.spacing
      stackView.addArrangedSubview(rowView)

      for button in rowItems {
        rowView.addArrangedSubview(button)
      }
      for _ in rowItems.count ..< Constants.columnCount {
        rowView.addArrangedSubview(UIView())
      }
    }
  }

  private func button(for predefinedColor: PredefinedColor) -> UIButton {
    let button = ColorPickerButton()
    button.translatesAutoresizingMaskIntoConstraints = false
    button.configure(predefinedColor: predefinedColor, selected: isSelected(predefinedColor))
    button.addAction(UIAction { [weak self] _ in
      self?.onSelectColor(BookmarksManager.color(from: predefinedColor))
    }, for: .touchUpInside)
    return button
  }

  private func customColorButton() -> UIButton {
    let button = ColorPickerButton()
    button.translatesAutoresizingMaskIntoConstraints = false
    button.configure(image: .icRainbowCircle, label: L("change_color"), selected: isCustomColorSelected)
    button.addAction(UIAction { [weak self] _ in
      guard let self else { return }
      onSelectCustomColor(self)
    }, for: .touchUpInside)
    return button
  }

  private func isSelected(_ predefinedColor: PredefinedColor) -> Bool {
    guard let currentColor else { return false }
    return currentColor.isSameColorAs(BookmarksManager.color(from: predefinedColor))
  }

  private var isCustomColorSelected: Bool {
    guard currentColor != nil else { return false }
    return !predefinedColors.contains { isSelected($0) }
  }
}

private extension Array {
  func chunked(by size: Int) -> [[Element]] {
    stride(from: 0, to: count, by: size).map {
      Array(self[$0 ..< Swift.min($0 + size, count)])
    }
  }
}
