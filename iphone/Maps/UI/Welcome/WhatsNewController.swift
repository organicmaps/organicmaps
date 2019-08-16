final class WhatsNewController: WelcomeViewController {

  private struct WhatsNewConfig: WelcomeConfig {
    let image: UIImage
    let title: String
    let text: String
    let buttonTitle: String
  }
  
  static var welcomeConfigs: [WelcomeConfig] {
  return [
    WhatsNewConfig(image: UIImage(named: "img_whatsnew_carplay")!,
                   title: "whatsnew_carplay_title",
                   text: "whatsnew_carplay_message",
                   buttonTitle: "whats_new_next_button"),
    WhatsNewConfig(image: UIImage(named: "img_whatsnew_catalogsubs")!,
                   title: "whatsnew_subscription_title",
                   text: "whatsnew_subscription_message",
                   buttonTitle: "done")
    ]
  }

  override class var key: String { return welcomeConfigs.reduce("\(self)", { return "\($0)_\($1.title)" }) }
  
  static var shouldShowWhatsNew: Bool {
    get {
      return !UserDefaults.standard.bool(forKey: key)
    }
    set {
      UserDefaults.standard.set(!newValue, forKey: key)
    }
  }

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
}
