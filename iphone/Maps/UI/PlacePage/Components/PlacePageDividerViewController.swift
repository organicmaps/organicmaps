final class PlacePageDividerViewController: UIViewController {
  @IBOutlet private var titleLabel: UILabel!

  var titleText: String? {
    didSet {
      titleLabel.text = titleText
    }
  }

  override func viewDidLoad() {
    super.viewDidLoad()
  }
}
