final class InfoItemView: UIView {

  private enum Constants {
    static let contentInsets = UIEdgeInsets(top: 8, left: 0, bottom: -8, right: 0)
    static let iconButtonSize: CGFloat = 56
    static let iconButtonEdgeInsets = UIEdgeInsets(top: 8, left: 0, bottom: 8, right: 0)
    static let infoLabelMinHeight: CGFloat = 28
    static let accessoryButtonSize: CGFloat = 44
    static let stackSpacing: CGFloat = 0
    static let infoLabelHorizontalInset: CGFloat = 16
  }

  enum Style {
    case regular
    case link
    case header
  }

  typealias TapHandler = () -> Void

  private let contentStackView = UIStackView()
  private var textLabelLeadingConstraint: NSLayoutConstraint!

  let iconButton = UIButton()
  let textLabel = InsetsLabel()
  let accessoryButton = UIButton()

  var infoLabelTapHandler: TapHandler?
  var infoLabelLongPressHandler: TapHandler?
  var iconButtonTapHandler: TapHandler?
  var accessoryImageTapHandler: TapHandler?

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupView()
    layout()
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    setupView()
    layout()
  }

  private func setupView() {
    addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(onTextLabelTap)))
    addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(onTextLabelLongPress(_:))))

    textLabel.lineBreakMode = .byWordWrapping
    textLabel.numberOfLines = 0
    textLabel.isUserInteractionEnabled = false

    iconButton.imageView?.contentMode = .scaleAspectFit
    iconButton.addTarget(self, action: #selector(onIconButtonTap), for: .touchUpInside)
    iconButton.contentEdgeInsets = Constants.iconButtonEdgeInsets
    iconButton.isHidden = true

    accessoryButton.addTarget(self, action: #selector(onAccessoryButtonTap), for: .touchUpInside)

    contentStackView.axis = .horizontal
    contentStackView.spacing = Constants.stackSpacing
    contentStackView.alignment = .center
    contentStackView.distribution = .fill
  }

  private func layout() {
    contentStackView.addArrangedSubview(iconButton)
    contentStackView.addArrangedSubview(textLabel)
    contentStackView.addArrangedSubview(accessoryButton)
    addSubview(contentStackView)

    contentStackView.translatesAutoresizingMaskIntoConstraints = false
    iconButton.translatesAutoresizingMaskIntoConstraints = false
    textLabel.translatesAutoresizingMaskIntoConstraints = false
    accessoryButton.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      contentStackView.leadingAnchor.constraint(equalTo: leadingAnchor),
      contentStackView.trailingAnchor.constraint(equalTo: trailingAnchor),
      contentStackView.topAnchor.constraint(equalTo: topAnchor),
      contentStackView.bottomAnchor.constraint(equalTo: bottomAnchor),

      iconButton.widthAnchor.constraint(equalToConstant: Constants.iconButtonSize),
      iconButton.heightAnchor.constraint(equalTo: contentStackView.heightAnchor),

      accessoryButton.widthAnchor.constraint(equalToConstant: Constants.accessoryButtonSize),
      accessoryButton.heightAnchor.constraint(equalTo: contentStackView.heightAnchor),

      textLabel.heightAnchor.constraint(greaterThanOrEqualToConstant: Constants.infoLabelMinHeight),
      textLabel.topAnchor.constraint(equalTo: contentStackView.topAnchor, constant: Constants.contentInsets.top),
      textLabel.bottomAnchor.constraint(equalTo: contentStackView.bottomAnchor, constant: Constants.contentInsets.bottom)
    ])
  }

  private func updateTextLabelLayout() {
    textLabel.insets.left = iconButton.isHidden ? Constants.infoLabelHorizontalInset : 0
    textLabel.insets.right = accessoryButton.isHidden ? Constants.infoLabelHorizontalInset : 0
  }

  // MARK: - Actions

  @objc
  private func onTextLabelTap() {
    infoLabelTapHandler?()
  }

  @objc
  private func onTextLabelLongPress(_ sender: UILongPressGestureRecognizer) {
    guard sender.state == .began else { return }
    infoLabelLongPressHandler?()
  }

  @objc
  private func onIconButtonTap() {
    iconButtonTapHandler?()
  }

  @objc
  private func onAccessoryButtonTap() {
    accessoryImageTapHandler?()
  }

  // MARK: - Public methods

  func setTitle(_ title: String, style: Style = .regular, tapHandler: TapHandler? = nil, longPressHandler: TapHandler? = nil) {
    switch style {
    case .regular:
      textLabel.text = title
      textLabel.setFontStyleAndApply(.regular16, color: .blackPrimary)
      iconButton.setStyleAndApply(.black)
    case .link:
      textLabel.text = title
      textLabel.setFontStyleAndApply(.regular16, color: .linkBlue)
      iconButton.setStyleAndApply(.blue)
    case .header:
      textLabel.text = title.uppercased()
      textLabel.setFontStyleAndApply(.regular14, color: .blackSecondary)
      iconButton.setStyleAndApply(.black)
    }
    if let tapHandler {
      infoLabelTapHandler = tapHandler
    }
    if let longPressHandler {
      infoLabelLongPressHandler = longPressHandler
    }
    updateTextLabelLayout()
  }

  func setIcon(image: UIImage?, tapHandler: TapHandler? = nil) {
    if let image {
      iconButton.setImage(image, for: .normal)
      iconButton.isHidden = false
      if let tapHandler {
        iconButtonTapHandler = tapHandler
      }
    } else {
      iconButton.setImage(nil, for: .normal)
      iconButton.isHidden = true
      iconButtonTapHandler = nil
    }
    updateTextLabelLayout()
  }

  func setAccessory(image: UIImage?, tapHandler: TapHandler? = nil) {
    if let image {
      accessoryButton.setTitle("", for: .normal)
      accessoryButton.setImage(image, for: .normal)
      accessoryButton.isHidden = false
      if let tapHandler {
        accessoryImageTapHandler = tapHandler
        accessoryButton.setStyleAndApply(.blue)
      } else {
        accessoryButton.setStyleAndApply(.black)
      }
    } else {
      accessoryButton.setTitle("", for: .normal)
      accessoryButton.setImage(nil, for: .normal)
      accessoryButton.isHidden = true
      accessoryImageTapHandler = nil
    }
    updateTextLabelLayout()
  }

  @available(iOS 14.0, *)
  func setAccessoryMenu(_ menu: UIMenu) {
    accessoryButton.menu = menu
    accessoryButton.showsMenuAsPrimaryAction = true
    accessoryButton.setStyleAndApply(.blue)
  }
}
