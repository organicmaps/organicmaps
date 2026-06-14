protocol BookmarksListInfoViewControllerDelegate: AnyObject {
  func didPressDescription()
  func didUpdateContent()
}

final class BookmarksListInfoViewController: UIViewController {
  var info: IBookmarksListInfoViewModel? {
    didSet {
      guard isViewLoaded, let info = info else { return }
      updateInfo(info)
    }
  }

  weak var delegate: BookmarksListInfoViewControllerDelegate?

  @IBOutlet private var titleImageView: UIImageView!
  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var descriptionLabel: UILabel!
  @IBOutlet private var authorContainerView: UIView!
  @IBOutlet private var infoStack: UIStackView!
  @IBOutlet private var separatorsConstraints: [NSLayoutConstraint]!

  @objc private func onDescriptionTap() {
    delegate?.didPressDescription()
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    separatorsConstraints.forEach { $0.constant = 1 / UIScreen.main.scale }
    descriptionLabel.numberOfLines = 2
    descriptionLabel.lineBreakMode = .byTruncatingTail
    descriptionLabel.isUserInteractionEnabled = true
    descriptionLabel.accessibilityTraits.insert(.button)
    descriptionLabel.addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(onDescriptionTap)))

    guard let info = info else { return }
    updateInfo(info)
  }

  private func updateInfo(_ info: IBookmarksListInfoViewModel) {
    titleLabel.text = info.title
    descriptionLabel.isHidden = !info.hasDescription
    if info.hasDescription {
      let description = info.isHtmlDescription
        ? BookmarksListInfoViewController.getPlainText(info.description)
        : info.description
      descriptionLabel.text = description
    }

    titleImageView.isHidden = true
    if let imageUrl = info.imageUrl {
      titleImageView.wi_setImage(with: imageUrl, transitionDuration: 0) { [weak self] image, _ in
        guard image != nil else { return }
        self?.titleImageView.isHidden = false
        self?.delegate?.didUpdateContent()
      }
    }
  }

  private static func getPlainText(_ htmlText: String) -> String? {
    let formattedText = NSAttributedString.string(withHtml: htmlText, defaultAttributes: [:])
    return formattedText?.string
  }
}
