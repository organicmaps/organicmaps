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
                        attributes: [NSAttributedString.Key.paragraphStyle: paragraphStyle,
                                     NSAttributedString.Key.font: UIFont.bold12(),
                                     NSAttributedString.Key.foregroundColor: UIColor.isNightMode() ? UIColor.pressBackground() : UIColor.blackSecondaryText()
                                    ])
}

class AdBannerView: UIView {
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
  @IBOutlet private weak var removeAdsSmallButton: UIButton!
  @IBOutlet private weak var removeAdsLargeButton: UIButton!
  @IBOutlet private weak var removeAdsImage: UIImageView!
  @IBOutlet private weak var fallbackAdView: UIView!
  @IBOutlet private var nativeAdViewBottom: NSLayoutConstraint!
  @IBOutlet private var fallbackAdViewBottom: NSLayoutConstraint!
  @IBOutlet private var fallbackAdViewHeight: NSLayoutConstraint!
  @IBOutlet private var ctaButtonLeftConstraint: NSLayoutConstraint!
  @IBOutlet private weak var nativeAdView: UIView! {
    didSet {
      nativeAdView.layer.borderColor = UIColor.blackDividers().cgColor
    }
  }

  @objc var onRemoveAds: MWMVoidBlock?

  @objc static let detailedBannerExcessHeight: Float = 52

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

  func cancelImageLoading() {
    adIconImageView.wi_cancelImageRequest()
    adIconImageView.image = nil;
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

  @IBAction
  private func removeAction(_ sender: UIButton) {
    Statistics.logEvent(kStatInappBannerClose, withParameters: [
      kStatBanner : state == .detailed ? 1 : 0,
      kStatButton : sender == removeAdsLargeButton ? 1 : 0])
    onRemoveAds?()
  }

  func reset() {
    state = .unset
  }

  @objc func config(ad: MWMBanner, containerType: AdBannerContainerType, canRemoveAds: Bool, onRemoveAds: (() -> Void)?) {
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
    default: assert(false)
    }
    self.onRemoveAds = onRemoveAds
    removeAdsSmallButton.isHidden = !canRemoveAds
    removeAdsLargeButton.isHidden = !canRemoveAds
    removeAdsImage.isHidden = !canRemoveAds
    ctaButtonLeftConstraint.priority = canRemoveAds ? .defaultLow : .defaultHigh
  }

  func setHighlighted(_ highlighted: Bool) {
      adCallToActionButtonCompact.isHighlighted = highlighted;
      adCallToActionButtonDetailed.isHighlighted = highlighted;
  }

  private func configFBBanner(ad: FBNativeBannerAd) {
    adType = .native
    DAAImageWidth.constant = adPrivacyImage.width;
    DAAImage.isHidden = false;

    let adCallToActionButtons: [UIView]
    if state == .search {
      adCallToActionButtons = [self, adCallToActionButtonCompact]
    } else {
      adCallToActionButtons = [adCallToActionButtonCompact, adCallToActionButtonDetailed]
    }
    ad.registerView(forInteraction: self,
                    iconImageView: adIconImageView,
                    viewController: UIViewController.topViewController(),
                    clickableViews: adCallToActionButtons)

    adTitleLabel.attributedText = attributedTitle(title: ad.headline ?? "",
                                                  indent: adPrivacyImage.width + DAAImageWidth.constant)
    adBodyLabel.text = ad.bodyText ?? ""
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
    mpNativeAd?.setAdView(self, iconView: adIconImageView, actionButtons: adCallToActionButtons)
    adTitleLabel.attributedText = attributedTitle(title: ad.title,
                                                  indent: adPrivacyImage.width + DAAImageWidth.constant)
    adBodyLabel.text = ad.text
    if let url = URL(string: ad.iconURL) {
      adIconImageView.wi_setImage(with: url)
    }
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


  override func applyTheme() {
    super.applyTheme()
    adTitleLabel.attributedText = attributedTitle(title: adTitleLabel.attributedText?.string ?? "",
                                                  indent: adPrivacyImage.width + DAAImageWidth.constant)
  }
}
