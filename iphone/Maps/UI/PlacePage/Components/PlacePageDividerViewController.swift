final class PlacePageDividerViewController: UIViewController {
  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var authorIcon: UIImageView!

  var titleText: String? {
    didSet {
      titleLabel.text = titleText
    }
  }

  var isAuthorIconHidden = true {
    didSet {
      authorIcon.isHidden = isAuthorIconHidden
    }
  }

  override func viewDidLoad() {
    super.viewDidLoad()
  }
}
