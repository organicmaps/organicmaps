@objc(MWMDiscoveryOnlineTemplateType)
enum DiscoveryOnlineTemplateType: Int {
  case locals
}

@objc(MWMDiscoveryOnlineTemplateCell)
final class DiscoveryOnlineTemplateCell: MWMTableViewCell {
  @IBOutlet private weak var spinner: UIImageView! {
    didSet {
      let postfix = UIColor.isNightMode() ? "_dark" : "_light"
      spinner.image = UIImage(named: "Spinner" + postfix)
    }
  }

  @IBOutlet private weak var title: UILabel!
  @IBOutlet private weak var subtitle: UILabel!

  typealias Tap = () -> ()
  private var tap: Tap?

  @objc func config(type: DiscoveryOnlineTemplateType, needSpinner: Bool, tap: @escaping Tap) {
    switch type {
      case .locals:
        title.text = needSpinner ? L("discovery_button_other_loading_message") :
                                   L("discovery_button_other_error_message")
        subtitle.text = ""
    }

    spinner.isHidden = !needSpinner
    if (needSpinner) {
      spinner.startRotation()
    }
    self.tap = tap
  }

  @IBAction private func onTap() {
    tap?()
  }
}
