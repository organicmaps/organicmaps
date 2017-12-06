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

  typealias Tap = () -> ()
  private var tap: Tap!

  @objc func config(avatarURL: String,
                    name: String,
                    ratingValue: String,
                    ratingType: MWMRatingSummaryViewValueType,
                    price: Double,
                    currency: String,
                    tap: @escaping Tap) {
    if avatarURL.count > 0 {
      avatar.af_setImage(withURL: URL(string: avatarURL)!, imageTransition: .crossDissolve(kDefaultAnimationDuration))
    } else {
      avatar.image = nil
    }
    self.name.text = name
    rating.value = ratingValue
    rating.type = ratingType
    let str: String
    if currency.count == 0 {
      str = L("free")
    } else {
      str = String(coreFormat: L("price_per_hour"), arguments:[String(price) + currency])
    }

    self.price.setTitle(str, for: .normal)
    self.tap = tap
  }

  @IBAction private func tapOnPrice() {
    tap?()
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    layer.borderColor = UIColor.blackDividers().cgColor
  }
}
