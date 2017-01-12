import UIKit

final class FirstLaunchController: MWMViewController, WelcomeProtocol {

  typealias ConfigBlock = (FirstLaunchController) -> Void
  static var pagesConfigBlocks: [ConfigBlock]! = [{
    $0.setup(image: #imageLiteral(resourceName: "img_onboarding_offline_maps"),
             title: L("onboarding_offline_maps_title"),
             text: L("onboarding_offline_maps_message"),
             buttonTitle: L("whats_new_next_button"),
             buttonAction: #selector(nextPage))
  }, {
    $0.setup(image: #imageLiteral(resourceName: "img_onboarding_geoposition"),
             title: L("onboarding_location_title"),
             text: L("onboarding_location_message"),
             buttonTitle: L("whats_new_next_button"),
             buttonAction: #selector(nextPage))
  }, {
    $0.setup(image: #imageLiteral(resourceName: "img_onboarding_notification"),
             title: L("onboarding_notifications_title"),
             text: L("onboarding_notifications_message"),
             buttonTitle: L("whats_new_next_button"),
             buttonAction: #selector(nextPage))
  }, {
    $0.setup(image: #imageLiteral(resourceName: "img_onboarding_done"),
             title: L("first_launch_congrats_title"),
             text: L("first_launch_congrats_text"),
             buttonTitle: L("done"),
             buttonAction: #selector(close))
  }]

  static var key: String { return "\(self)" }

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
    MWMFrameworkHelper.zoomToCurrentPosition()
  }
}
