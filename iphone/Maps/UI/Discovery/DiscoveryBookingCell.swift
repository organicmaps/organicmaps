@objc(MWMDiscoveryBookingCell)
final class DiscoveryBookingCell: UICollectionViewCell {
  @IBOutlet private weak var avatar: UIImageView!
  @IBOutlet private weak var name: UILabel! {
    didSet {
      name.font = UIFont.medium14()
      name.textColor = UIColor.blackPrimaryText()
    }
  }

  @IBOutlet private weak var stars: UILabel! {
    didSet {
      stars.font = UIFont.regular12()
      stars.textColor = UIColor.blackSecondaryText()
    }
  }

  @IBOutlet private weak var price: UILabel! {
    didSet {
      price.font = UIFont.medium14()
      price.textColor = UIColor.blackSecondaryText()
    }
  }

  @IBOutlet private weak var rating: RatingSummaryView! {
    didSet {
      rating.defaultConfig()
      rating.textFont = UIFont.bold12()
      rating.textSize = 12
    }
  }

  @IBOutlet private weak var distance: UILabel! {
    didSet {
      distance.font = UIFont.medium14()
      distance.textColor = UIColor.linkBlue()
    }
  }

  @IBOutlet private weak var buildRoute: UIButton! {
    didSet {
      buildRoute.setTitleColor(UIColor.linkBlue(), for: .normal)
      buildRoute.setTitle(L("p2p_to_here"), for: .normal)
    }
  }

  typealias OnBuildRoute = () -> Void
  private var onBuildRoute: OnBuildRoute!

  override var isHighlighted: Bool {
    didSet {
      UIView.animate(withDuration: kDefaultAnimationDuration,
                     delay: 0,
                     options: [.allowUserInteraction, .beginFromCurrentState],
                     animations: { self.alpha = self.isHighlighted ? 0.3 : 1 },
                     completion: nil)
    }
  }

  private func setAvatar(_ avatarURL: String?) {
    guard let avatarURL = avatarURL else { return }
    if !avatarURL.isEmpty, let url = URL(string: avatarURL) {
      avatar.af_setImage(withURL: url, placeholderImage: #imageLiteral(resourceName: "img_localsdefault"), imageTransition: .crossDissolve(kDefaultAnimationDuration))
    } else {
      avatar.image = #imageLiteral(resourceName: "img_localsdefault")
    }
  }

  private func setRating(_ ratingValue: String, _ ratingType: MWMRatingSummaryViewValueType) {
    rating.value = ratingValue
    rating.type = ratingType
  }

  @objc func config(avatarURL: String?,
                    title: String,
                    subtitle: String,
                    price: String,
                    ratingValue: String,
                    ratingType: MWMRatingSummaryViewValueType,
                    distance: String,
                    onBuildRoute: @escaping OnBuildRoute) {
    setAvatar(avatarURL)
    self.name.text = title
    self.stars.text = subtitle
    self.price.text = price.isEmpty ? "" : "\(price) • "
    setRating(ratingValue, ratingType)
    self.distance.text = distance
    self.onBuildRoute = onBuildRoute
  }

  @IBAction private func buildRouteAction() {
    onBuildRoute()
  }

  override func awakeFromNib() {
    super.awakeFromNib()
    layer.borderColor = UIColor.blackDividers().cgColor
  }
}
