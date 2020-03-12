protocol PlacePageHeaderViewProtocol: class {
  var presenter: PlacePageHeaderPresenterProtocol?  { get set }
  func setTitle(_ title: String)
  func setViewStyle(_ style: String)
  func setExpandButtonEnabled(_ val: Bool)
}

class PlacePageHeaderViewController: UIViewController {
  var presenter: PlacePageHeaderPresenterProtocol?

  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var expandButton: UIButton!

  override func viewDidLoad() {
    super.viewDidLoad()
    presenter?.configure()
  }

  @IBAction func onExpandButtonPressed(_ sender: Any) {
    presenter?.onExpandPress()
  }

  @IBAction private func onCloseButtonPressed(_ sender: Any) {
    presenter?.onClosePress()
  }
}

extension PlacePageHeaderViewController: PlacePageHeaderViewProtocol {
  func setTitle(_ title: String) {
    titleLabel.text = title
  }

  func setViewStyle(_ style: String) {
    view.setStyleAndApply(style)
  }

  func setExpandButtonEnabled(_ val: Bool) {
    expandButton.isHidden = !val
  }
}
