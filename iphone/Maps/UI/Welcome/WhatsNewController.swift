import UIKit

final class WhatsNewController: MWMViewController, WelcomeProtocol {

  typealias ConfigBlock = (WhatsNewController) -> Void
  static var pagesConfigBlocks: [ConfigBlock]! = [{
    $0.setup(image: #imageLiteral(resourceName: "wn_img_1"),
             title: L("whatsnew_improved_search"),
             text: L("whatsnew_improved_search_text"),
             buttonTitle: L("whats_new_next_button"),
             buttonAction: #selector(nextPage))
  }, {
    $0.setup(image: #imageLiteral(resourceName: "wn_img_2"),
             title: L("whatsnew_filters_in_search"),
             text: L("whatsnew_filters_in_search_text"),
             buttonTitle: L("whats_new_next_button"),
             buttonAction: #selector(nextPage))
  }, {
    $0.setup(image: #imageLiteral(resourceName: "wn_img_3"),
             title: L("whatsnew_font_size"),
             text: L("whatsnew_font_size_text"),
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
