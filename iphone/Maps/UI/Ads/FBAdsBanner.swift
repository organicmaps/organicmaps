import FBAudienceNetwork

@objc(MWMFBAdsBannerState)
enum FBAdsBannerState: Int {
  case compact
  case detailed

  func config() -> (priority: UILayoutPriority, numberOfLines: Int) {
    switch self {
    case .compact:
      return alternative(iPhone: (priority: UILayoutPriorityDefaultLow, numberOfLines: 2),
                         iPad: (priority: UILayoutPriorityDefaultHigh, numberOfLines: 0))
    case .detailed:
      return (priority: UILayoutPriorityDefaultHigh, numberOfLines: 0)
    }
  }
}

@objc(MWMFBAdsBanner)
final class FBAdsBanner: UITableViewCell {
  @IBOutlet private var detailedModeConstraints: [NSLayoutConstraint]!
  @IBOutlet private weak var adIconImageView: UIImageView!
  @IBOutlet private weak var adTitleLabel: UILabel!
  @IBOutlet private weak var adBodyLabel: UILabel!
  @IBOutlet private var adCallToActionButtonCompact: UIButton!
  @IBOutlet private var adCallToActionButtonDetailed: UIButton!
  static let detailedBannerExcessHeight: Float = 36

  var state = alternative(iPhone: FBAdsBannerState.compact, iPad: FBAdsBannerState.detailed) {
    didSet {
      let config = state.config()
      adBodyLabel.numberOfLines = config.numberOfLines
      detailedModeConstraints.forEach { $0.priority = config.priority }
      setNeedsLayout()
      UIView.animate(withDuration: kDefaultAnimationDuration) { self.layoutIfNeeded() }
    }
  }

  private var nativeAd: FBNativeAd?

  func config(ad: FBNativeAd) {
    nativeAd = ad
    ad.unregisterView()
    let adCallToActionButtons = [adCallToActionButtonCompact!, adCallToActionButtonDetailed!]
    ad.registerView(forInteraction: self, with: nil, withClickableViews: adCallToActionButtons)

    ad.icon?.loadAsync { [weak self] image in
      self?.adIconImageView.image = image
    }

    adTitleLabel.text = ad.title
    adBodyLabel.text = ad.body
    adCallToActionButtons.forEach { $0.setTitle(ad.callToAction, for: .normal) }
  }
}
