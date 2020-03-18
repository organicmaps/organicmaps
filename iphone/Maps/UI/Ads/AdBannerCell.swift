@objc(MWMAdBannerCell)
final class AdBannerCell: UITableViewCell {
  private let adBannerView: AdBannerView

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    adBannerView = Bundle.main.load(viewClass: AdBannerView.self)?.first as! AdBannerView
    super.init(style: style, reuseIdentifier: reuseIdentifier)
    adBannerView.frame = self.bounds
    addSubview(adBannerView)
    clipsToBounds = true
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  @objc func config(ad: MWMBanner, containerType: AdBannerContainerType, canRemoveAds: Bool, onRemoveAds: (() -> Void)?) {
    adBannerView.config(ad: ad, containerType: containerType, canRemoveAds: canRemoveAds, onRemoveAds: onRemoveAds)
  }
}
