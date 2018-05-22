fileprivate struct FirstLaunchConfig: WelcomeConfig {
  let image: UIImage
  let title: String
  let text: String
  let buttonTitle: String
  let requestLocationPermission: Bool
  let requestNotificationsPermission: Bool
}

final class FirstLaunchController: WelcomeViewController {

  static var welcomeConfigs: [WelcomeConfig] {
    return [
      FirstLaunchConfig(image: #imageLiteral(resourceName: "img_onboarding_offline_maps"),
                        title: "onboarding_offline_maps_title",
                        text: "onboarding_offline_maps_message",
                        buttonTitle: "whats_new_next_button",
                        requestLocationPermission: false,
                        requestNotificationsPermission: false),
      FirstLaunchConfig(image: #imageLiteral(resourceName: "img_onboarding_geoposition"),
                        title: "onboarding_location_title",
                        text: "onboarding_location_message",
                        buttonTitle: "whats_new_next_button",
                        requestLocationPermission: false,
                        requestNotificationsPermission: false),
      FirstLaunchConfig(image: #imageLiteral(resourceName: "img_onboarding_notification"),
                        title: "onboarding_notifications_title",
                        text: "onboarding_notifications_message",
                        buttonTitle: "whats_new_next_button",
                        requestLocationPermission: true,
                        requestNotificationsPermission: false),
      FirstLaunchConfig(image: #imageLiteral(resourceName: "img_onboarding_done"),
                        title: "first_launch_congrats_title",
                        text: "first_launch_congrats_text",
                        buttonTitle: "done",
                        requestLocationPermission: false,
                        requestNotificationsPermission: true)
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
    if let config = pageConfig as? FirstLaunchConfig {
      if config.requestLocationPermission {
        MWMLocationManager.start()
      }
      if config.requestNotificationsPermission {
        MWMPushNotifications.setup(nil)
      }
    }
  }

  override func close() {
    super.close()
    MWMFrameworkHelper.processFirstLaunch()
  }
}
