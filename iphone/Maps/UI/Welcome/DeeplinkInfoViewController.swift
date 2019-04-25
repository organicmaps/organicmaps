protocol DeeplinkInfoViewControllerDelegate: AnyObject {
  func deeplinkInfoViewControllerDidFinish(_ viewController: DeeplinkInfoViewController)
}

class DeeplinkInfoViewController: UIViewController {
  @IBOutlet weak var image: UIImageView!
  @IBOutlet weak var alertTitle: UILabel!
  @IBOutlet weak var alertText: UILabel!
  @IBOutlet weak var nextPageButton: UIButton!

  var deeplinkURL: URL?
  var delegate: DeeplinkInfoViewControllerDelegate?

  override func viewDidLoad() {
    super.viewDidLoad()

    guard let dlUrl = deeplinkURL else { return }
    switch dlUrl.path {
    case "/guides_page":
      alertTitle.text = L("onboarding_guide_direct_download_title")
      alertText.text = L("onboarding_guide_direct_download_subtitle")
      nextPageButton.setTitle(L("onboarding_guide_direct_download_button"), for: .normal)
    case "/catalogue":
      alertTitle.text = L("onboarding_bydeeplink_guide_title")
      alertText.text = L("onboarding_bydeeplink_guide_subtitle")
      nextPageButton.setTitle(L("current_location_unknown_continue_button"), for: .normal)
    default:
      break
    }
  }

  @IBAction func onNextButton(_ sender: UIButton) {
    delegate?.deeplinkInfoViewControllerDidFinish(self)
  }
}
