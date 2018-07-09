@objc(MWMDiscoveryLocalExpertCell)
final class DiscoveryLocalExpertCell: UICollectionViewCell {
  @IBOutlet private weak var avatar: UIImageView!
  @IBOutlet private weak var name: UILabel!
  @IBOutlet private weak var rating: RatingSummaryView! {
    didSet {
      rating.defaultConfig()
      rating.textFont = UIFont.bold12()
      rating.textSize = 12
    }
  }
  @IBOutlet private weak var price: UIButton!

  private lazy var formatter: NumberFormatter = {
    let f = NumberFormatter()
    f.numberStyle = .currency
    return f
  }()

  typealias Tap = () -> ()
  private var tap: Tap!

  override var isHighlighted: Bool {
    didSet {
      UIView.animate(withDuration: kDefaultAnimationDuration,
                     delay: 0,
                     options: [.allowUserInteraction, .beginFromCurrentState],
                     animations: { self.alpha = self.isHighlighted ? 0.3 : 1 },
                     completion: nil)
    }
  }

  @objc func config(avatarURL: String,
                    name: String,
                    ratingValue: String,
                    ratingType: MWMRatingSummaryViewValueType,
                    price: Double,
                    currency: String,
                    tap: @escaping Tap) {
    if avatarURL.count > 0, let url = URL(string: avatarURL) {
      avatar.af_setImage(withURL: url, placeholderImage: #imageLiteral(resourceName: "img_localsdefault"), imageTransition: .crossDissolve(kDefaultAnimationDuration))
    } else {
      avatar.image = #imageLiteral(resourceName: "img_localsdefault")
    }
    
    self.name.text = name
    rating.value = ratingValue
    rating.type = ratingType
    let str: String
    if currency.count > 0, let cur = stringFor(price: price, currencyCode: currency) {
      str = String(coreFormat: L("price_per_hour"), arguments:[cur])
    } else {
      str = L("free")
    }

    self.price.setTitle(str, for: .normal)
    self.tap = tap
  }

  @IBAction private func tapOnPrice() {
    tap?()
  }

  private func stringFor(price: Double, currencyCode: String) -> String? {
    formatter.currencyCode = currencyCode
    return formatter.string(for: price)
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    layer.borderColor = UIColor.blackDividers().cgColor
  }
}
