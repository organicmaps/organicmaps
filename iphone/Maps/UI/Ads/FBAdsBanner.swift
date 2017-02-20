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
  @IBOutlet fileprivate weak var adIconImageView: UIImageView!
  @IBOutlet fileprivate weak var adTitleLabel: UILabel!
  @IBOutlet fileprivate weak var adBodyLabel: UILabel!
  @IBOutlet fileprivate var adCallToActionButtonCompact: UIButton!
  @IBOutlet fileprivate var adCallToActionButtonDetailed: UIButton!

  var state = FBAdsBannerState.compact {
    didSet {
      let config = state.config()
      layoutIfNeeded()
      adBodyLabel.numberOfLines = config.numberOfLines
      detailedModeConstraints.forEach { $0.priority = config.priority }
      UIView.animate(withDuration: kDefaultAnimationDuration) { self.layoutIfNeeded() }
    }
  }

  fileprivate var nativeAd: FBNativeAd?

  func config(placementID: String) {
    style()
    contentView.alpha = 0
    state = .compact
    let nativeAd = FBNativeAd(placementID: placementID)
    nativeAd.delegate = self
    nativeAd.mediaCachePolicy = .all
    nativeAd.load()
  }

  private func style() {
    adTitleLabel.font = UIFont.bold12()
    adBodyLabel.font = UIFont.regular12()
    adCallToActionButtonCompact.titleLabel?.font = UIFont.regular12()
    adCallToActionButtonDetailed.titleLabel?.font = UIFont.regular15()

    backgroundColor = UIColor.bannerBackground()
    let color = UIColor.blackSecondaryText()!
    adTitleLabel.textColor = color
    adBodyLabel.textColor = color
    adCallToActionButtonCompact.setTitleColor(color, for: .normal)
    adCallToActionButtonDetailed.setTitleColor(color, for: .normal)
    adCallToActionButtonDetailed.backgroundColor = UIColor.bannerButtonBackground()

    let layer = adCallToActionButtonCompact.layer
    layer.borderColor = color.cgColor
    layer.borderWidth = 1
    layer.cornerRadius = 4
  }
}

extension FBAdsBanner: FBNativeAdDelegate {
  func nativeAdDidLoad(_ nativeAd: FBNativeAd) {
    if let sNativeAd = self.nativeAd {
      sNativeAd.unregisterView()
    }
    self.nativeAd = nativeAd
    let adCallToActionButtons = [adCallToActionButtonCompact!, adCallToActionButtonDetailed!]
    nativeAd.registerView(forInteraction: self, with: nil, withClickableViews: adCallToActionButtons)

    nativeAd.icon?.loadAsync { [weak self] image in
      self?.adIconImageView.image = image
    }
    adTitleLabel.text = nativeAd.title
    adBodyLabel.text = nativeAd.body
    adCallToActionButtons.forEach { $0.setTitle(nativeAd.callToAction, for: .normal) }
    UIView.animate(withDuration: kDefaultAnimationDuration) { self.contentView.alpha = 1 }
  }
}
