import AlamofireImage
import FBAudienceNetwork

@objc(MWMAdBannerState)
enum AdBannerState: Int {
  case unset
  case compact
  case detailed
  case search

  func config() -> (priority: UILayoutPriority, numberOfTitleLines: Int, numberOfBodyLines: Int) {
    switch self {
    case .unset:
      assert(false)
      return (priority: 0, numberOfTitleLines: 0, numberOfBodyLines: 0)
    case .compact:
      return alternative(iPhone: (priority: UILayoutPriorityDefaultLow, numberOfTitleLines: 1, numberOfBodyLines: 2),
                         iPad: (priority: UILayoutPriorityDefaultHigh, numberOfTitleLines: 0, numberOfBodyLines: 0))
    case .search:
      return (priority: UILayoutPriorityDefaultLow, numberOfTitleLines: 2, numberOfBodyLines: 0)
    case .detailed:
      return (priority: UILayoutPriorityDefaultHigh, numberOfTitleLines: 0, numberOfBodyLines: 0)
    }
  }
}

@objc(MWMAdBannerContainerType)
enum AdBannerContainerType: Int {
  case placePage
  case search
}

@objc(MWMAdBanner)
final class AdBanner: UITableViewCell {
  @IBOutlet private var detailedModeConstraints: [NSLayoutConstraint]!
  @IBOutlet private weak var adCallToActionButtonCompactLeading: NSLayoutConstraint!
  @IBOutlet private weak var adIconImageView: UIImageView!
  @IBOutlet private weak var adTitleLabel: UILabel!
  @IBOutlet private weak var adBodyLabel: UILabel!
  @IBOutlet private weak var adCallToActionButtonCompact: UIButton!
  @IBOutlet private weak var adCallToActionButtonDetailed: UIButton!
  @IBOutlet private weak var adCallToActionButtonCustom: UIButton!
  @IBOutlet private weak var adPrivacyButton: UIButton!
  static let detailedBannerExcessHeight: Float = 36

  var state = AdBannerState.unset {
    didSet {
      guard state != .unset else {
        adPrivacyButton.isHidden = true
        adCallToActionButtonCustom.isHidden = true
        mpNativeAd = nil
        nativeAd = nil
        return
      }
      guard state != oldValue else { return }
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

  private var nativeAd: Banner? {
    willSet {
      nativeAd?.unregister()
    }
  }

  @IBAction
  private func privacyAction() {
    if let ad = nativeAd as? MopubBanner, let urlStr = ad.privacyInfoURL, let url = URL(string: urlStr) {
      UIViewController.topViewController().open(url)
    }
  }

  func reset() {
    state = .unset
  }

  func config(ad: MWMBanner, containerType: AdBannerContainerType) {
    reset()
    switch containerType {
    case .placePage:
      state = alternative(iPhone: .compact, iPad: .detailed)
    case .search:
      state = .search
    }

    nativeAd = ad as? Banner
    switch ad.mwmType {
    case .none:
      assert(false)
    case .facebook:
      configFBBanner(ad: (ad as! FacebookBanner).nativeAd)
    case .rb:
      configRBBanner(ad: ad as! MTRGNativeAd)
    case .mopub:
      configMopubBanner(ad: ad as! MopubBanner)
    case .google:
      assert(false)
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
    let adCallToActionButtons: [UIView]
    if state == .search {
      adCallToActionButtons = [self, adCallToActionButtonCompact]
    } else {
      adCallToActionButtons = [adCallToActionButtonCompact, adCallToActionButtonDetailed]
    }
    ad.registerView(forInteraction: self, with: nil, withClickableViews: adCallToActionButtons)

    ad.icon?.loadAsync { [weak self] image in
      self?.adIconImageView.image = image
    }

    let paragraphStyle = NSMutableParagraphStyle()
    paragraphStyle.firstLineHeadIndent = 24
    paragraphStyle.lineBreakMode = .byTruncatingTail
    let adTitle = NSAttributedString(string: ad.title ?? "",
                                     attributes: [
                                       NSParagraphStyleAttributeName: paragraphStyle,
                                       NSFontAttributeName: UIFont.bold12(),
                                       NSForegroundColorAttributeName: UIColor.blackSecondaryText(),
    ])
    adTitleLabel.attributedText = adTitle
    adBodyLabel.text = ad.body ?? ""
    let config = state.config()
    adTitleLabel.numberOfLines = config.numberOfTitleLines
    adBodyLabel.numberOfLines = config.numberOfBodyLines
    [adCallToActionButtonCompact, adCallToActionButtonDetailed].forEach { $0.setTitle(ad.callToAction, for: .normal) }
  }

  private func configRBBanner(ad: MTRGNativeAd) {
    guard let banner = ad.banner else { return }

    MTRGNativeAd.loadImage(banner.icon, to: adIconImageView)

    let paragraphStyle = NSMutableParagraphStyle()
    paragraphStyle.firstLineHeadIndent = 24
    paragraphStyle.lineBreakMode = .byTruncatingTail
    let adTitle = NSAttributedString(string: banner.title ?? "",
                                     attributes: [
                                       NSParagraphStyleAttributeName: paragraphStyle,
                                       NSFontAttributeName: UIFont.bold12(),
                                       NSForegroundColorAttributeName: UIColor.blackSecondaryText(),
    ])
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

    let adCallToActionButtons: [UIButton]
    if state == .search {
      adCallToActionButtonCustom.isHidden = false
      adCallToActionButtons = [adCallToActionButtonCustom, adCallToActionButtonCompact]
    } else {
      adCallToActionButtons = [adCallToActionButtonCompact, adCallToActionButtonDetailed]
      adCallToActionButtons.forEach { $0.setTitle(ad.ctaText, for: .normal) }
    }
    mpNativeAd?.setAdView(self, actionButtons: adCallToActionButtons)

    let paragraphStyle = NSMutableParagraphStyle()
    paragraphStyle.firstLineHeadIndent = 24
    paragraphStyle.lineBreakMode = .byTruncatingTail
    let adTitle = NSAttributedString(string: ad.title,
                                     attributes: [
                                       NSParagraphStyleAttributeName: paragraphStyle,
                                       NSFontAttributeName: UIFont.bold12(),
                                       NSForegroundColorAttributeName: UIColor.blackSecondaryText(),
    ])
    adTitleLabel.attributedText = adTitle
    adBodyLabel.text = ad.text
    if let url = URL(string: ad.iconURL) {
      adIconImageView.af_setImage(withURL: url)
    }
    adPrivacyButton.isHidden = ad.privacyInfoURL == nil
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
      case .search: clickableView = self
      }
      ad.register(clickableView, with: UIViewController.topViewController())
    }
  }

  override func willMove(toSuperview newSuperview: UIView?) {
    super.willMove(toSuperview: newSuperview)
    mpNativeAd?.nativeViewWillMove(toSuperview: newSuperview)
  }
}
