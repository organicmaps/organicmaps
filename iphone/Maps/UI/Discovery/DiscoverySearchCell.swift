@objc(MWMDiscoverySearchCell)
final class DiscoverySearchCell: UICollectionViewCell {
  @IBOutlet private weak var titleLabel: UILabel!
  @IBOutlet private weak var subtitleLabel: UILabel!
  @IBOutlet private weak var distanceLabel: UILabel!
  @IBOutlet private weak var popularView: UIView! {
    didSet {
      popularView.tintColor = UIColor.linkBlue()
    }
  }
  
  @IBOutlet private weak var ratingView: RatingSummaryView! {
    didSet {
      ratingView.defaultConfig()
      ratingView.textFont = UIFont.bold12()
      ratingView.textSize = 12
    }
  }

  typealias Tap = () -> ()
  private var tap: Tap?

  @objc func config(title: String,
                    subtitle: String,
                    distance: String,
                    popular: Bool,
                    ratingValue: String,
                    ratingType: MWMRatingSummaryViewValueType,
                    tap: @escaping Tap) {
    titleLabel.text = title
    subtitleLabel.text = subtitle
    distanceLabel.text = distance
    popularView.isHidden = !popular
    ratingView.value = ratingValue
    ratingView.type = ratingType
    self.tap = tap
  }

  @IBAction private func routeTo() {
    tap?()
  }

  override var isHighlighted: Bool {
    didSet {
      UIView.animate(withDuration: kDefaultAnimationDuration,
                     delay: 0,
                     options: [.allowUserInteraction, .beginFromCurrentState],
                     animations: { self.alpha = self.isHighlighted ? 0.3 : 1 },
                     completion: nil)
    }
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    layer.borderColor = UIColor.blackDividers().cgColor
  }
}
