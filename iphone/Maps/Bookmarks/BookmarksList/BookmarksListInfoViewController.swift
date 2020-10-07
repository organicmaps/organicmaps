import UIKit

protocol IBookmakrsListInfoViewModel {
  var title: String { get }
  var author: String { get }
  var hasDescription: Bool { get }
  var imageUrl: URL? { get }
  var hasLogo: Bool { get } //TODO: maybe replace with logo url or similar
}

protocol BookmarksListInfoViewControllerDelegate: AnyObject {
  func didPressDescription()
  func didUpdateContent()
}

class BookmarksListInfoViewController: UIViewController {
  var info: IBookmakrsListInfoViewModel? {
    didSet {
      guard isViewLoaded, let info = info else { return }
      updateInfo(info)
    }
  }

  weak var delegate: BookmarksListInfoViewControllerDelegate?

  @IBOutlet var titleImageView: UIImageView!
  @IBOutlet var titleLabel: UILabel!
  @IBOutlet var descriptionButton: UIButton!
  @IBOutlet var descriptionButtonView: UIView!
  @IBOutlet var authorImageView: UIImageView!
  @IBOutlet var authorLabel: UILabel!
  @IBOutlet var infoStack: UIStackView!

  @IBAction func onDescription(_ sender: UIButton) {
    delegate?.didPressDescription()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    descriptionButton.setTitle(L("description_guide").uppercased(), for: .normal)
    guard let info = info else { return }
    updateInfo(info)
  }

  private func updateInfo(_ info: IBookmakrsListInfoViewModel) {
    titleLabel.text = info.title
    authorLabel.text = String(coreFormat: L("author_name_by_prefix"), arguments: [info.author])
    authorImageView.isHidden = !info.hasLogo
    descriptionButtonView.isHidden = !info.hasDescription

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
