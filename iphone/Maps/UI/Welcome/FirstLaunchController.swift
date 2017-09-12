import UIKit

final class FirstLaunchController: MWMViewController, WelcomeProtocol {

  static var welcomeConfigs: [WelcomeConfig] = [
    WelcomeConfig(image: #imageLiteral(resourceName: "img_onboarding_offline_maps"),
                  title: "onboarding_offline_maps_title",
                  text: "onboarding_offline_maps_message",
                  buttonTitle: "whats_new_next_button",
                  buttonAction: #selector(nextPage)),
    WelcomeConfig(image: #imageLiteral(resourceName: "img_onboarding_geoposition"),
                  title: "onboarding_location_title",
                  text: "onboarding_location_message",
                  buttonTitle: "whats_new_next_button",
                  buttonAction: #selector(nextPage)),
    WelcomeConfig(image: #imageLiteral(resourceName: "img_onboarding_notification"),
                  title: "onboarding_notifications_title",
                  text: "onboarding_notifications_message",
                  buttonTitle: "whats_new_next_button",
                  buttonAction: #selector(nextPage)),
    WelcomeConfig(image: #imageLiteral(resourceName: "img_onboarding_done"),
                  title: "first_launch_congrats_title",
                  text: "first_launch_congrats_text",
                  buttonTitle: "done",
                  buttonAction: #selector(close)),
  ]

  var pageIndex: Int!
  weak var pageController: WelcomePageController!

  @IBOutlet weak var image: UIImageView!
  @IBOutlet weak var alertTitle: UILabel!
  @IBOutlet weak var alertText: UILabel!
  @IBOutlet weak var nextPageButton: UIButton!
  @IBOutlet weak var containerWidth: NSLayoutConstraint!
  @IBOutlet weak var containerHeight: NSLayoutConstraint!

  @IBOutlet weak var imageMinHeight: NSLayoutConstraint!
  @IBOutlet weak var imageHeight: NSLayoutConstraint!

  @IBOutlet weak var titleTopOffset: NSLayoutConstraint!
  @IBOutlet weak var titleImageOffset: NSLayoutConstraint!

  override func viewDidLoad() {
    super.viewDidLoad()
    config()
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    updateSize()
    if pageIndex == 2 {
      MWMLocationManager.start()
    } else if pageIndex == 3 {
      MWMPushNotifications.setup(nil)
    }
  }

  @objc
  private func nextPage() {
    pageController.nextPage()
  }

  @objc
  private func close() {
    pageController.close()
    MWMFrameworkHelper.processFirstLaunch()
  }
}
