import UIKit

final class WhatsNewController: MWMViewController, WelcomeProtocol {

  static var welcomeConfigs: [WelcomeConfig] = [
//    WelcomeConfig(image: #imageLiteral(resourceName: "img_whats_new_route"),
//                  title: "whats_new_title_route",
//                  text: "whats_new_message_route",
//                  buttonTitle: "whats_new_next_button",
//                  buttonAction: #selector(nextPage)),
    WelcomeConfig(image: #imageLiteral(resourceName: "img_whats_new_ugc"),
                  title: "whats_new_ugc_title",
                  text: "whats_new_ugc_message",
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
  }

  @objc
  private func nextPage() {
    pageController.nextPage()
  }

  @IBAction
  private func close() {
    pageController.close()
  }
}
