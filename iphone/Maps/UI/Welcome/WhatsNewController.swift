fileprivate struct WhatsNewConfig: WelcomeConfig {
  let image: UIImage
  let title: String
  let text: String
  let buttonTitle: String
}

final class WhatsNewController: WelcomeViewController {

  static var welcomeConfigs: [WelcomeConfig] {
  return [
    WhatsNewConfig(image: #imageLiteral(resourceName: "img_wn_business"),
                   title: "whats_new_localbiz_title",
                   text: "whats_new_localbiz_message",
                   buttonTitle: "done")
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
}
