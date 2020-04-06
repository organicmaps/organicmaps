protocol PlacePageHeaderViewProtocol: class {
  var presenter: PlacePageHeaderPresenterProtocol?  { get set }
  func setTitle(_ title: String)
  func setViewStyle(_ style: String)
  func setExpandViewEnabled(_ val: Bool)
}

class PlacePageHeaderViewController: UIViewController {
  var presenter: PlacePageHeaderPresenterProtocol?

  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var expandView: UIView!

  override func viewDidLoad() {
    super.viewDidLoad()
    presenter?.configure()
    let tap = UITapGestureRecognizer(target: self, action: #selector(onExpandPressed(sender:)))
    expandView.addGestureRecognizer(tap)
  }

  @objc func onExpandPressed(sender: UITapGestureRecognizer) {
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

  func setExpandViewEnabled(_ val: Bool) {
    expandView.isHidden = !val
  }
}
