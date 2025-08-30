final class InfoItemView: UIView {

  private enum Constants {
    static let contentInsets = UIEdgeInsets(top: 8, left: 0, bottom: -8, right: 0)
    static let iconButtonSize: CGFloat = 56
    static let iconButtonEdgeInsets = UIEdgeInsets(top: 8, left: 0, bottom: 8, right: 0)
    static let infoLabelMinHeight: CGFloat = 28
    static let accessoryButtonSize: CGFloat = 44
  }

  enum Style {
    case regular
    case link
  }

  typealias TapHandler = () -> Void

  let iconButton = UIButton()
  let infoLabel = UILabel()
  let accessoryButton = UIButton()

  var infoLabelTapHandler: TapHandler?
  var infoLabelLongPressHandler: TapHandler?
  var iconButtonTapHandler: TapHandler?
  var accessoryImageTapHandler: TapHandler?

  private var style: Style = .regular

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
    addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(onInfoLabelTap)))
    addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(onInfoLabelLongPress(_:))))

    infoLabel.lineBreakMode = .byWordWrapping
    infoLabel.numberOfLines = 0
    infoLabel.isUserInteractionEnabled = false

    iconButton.imageView?.contentMode = .scaleAspectFit
    iconButton.addTarget(self, action: #selector(onIconButtonTap), for: .touchUpInside)
    iconButton.contentEdgeInsets = Constants.iconButtonEdgeInsets

    accessoryButton.addTarget(self, action: #selector(onAccessoryButtonTap), for: .touchUpInside)
  }

  private func layout() {
    addSubview(iconButton)
    addSubview(infoLabel)
    addSubview(accessoryButton)

    iconButton.translatesAutoresizingMaskIntoConstraints = false
    infoLabel.translatesAutoresizingMaskIntoConstraints = false
    accessoryButton.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      iconButton.leadingAnchor.constraint(equalTo: leadingAnchor),
      iconButton.centerYAnchor.constraint(equalTo: centerYAnchor),
      iconButton.widthAnchor.constraint(equalToConstant: Constants.iconButtonSize),
      iconButton.topAnchor.constraint(equalTo: topAnchor),
      iconButton.bottomAnchor.constraint(equalTo: bottomAnchor),

      infoLabel.leadingAnchor.constraint(equalTo: iconButton.trailingAnchor),
      infoLabel.topAnchor.constraint(equalTo: topAnchor, constant: Constants.contentInsets.top),
      infoLabel.bottomAnchor.constraint(equalTo: bottomAnchor, constant: Constants.contentInsets.bottom),
      infoLabel.trailingAnchor.constraint(equalTo: accessoryButton.leadingAnchor),
      infoLabel.heightAnchor.constraint(greaterThanOrEqualToConstant: Constants.infoLabelMinHeight),

      accessoryButton.trailingAnchor.constraint(equalTo: trailingAnchor),
      accessoryButton.centerYAnchor.constraint(equalTo: centerYAnchor),
      accessoryButton.widthAnchor.constraint(equalToConstant: Constants.accessoryButtonSize),
      accessoryButton.topAnchor.constraint(equalTo: topAnchor),
      accessoryButton.bottomAnchor.constraint(equalTo: bottomAnchor)
    ])
  }

  @objc
  private func onInfoLabelTap() {
    infoLabelTapHandler?()
  }

  @objc
  private func onInfoLabelLongPress(_ sender: UILongPressGestureRecognizer) {
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

  func setStyle(_ style: Style) {
    switch style {
    case .regular:
      iconButton.setStyleAndApply(.black)
      infoLabel.setFontStyleAndApply(.regular16, color: .blackPrimary)
      accessoryButton.setStyleAndApply(.black)
    case .link:
      iconButton.setStyleAndApply(.blue)
      infoLabel.setFontStyleAndApply(.regular16, color: .linkBlue)
      accessoryButton.setStyleAndApply(.blue)
    }
    self.style = style
  }

  func setAccessory(image: UIImage?, tapHandler: TapHandler? = nil) {
    accessoryButton.setTitle("", for: .normal)
    accessoryButton.setImage(image, for: .normal)
    accessoryButton.isHidden = image == nil
    accessoryImageTapHandler = tapHandler
  }
}
