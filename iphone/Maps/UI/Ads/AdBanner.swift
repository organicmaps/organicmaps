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
      return (priority: UILayoutPriority(rawValue: 0), numberOfTitleLines: 0, numberOfBodyLines: 0)
    case .compact:
      return alternative(iPhone: (priority: UILayoutPriority.defaultLow, numberOfTitleLines: 1, numberOfBodyLines: 2),
                         iPad: (priority: UILayoutPriority.defaultHigh, numberOfTitleLines: 0, numberOfBodyLines: 0))
    case .search:
      return (priority: UILayoutPriority.defaultLow, numberOfTitleLines: 2, numberOfBodyLines: 0)
    case .detailed:
      return (priority: UILayoutPriority.defaultHigh, numberOfTitleLines: 0, numberOfBodyLines: 0)
    }
  }
}

@objc(MWMAdBannerContainerType)
enum AdBannerContainerType: Int {
  case placePage
  case search
}

private func attributedTitle(title: String, indent: CGFloat) -> NSAttributedString {
  let paragraphStyle = NSMutableParagraphStyle()
  paragraphStyle.firstLineHeadIndent = indent
  paragraphStyle.lineBreakMode = .byTruncatingTail
  return NSAttributedString(string: title,
                        attributes: [NSAttributedStringKey.paragraphStyle: paragraphStyle,
                                     NSAttributedStringKey.font: UIFont.bold12(),
                                     NSAttributedStringKey.foregroundColor: UIColor.blackSecondaryText()
                                    ])
}

@objc(MWMAdBanner)
final class AdBanner: UITableViewCell {
  @IBOutlet private var detailedModeConstraints: [NSLayoutConstraint]!
  @IBOutlet private weak var adCallToActionButtonCompactLeading: NSLayoutConstraint!
  @IBOutlet private weak var adIconImageView: UIImageView!
  @IBOutlet private weak var adTitleLabel: UILabel!
  @IBOutlet private weak var adBodyLabel: UILabel!
  @IBOutlet private weak var DAAImage: UIImageView!
  @IBOutlet private weak var DAAImageWidth: NSLayoutConstraint!
  @IBOutlet private weak var adPrivacyImage: UIImageView!
  @IBOutlet private weak var adCallToActionButtonCompact: UIButton!
  @IBOutlet private weak var adCallToActionButtonDetailed: UIButton!
  @IBOutlet private weak var adCallToActionButtonCustom: UIButton!
  @IBOutlet private weak var nativeAdView: UIView!
  @IBOutlet private weak var fallbackAdView: UIView!
  @IBOutlet private var nativeAdViewBottom: NSLayoutConstraint!
  @IBOutlet private var fallbackAdViewBottom: NSLayoutConstraint!
  @IBOutlet private var fallbackAdViewHeight: NSLayoutConstraint!
  @objc static let detailedBannerExcessHeight: Float = 36

  enum AdType {
    case native
    case fallback
  }

  var adType = AdType.native {
    didSet {
      let isNative = adType == .native
      nativeAdView.isHidden = !isNative
      fallbackAdView.isHidden = isNative

      nativeAdViewBottom.isActive = isNative
      fallbackAdViewBottom.isActive = !isNative
      fallbackAdViewHeight.isActive = !isNative
    }
  }

  @objc var state = AdBannerState.unset {
    didSet {
      guard state != .unset else {
        adCallToActionButtonCustom.isHidden = true
        mpNativeAd = nil
        nativeAd = nil
        return
      }
      guard state != oldValue else { return }
      let config = state.config()
      animateConstraints(animations: {
        self.adTitleLabel.numberOfLines = config.numberOfTitleLines
        self.adBodyLabel.numberOfLines = config.numberOfBodyLines
        self.detailedModeConstraints.forEach { $0.priority = config.priority }
        self.refreshBannerIfNeeded()
      })
    }
  }

  @objc weak var mpNativeAd: MPNativeAd?

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
    let url: URL?
    if let ad = nativeAd as? FacebookBanner {
      url = ad.nativeAd.adChoicesLinkURL
    } else if let ad = nativeAd as? MopubBanner, let u = ad.privacyInfoURL {
      url = u
    } else {
      return
    }

    UIViewController.topViewController().open(url)
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    switch nativeAd {
    case let ad as GoogleFallbackBanner: updateFallbackBannerLayout(ad: ad)
    default: break
    }
  }

  func reset() {
    state = .unset
  }

  @objc func config(ad: MWMBanner, containerType: AdBannerContainerType) {
    reset()
    switch containerType {
    case .placePage:
      state = alternative(iPhone: .compact, iPad: .detailed)
    case .search:
      state = .search
    }

    nativeAd = ad as? Banner
    switch ad {
    case let ad as FacebookBanner: configFBBanner(ad: ad.nativeAd)
    case let ad as RBBanner: configRBBanner(ad: ad)
    case let ad as MopubBanner: configMopubBanner(ad: ad)
    case let ad as GoogleFallbackBanner: configGoogleFallbackBanner(ad: ad)
    case let ad as GoogleNativeBanner: configGoogleNativeBanner(ad: ad)
    default: assert(false)
    }
  }
  
  override func setHighlighted(_ highlighted: Bool, animated: Bool) {
      adCallToActionButtonCompact.isHighlighted = highlighted;
      adCallToActionButtonDetailed.isHighlighted = highlighted;
  }

  private func configFBBanner(ad: FBNativeAd) {
    adType = .native
    DAAImageWidth.constant = adPrivacyImage.width;
    DAAImage.isHidden = false;

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
    adTitleLabel.attributedText = attributedTitle(title: ad.title ?? "",
                                                  indent: adPrivacyImage.width + DAAImageWidth.constant)
    adBodyLabel.text = ad.body ?? ""
    let config = state.config()
    adTitleLabel.numberOfLines = config.numberOfTitleLines
    adBodyLabel.numberOfLines = config.numberOfBodyLines
    [adCallToActionButtonCompact, adCallToActionButtonDetailed].forEach { $0.setTitle(ad.callToAction, for: .normal) }
  }

  private func configRBBanner(ad: MTRGNativeAd) {
    guard let banner = ad.banner else { return }
    adType = .native
    DAAImageWidth.constant = 0;
    DAAImage.isHidden = true;

    MTRGNativeAd.loadImage(banner.icon, to: adIconImageView)
    adTitleLabel.attributedText = attributedTitle(title: banner.title,
                                                  indent: adPrivacyImage.width + DAAImageWidth.constant)
    adBodyLabel.text = banner.descriptionText ?? ""
    let config = state.config()
    adTitleLabel.numberOfLines = config.numberOfTitleLines
    adBodyLabel.numberOfLines = config.numberOfBodyLines

    [adCallToActionButtonCompact, adCallToActionButtonDetailed].forEach { $0.setTitle(banner.ctaText, for: .normal) }
    refreshBannerIfNeeded()
  }

  private func configMopubBanner(ad: MopubBanner) {
    mpNativeAd = ad.nativeAd
    adType = .native

    DAAImageWidth.constant = adPrivacyImage.width;
    DAAImage.isHidden = false;

    let adCallToActionButtons: [UIButton]
    if state == .search {
      adCallToActionButtonCustom.isHidden = false
      adCallToActionButtons = [adCallToActionButtonCustom, adCallToActionButtonCompact]
    } else {
      adCallToActionButtons = [adCallToActionButtonCompact, adCallToActionButtonDetailed]
      adCallToActionButtons.forEach { $0.setTitle(ad.ctaText, for: .normal) }
    }
    mpNativeAd?.setAdView(self, actionButtons: adCallToActionButtons)

    adTitleLabel.attributedText = attributedTitle(title: ad.title,
                                                  indent: adPrivacyImage.width + DAAImageWidth.constant)
    adBodyLabel.text = ad.text
    if let url = URL(string: ad.iconURL) {
      adIconImageView.af_setImage(withURL: url)
    }
  }

  private func configGoogleFallbackBanner(ad: GoogleFallbackBanner) {
    adType = .fallback
    DAAImageWidth.constant = adPrivacyImage.width;
    DAAImage.isHidden = false;

    fallbackAdView.subviews.forEach { $0.removeFromSuperview() }
    fallbackAdView.addSubview(ad)
    updateFallbackBannerLayout(ad: ad)
  }

  private func updateFallbackBannerLayout(ad: GoogleFallbackBanner) {
    ad.width = fallbackAdView.width
    fallbackAdViewHeight.constant = ad.dynamicSize.height
  }

  private func configGoogleNativeBanner(ad _: GoogleNativeBanner) {
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
