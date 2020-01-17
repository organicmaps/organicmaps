@objc(MWMDiscoveryOnlineTemplateType)
enum DiscoveryOnlineTemplateType: Int {
  case promo
}

@objc(MWMDiscoveryOnlineTemplateCell)
final class DiscoveryOnlineTemplateCell: MWMTableViewCell {
  @IBOutlet var spinner: UIImageView! {
    didSet {
      let postfix = UIColor.isNightMode() ? "_dark" : "_light"
      spinner.image = UIImage(named: "Spinner" + postfix)
      spinner.startRotation()
    }
  }
  
  @IBOutlet var containerView: UIView!
  @IBOutlet var header: UILabel!
  @IBOutlet var title: UILabel!
  @IBOutlet var subtitle: UILabel!
  @IBOutlet var actionButton: UIButton!

  typealias Tap = () -> ()
  private var tap: Tap?
  
  override func awakeFromNib() {
    super.awakeFromNib()
    backgroundColor = .clear
  }
  
  override func prepareForReuse() {
    super.prepareForReuse()
    spinner.isHidden = true
    spinner.stopRotation()
    actionButton.isHidden = true
  }

  @objc func config(type: DiscoveryOnlineTemplateType, needSpinner: Bool, canUseNetwork: Bool, tap: @escaping Tap) {
    switch type {
    case .promo:
      header.text = L("gallery_pp_download_guides_title").uppercased()
      title.text = L("gallery_pp_download_guides_offline_title")
      subtitle.text = L("gallery_pp_download_guides_offline_subtitle")
      if canUseNetwork {
        actionButton.setTitle(L("details"),
                              for: .normal)
      } else {
        actionButton.setTitle(L("gallery_pp_download_guides_offline_cta"),
                              for: .normal)
      }
    }
    
    if (needSpinner) {
      spinner.isHidden = false
      spinner.startRotation()
      actionButton.isHidden = true
    } else {
      spinner.stopRotation()
      spinner.isHidden = true
      actionButton.isHidden = canUseNetwork ? true : false
    }
    self.tap = tap
  }

  @IBAction private func onTap() {
    tap?()
  }
}
