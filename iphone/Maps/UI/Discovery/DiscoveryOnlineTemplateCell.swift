@objc(MWMDiscoveryOnlineTemplateType)
enum DiscoveryOnlineTemplateType: Int {
  case viator
  case locals
}

@objc(MWMDiscoveryOnlineTemplateCell)
final class DiscoveryOnlineTemplateCell: MWMTableViewCell {
  @IBOutlet private weak var spinner: UIImageView! {
    didSet {
      let postfix = UIColor.isNightMode() ? "_dark" : "_light"
      let animationImagesCount = 12
      var images = Array<UIImage>()
      for i in 1...animationImagesCount {
        images.append(UIImage(named: "Spinner_\(i)" + postfix)!)
      }
      spinner.animationDuration = 0.8
      spinner.animationImages = images
    }
  }

  @IBOutlet private weak var title: UILabel!
  @IBOutlet private weak var subtitle: UILabel!

  typealias Tap = () -> ()
  private var tap: Tap?

  private var type: DiscoveryOnlineTemplateType = .viator

  @objc func config(type: DiscoveryOnlineTemplateType, needSpinner: Bool, tap: @escaping Tap) {
    self.type = type;

    switch type {
      case .viator:
        title.text = L("preloader_viator_title")
        subtitle.text = L("preloader_viator_message")
      case .locals:
        title.text = L("discovery_button_other_error_message")
        subtitle.text = ""
    }

    spinner.isHidden = !needSpinner
    if (needSpinner) {
      spinner.startAnimating()
    }
    self.tap = tap
  }

  @IBAction private func onTap() {
    tap?()
  }
}
