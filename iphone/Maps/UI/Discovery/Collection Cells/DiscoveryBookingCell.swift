@objc(MWMDiscoveryBookingCell)
final class DiscoveryBookingCell: UICollectionViewCell {
  @IBOutlet private weak var avatar: UIImageView!
  @IBOutlet private weak var name: UILabel!
  @IBOutlet private weak var stars: UILabel!
  @IBOutlet private weak var price: UILabel!
  @IBOutlet private weak var rating: RatingSummaryView!
  @IBOutlet private weak var distance: UILabel!

  @IBOutlet private weak var buildRoute: UIButton! {
    didSet {
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
      avatar.image = #imageLiteral(resourceName: "img_localsdefault")
      avatar.wi_setImage(with: url, transitionDuration: kDefaultAnimationDuration)
    } else {
      avatar.image = #imageLiteral(resourceName: "img_localsdefault")
    }
  }

  private func setRating(_ ratingValue: String, _ ratingType: UgcSummaryRatingType) {
    rating.value = ratingValue
    rating.type = ratingType
  }

  @objc func config(avatarURL: String?,
                    title: String,
                    subtitle: String,
                    price: String,
                    ratingValue: String,
                    ratingType: UgcSummaryRatingType,
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
  }
}
