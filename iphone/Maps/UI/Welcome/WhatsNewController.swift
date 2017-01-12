import UIKit

final class WhatsNewController: MWMViewController, WelcomeProtocol {

  typealias ConfigBlock = (WhatsNewController) -> Void
  static var pagesConfigBlocks: [ConfigBlock]! = [{
    $0.setup(image: #imageLiteral(resourceName: "whats_new_traffic"),
             title: L("whatsnew_traffic"),
             text: L("whatsnew_traffic_text"),
             buttonTitle: L("whats_new_next_button"),
             buttonAction: #selector(nextPage))
  }, {
    $0.setup(image: #imageLiteral(resourceName: "whats_new_traffic_roaming"),
             title: L("whatsnew_traffic_roaming"),
             text: L("whatsnew_traffic_roaming_text"),
             buttonTitle: L("whats_new_next_button"),
             buttonAction: #selector(nextPage))
  }, {
    $0.setup(image: #imageLiteral(resourceName: "whats_new_update_uber"),
             title: L("whatsnew_order_taxi"),
             text: L("whatsnew_order_taxi_text"),
             buttonTitle: L("done"),
             buttonAction: #selector(close))
  }]

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
