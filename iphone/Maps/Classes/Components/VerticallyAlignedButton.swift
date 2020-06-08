@IBDesignable
class VerticallyAlignedButton: UIControl {
  @IBInspectable
  var image: UIImage? {
    didSet {
      imageView.image = image
    }
  }

  @IBInspectable
  var title: String? {
    didSet {
      if localizedText == nil {
        titleLabel.text = title
      }
    }
  }

  @IBInspectable
  var localizedText: String? {
    didSet {
      if let localizedText = localizedText {
        titleLabel.text = L(localizedText)
      }
    }
  }

  @IBInspectable
  var spacing: CGFloat = 4 {
    didSet {
      spacingConstraint.constant = spacing
    }
  }

  @IBInspectable
  var numberOfLines: Int {
    get {
      return titleLabel.numberOfLines
    }
    set {
      titleLabel.numberOfLines = newValue
    }
  }

  private lazy var spacingConstraint: NSLayoutConstraint = {
    let spacingConstraint = titleLabel.topAnchor.constraint(equalTo: imageView.bottomAnchor, constant: spacing)
    return spacingConstraint
  }()

  lazy var titleLabel: UILabel = {
    let titleLabel = UILabel()
    titleLabel.textAlignment = .center
    titleLabel.translatesAutoresizingMaskIntoConstraints = false
    return titleLabel
  }()

  lazy var imageView: UIImageView = {
    let imageView = UIImageView()
    imageView.contentMode = .center
    imageView.translatesAutoresizingMaskIntoConstraints = false
    return imageView
  }()

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupView()
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    setupView()
  }

  private func setupView() {
    addSubview(titleLabel)
    addSubview(imageView)

    NSLayoutConstraint.activate([
      imageView.topAnchor.constraint(equalTo: topAnchor),
      imageView.centerXAnchor.constraint(equalTo: centerXAnchor),
      titleLabel.leadingAnchor.constraint(equalTo: leadingAnchor),
      titleLabel.trailingAnchor.constraint(equalTo: trailingAnchor),
      spacingConstraint
    ])
  }
}
