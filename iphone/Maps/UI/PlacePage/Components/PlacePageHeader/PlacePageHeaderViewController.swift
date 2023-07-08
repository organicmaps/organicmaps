protocol PlacePageHeaderViewProtocol: AnyObject {
  var presenter: PlacePageHeaderPresenterProtocol?  { get set }
  var isExpandViewHidden: Bool { get set }
  var isShadowViewHidden: Bool { get set }

  func setTitle(_ title: String?, secondaryTitle: String?)
}

class PlacePageHeaderViewController: UIViewController {
  var presenter: PlacePageHeaderPresenterProtocol?

  @IBOutlet private var headerView: PlacePageHeaderView!
  @IBOutlet private var titleLabel: UILabel?
  @IBOutlet private var expandView: UIView!
  @IBOutlet private var shadowView: UIView!

  override func viewDidLoad() {
    super.viewDidLoad()
    presenter?.configure()
    let tap = UITapGestureRecognizer(target: self, action: #selector(onExpandPressed(sender:)))
    expandView.addGestureRecognizer(tap)
    headerView.layer.maskedCorners = [.layerMinXMinYCorner, .layerMaxXMinYCorner]
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

  func setTitle(_ title: String?, secondaryTitle: String?) {
    // XCode 13 is not smart enough to detect that title is used below, and requires explicit unwrapped variable.
    guard let unwrappedTitle = title else {
      titleLabel?.attributedText = nil
      return
    }

    let titleAttributes: [NSAttributedString.Key: Any] = [
      .font: StyleManager.shared.theme!.fonts.medium20,
      .foregroundColor: UIColor.blackPrimaryText()
    ]

    let attributedText = NSMutableAttributedString(string: unwrappedTitle, attributes: titleAttributes)

    guard let unwrappedSecondaryTitle = secondaryTitle else {
      titleLabel?.attributedText = attributedText
      return
    }

    let paragraphStyle = NSMutableParagraphStyle()
    paragraphStyle.paragraphSpacingBefore = 2
    let secondaryTitleAttributes: [NSAttributedString.Key: Any] = [
      .font: StyleManager.shared.theme!.fonts.medium16,
      .foregroundColor: UIColor.blackPrimaryText(),
      .paragraphStyle: paragraphStyle
    ]

    attributedText.append(NSAttributedString(string: "\n" + unwrappedSecondaryTitle, attributes: secondaryTitleAttributes))
    titleLabel?.attributedText = attributedText
  }
}
