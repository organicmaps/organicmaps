import FBAudienceNetwork

@objc(MWMFBAdsBannerState)
enum FBAdsBannerState: Int {
  case compact
  case detailed

  func config() -> (priority: UILayoutPriority, numberOfTitleLines: Int, numberOfBodyLines: Int) {
    switch self {
    case .compact:
      return alternative(iPhone: (priority: UILayoutPriorityDefaultLow, numberOfTitleLines: 1, numberOfBodyLines: 2),
                         iPad: (priority: UILayoutPriorityDefaultHigh, numberOfTitleLines: 0, numberOfBodyLines: 0))
    case .detailed:
      return (priority: UILayoutPriorityDefaultHigh, numberOfTitleLines: 0, numberOfBodyLines: 0)
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
      adTitleLabel.numberOfLines = config.numberOfTitleLines
      adBodyLabel.numberOfLines = config.numberOfBodyLines
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

    let paragraphStyle = NSMutableParagraphStyle()
    paragraphStyle.firstLineHeadIndent = 24
    paragraphStyle.lineBreakMode = .byTruncatingTail
    let adTitle = NSAttributedString(string: ad.title ?? "",
                                     attributes: [NSParagraphStyleAttributeName: paragraphStyle,
                                                  NSFontAttributeName: UIFont.bold12(),
                                                  NSForegroundColorAttributeName: UIColor.blackSecondaryText()])
    adTitleLabel.attributedText = adTitle
    adBodyLabel.text = ad.body ?? ""
    let config = state.config()
    adTitleLabel.numberOfLines = config.numberOfTitleLines
    adBodyLabel.numberOfLines = config.numberOfBodyLines
    adCallToActionButtons.forEach { $0.setTitle(ad.callToAction, for: .normal) }
  }
}
