final class WhatsNewController: WelcomeViewController {

  private struct WhatsNewConfig: WelcomeConfig {
    let image: UIImage
    let title: String
    let text: String
    let buttonTitle: String
    let ctaButtonTitle: String?
    let ctaButtonUrl: String?
  }
  
  static let ctaUrl = "https://b2b.maps.me/whatsnew/us"
  static var welcomeConfigs: [WelcomeConfig] {
  return [
    WhatsNewConfig(image: #imageLiteral(resourceName: "img_wn_business"),
                   title: "whats_new_localbiz_title",
                   text: "whats_new_localbiz_message",
                   buttonTitle: "done",
                   ctaButtonTitle: "whats_new_order_button",
                   ctaButtonUrl: ctaUrl)
    ]
  }

  override class var key: String { return welcomeConfigs.reduce("\(self)", { return "\($0)_\($1.title)" }) }
  
  static func controllers() -> [WelcomeViewController] {
    var result = [WelcomeViewController]()
    let sb = UIStoryboard.instance(.welcome)
    WhatsNewController.welcomeConfigs.forEach { (config) in
      let vc = sb.instantiateViewController(withIdentifier: toString(self)) as! WelcomeViewController
      vc.pageConfig = config
      result.append(vc)
    }
    return result
  }
  
  @IBOutlet weak var ctaButton: UIButton!
  
  override func viewDidLoad() {
    super.viewDidLoad()
    let config = pageConfig as! WhatsNewConfig
    if let ctaTitleKey = config.ctaButtonTitle {
      ctaButton.setTitle(L(ctaTitleKey), for: .normal)
    } else {
      ctaButton.isHidden = true
    }
  }
  
  @IBAction func onCta() {
    let config = pageConfig as! WhatsNewConfig
    if let url = URL(string: config.ctaButtonUrl!) {
      UIApplication.shared.openURL(url)
    } else {
      assertionFailure()
    }
    close()
  }
}
