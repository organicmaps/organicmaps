final class FirstLaunchController: WelcomeViewController {

  private enum Permission {
    case location
    case notifications
    case nothing
  }
  
  private struct FirstLaunchConfig: WelcomeConfig {
    let image: UIImage
    let title: String
    let text: String
    let buttonTitle: String
    let requestPermission: Permission
  }
  
  static var welcomeConfigs: [WelcomeConfig] {
    return [
      FirstLaunchConfig(image: #imageLiteral(resourceName: "img_onboarding_offline_maps"),
                        title: "onboarding_offline_maps_title",
                        text: "onboarding_offline_maps_message",
                        buttonTitle: "whats_new_next_button",
                        requestPermission: .nothing),
      FirstLaunchConfig(image: #imageLiteral(resourceName: "img_onboarding_geoposition"),
                        title: "onboarding_location_title",
                        text: "onboarding_location_message",
                        buttonTitle: "whats_new_next_button",
                        requestPermission: .nothing),
      FirstLaunchConfig(image: #imageLiteral(resourceName: "img_onboarding_notification"),
                        title: "onboarding_notifications_title",
                        text: "onboarding_notifications_message",
                        buttonTitle: "whats_new_next_button",
                        requestPermission: .location),
      FirstLaunchConfig(image: #imageLiteral(resourceName: "img_onboarding_done"),
                        title: "first_launch_congrats_title",
                        text: "first_launch_congrats_text",
                        buttonTitle: "done",
                        requestPermission: .notifications)
    ]
  }

  override class var key: String { return toString(self) }
  
  static func controllers() -> [FirstLaunchController] {
    var result = [FirstLaunchController]()
    let sb = UIStoryboard.instance(.welcome)
    FirstLaunchController.welcomeConfigs.forEach { (config) in
      let vc = sb.instantiateViewController(withIdentifier: toString(self)) as! FirstLaunchController
      vc.pageConfig = config
      result.append(vc)
    }
    return result
  }
  
  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    let config = pageConfig as! FirstLaunchConfig
    switch config.requestPermission {
    case .location:
      MWMLocationManager.start()
    case .notifications:
      MWMPushNotifications.setup(nil)
    case .nothing:
      break
    }
  }
}
