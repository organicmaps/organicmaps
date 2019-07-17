class BookmarksSubscriptionCellViewController: UIViewController {
  @IBOutlet private var subtitleLabel: UILabel!
  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var priceLabel: UILabel!
  @IBOutlet private var discountView: UIView!
  @IBOutlet private var discountLabel: UILabel!
  @IBOutlet private var imageView: UIImageView!
  @IBOutlet private var containerView: UIView!
  @IBOutlet private var contentView: UIView!

  private(set) var isSelected = false

  override func viewDidLoad() {
    super.viewDidLoad()
    if !UIColor.isNightMode() {
      setupShadow(containerView, color: .shadowColorBlue())
      setupShadow(discountView, color: .shadowColorPurple())
    }
    contentView.layer.borderColor = UIColor.subscriptionCellBorder().cgColor
    setSelected(false, animated: false)
  }

  func setSelected(_ selected: Bool, animated: Bool = false) {
    isSelected = selected
    let setSelectedClosure = { [unowned self] in
      self.titleLabel.textColor = selected ? .linkBlue() : .blackPrimaryText()
      self.subtitleLabel.textColor = selected ? .linkBlue() : .blackSecondaryText()
      self.priceLabel.textColor = selected ? .linkBlue() : .blackPrimaryText()
      self.contentView.backgroundColor = selected ? .linkBlueHighlighted() : .clear
      self.containerView.backgroundColor = selected ? .white() : .clear
    }

    if animated {
      UIView.animate(withDuration: kDefaultAnimationDuration) {
        setSelectedClosure()
      }
      animateBorder(contentView, show: !selected)
      if !UIColor.isNightMode() {
        animateShadow(containerView, opacity: selected ? 0.5 : 0)
        animateShadow(discountView, opacity: selected ? 0.62 : 0)
      }
    } else {
      setSelectedClosure()
      contentView.layer.borderWidth = selected ? 0 : 1
      if !UIColor.isNightMode() {
        containerView.layer.shadowOpacity = selected ? 0.5 : 0
        discountView.layer.shadowOpacity = selected ? 0.62 : 0
      }
    }
  }

  func config(title: String, subtitle: String, price: String, image: UIImage, discount: String? = nil) {
    titleLabel.text = title
    subtitleLabel.text = subtitle
    priceLabel.text = price
    imageView.image = image

    guard let discount = discount else {
      discountView.isHidden = true
      return
    }
    
    discountLabel.text = discount
    discountView.isHidden = false
  }

  private func setupShadow(_ view: UIView, color: UIColor) {
    view.layer.shadowRadius = 4
    view.layer.shadowOffset = CGSize(width: 0, height: 2)
    view.layer.shadowColor = color.cgColor
  }

  private func animateShadow(_ view: UIView, opacity: Float) {
    let shadowOpacityKey = "shadowOpacity"
    let animation = CABasicAnimation(keyPath: shadowOpacityKey)
    animation.fromValue = view.layer.shadowOpacity
    animation.duration = kDefaultAnimationDuration
    view.layer.add(animation, forKey: shadowOpacityKey)
    view.layer.shadowOpacity = opacity
  }

  private func animateBorder(_ view: UIView, show: Bool) {
    let borderWidthKey = "borderWidth"
    let animation = CABasicAnimation(keyPath: borderWidthKey)
    animation.fromValue = view.layer.borderWidth
    animation.duration = kDefaultAnimationDuration
    view.layer.add(animation, forKey: borderWidthKey)
    view.layer.borderWidth = show ? 1 : 0
  }
}
