protocol BookmarksListInfoViewControllerDelegate: AnyObject {
  func didPressDescription()
  func didUpdateContent()
}

final class BookmarksListInfoViewController: UIViewController {
  var info: IBookmakrsListInfoViewModel? {
    didSet {
      guard isViewLoaded, let info = info else { return }
      updateInfo(info)
    }
  }

  weak var delegate: BookmarksListInfoViewControllerDelegate?

  @IBOutlet private var titleImageView: UIImageView!
  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var descriptionButton: UIButton!
  @IBOutlet private var authorContainerView: UIView!
  @IBOutlet private var authorImageView: UIImageView!
  @IBOutlet private var authorLabel: UILabel!
  @IBOutlet private var infoStack: UIStackView!
  @IBOutlet private var separatorsConstraints: [NSLayoutConstraint]!

  @IBAction private func onDescription(_ sender: UIButton) {
    delegate?.didPressDescription()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    descriptionButton.setTitle(L("description_guide").uppercased(), for: .normal)
    separatorsConstraints.forEach { $0.constant = 1 / UIScreen.main.scale }

    guard let info = info else { return }
    updateInfo(info)
  }

  private func updateInfo(_ info: IBookmakrsListInfoViewModel) {
    titleLabel.text = info.title
    authorLabel.text = String(coreFormat: L("author_name_by_prefix"), arguments: [info.author])
    authorImageView.isHidden = !info.hasLogo
    authorContainerView.isHidden = info.author.isEmpty
    descriptionButton.isHidden = !info.hasDescription

    titleImageView.isHidden = true
    if let imageUrl = info.imageUrl {
      titleImageView.wi_setImage(with: imageUrl, transitionDuration: 0) { [weak self] (image, error) in
        guard image != nil else { return }
        self?.titleImageView.isHidden = false
        self?.delegate?.didUpdateContent()
      }
    }
  }
}
