protocol PlacePageHeaderViewProtocol: class {
  var presenter: PlacePageHeaderPresenterProtocol?  { get set }
  var isExpandViewHidden: Bool { get set }
  var isShadowViewHidden: Bool { get set }

  func setTitle(_ title: String)
}

class PlacePageHeaderViewController: UIViewController {
  var presenter: PlacePageHeaderPresenterProtocol?

  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var expandView: UIView!
  @IBOutlet private var shadowView: UIView!

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
  var isExpandViewHidden: Bool {
    get {
      expandView.isHidden
    }
    set {
      expandView.isHidden = newValue
    }
  }

  var isShadowViewHidden: Bool {
    get {
      shadowView.isHidden
    }
    set {
      shadowView.isHidden = newValue
    }
  }

  func setTitle(_ title: String) {
    titleLabel.text = title
  }
}
