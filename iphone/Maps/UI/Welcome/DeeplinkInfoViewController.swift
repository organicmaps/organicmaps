protocol DeeplinkInfoViewControllerDelegate: AnyObject {
  func deeplinkInfoViewControllerDidFinish(_ viewController: DeeplinkInfoViewController, deeplink: URL?)
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

    guard let dlUrl = deeplinkURL, let host = dlUrl.host else { return }
    switch host {
    case "guides_page":
      alertTitle.text = L("onboarding_guide_direct_download_title")
      alertText.text = L("onboarding_guide_direct_download_subtitle")
      nextPageButton.setTitle(L("onboarding_guide_direct_download_button"), for: .normal)
    case "catalogue":
      alertTitle.text = L("onboarding_bydeeplink_guide_title")
      alertText.text = L("onboarding_bydeeplink_guide_subtitle")
      nextPageButton.setTitle(L("current_location_unknown_continue_button"), for: .normal)
    default:
      break
    }

    Statistics.logEvent(kStatOnboardingDlShow, withParameters: [kStatType : host])
  }

  @IBAction func onNextButton(_ sender: UIButton) {
    delegate?.deeplinkInfoViewControllerDidFinish(self, deeplink: deeplinkURL)
    guard let dlUrl = deeplinkURL, let host = dlUrl.host else { return }
    Statistics.logEvent(kStatOnboardingDlAccept, withParameters: [kStatType : host])
  }
}
