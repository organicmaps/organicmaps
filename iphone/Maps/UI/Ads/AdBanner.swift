import AlamofireImage
import FBAudienceNetwork

@objc(MWMAdBannerState)
enum AdBannerState: Int {
  case unset
  case compact
  case detailed

  func config() -> (priority: UILayoutPriority, numberOfTitleLines: Int, numberOfBodyLines: Int) {
    switch self {
    case .unset:
      assert(false)
      return (priority: 0, numberOfTitleLines: 0, numberOfBodyLines: 0)
    case .compact:
      return alternative(iPhone: (priority: UILayoutPriorityDefaultLow, numberOfTitleLines: 1, numberOfBodyLines: 2),
                         iPad: (priority: UILayoutPriorityDefaultHigh, numberOfTitleLines: 0, numberOfBodyLines: 0))
    case .detailed:
      return (priority: UILayoutPriorityDefaultHigh, numberOfTitleLines: 0, numberOfBodyLines: 0)
    }
  }
}

@objc(MWMAdBanner)
final class AdBanner: UITableViewCell {
  @IBOutlet private var detailedModeConstraints: [NSLayoutConstraint]!
  @IBOutlet private weak var adIconImageView: UIImageView!
  @IBOutlet private weak var adTitleLabel: UILabel!
  @IBOutlet private weak var adBodyLabel: UILabel!
  @IBOutlet private weak var adCallToActionButtonCompact: UIButton!
  @IBOutlet private weak var adCallToActionButtonDetailed: UIButton!
  static let detailedBannerExcessHeight: Float = 36

  var state = AdBannerState.unset {
    didSet {
      let config = state.config()
      adTitleLabel.numberOfLines = config.numberOfTitleLines
      adBodyLabel.numberOfLines = config.numberOfBodyLines
      detailedModeConstraints.forEach { $0.priority = config.priority }
      setNeedsLayout()
      UIView.animate(withDuration: kDefaultAnimationDuration) { self.layoutIfNeeded() }
      refreshBannerIfNeeded()
    }
  }

  weak var mpNativeAd: MPNativeAd?

  override func prepareForReuse() {
    adIconImageView.af_cancelImageRequest()
  }

  private var nativeAd: MWMBanner?

  func config(ad: MWMBanner) {
    state = alternative(iPhone: AdBannerState.compact, iPad: AdBannerState.detailed)
    nativeAd = ad
    switch ad.mwmType {
    case .none:
      assert(false)
    case .facebook:
      configFBBanner(ad: ad as! FBNativeAd)
    case .rb:
      configRBBanner(ad: ad as! MTRGNativeAd)
    case .mopub:
      configMopubBanner(ad: ad as! MopubBanner)
    }
  }

  func highlightButton() {
    adCallToActionButtonDetailed.setBackgroundImage(nil, for: .normal)
    adCallToActionButtonCompact.setBackgroundImage(nil, for: .normal)

    adCallToActionButtonDetailed.backgroundColor = UIColor.bannerButtonBackground()
    adCallToActionButtonCompact.backgroundColor = UIColor.bannerBackground()
    let duration = 0.5 * kDefaultAnimationDuration
    let darkerPercent: CGFloat = 0.2
    UIView.animate(withDuration: duration, animations: {
      self.adCallToActionButtonDetailed.backgroundColor = UIColor.bannerButtonBackground().darker(percent: darkerPercent)
      self.adCallToActionButtonCompact.backgroundColor = UIColor.bannerBackground().darker(percent: darkerPercent)
    }, completion: { _ in
      UIView.animate(withDuration: duration, animations: {
        self.adCallToActionButtonDetailed.backgroundColor = UIColor.bannerButtonBackground()
        self.adCallToActionButtonCompact.backgroundColor = UIColor.bannerBackground()
      }, completion: { _ in
        self.adCallToActionButtonDetailed.setBackgroundColor(UIColor.bannerButtonBackground(), for: .normal)
        self.adCallToActionButtonCompact.setBackgroundColor(UIColor.bannerBackground(), for: .normal)
      })
    })
  }

  private func configFBBanner(ad: FBNativeAd) {
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

  private func configRBBanner(ad: MTRGNativeAd) {
    ad.unregisterView()

    guard let banner = ad.banner else { return }

    ad.loadIcon(to: adIconImageView)

    let paragraphStyle = NSMutableParagraphStyle()
    paragraphStyle.firstLineHeadIndent = 24
    paragraphStyle.lineBreakMode = .byTruncatingTail
    let adTitle = NSAttributedString(string: banner.title ?? "",
                                     attributes: [NSParagraphStyleAttributeName: paragraphStyle,
                                                  NSFontAttributeName: UIFont.bold12(),
                                                  NSForegroundColorAttributeName: UIColor.blackSecondaryText()])
    adTitleLabel.attributedText = adTitle
    adBodyLabel.text = banner.descriptionText ?? ""
    let config = state.config()
    adTitleLabel.numberOfLines = config.numberOfTitleLines
    adBodyLabel.numberOfLines = config.numberOfBodyLines

    [adCallToActionButtonCompact, adCallToActionButtonDetailed].forEach { $0.setTitle(banner.ctaText, for: .normal) }
    refreshBannerIfNeeded()
  }

  private func configMopubBanner(ad: MopubBanner) {
    mpNativeAd = ad.nativeAd
    mpNativeAd?.setAdView(self)

    let adCallToActionButtons = [adCallToActionButtonCompact!, adCallToActionButtonDetailed!]
    mpNativeAd?.setActionButtons(adCallToActionButtons)

    let paragraphStyle = NSMutableParagraphStyle()
    paragraphStyle.firstLineHeadIndent = 24
    paragraphStyle.lineBreakMode = .byTruncatingTail
    let adTitle = NSAttributedString(string: ad.title,
                                     attributes: [NSParagraphStyleAttributeName: paragraphStyle,
                                                  NSFontAttributeName: UIFont.bold12(),
                                                  NSForegroundColorAttributeName: UIColor.blackSecondaryText()])
    adTitleLabel.attributedText = adTitle
    adBodyLabel.text = ad.text
    if let url = URL(string: ad.iconURL) {
      adIconImageView.af_setImage(withURL: url)
    }

    adCallToActionButtons.forEach { $0.setTitle(ad.ctaText, for: .normal) }
  }

  private func refreshBannerIfNeeded() {
    if let ad = nativeAd as? MTRGNativeAd {
      let clickableView: UIView
      switch state {
      case .unset:
        assert(false)
        clickableView = adCallToActionButtonCompact
      case .compact: clickableView = adCallToActionButtonCompact
      case .detailed: clickableView = adCallToActionButtonDetailed
      }
      ad.register(clickableView, with: UIViewController.topViewController())
    }
  }

  override func willMove(toSuperview newSuperview: UIView?) {
    super.willMove(toSuperview: newSuperview)
    mpNativeAd?.nativeViewWillMove(toSuperview: newSuperview)
  }
}
