import UIKit

protocol IGuidesGalleryItemViewModel {
  var title: String { get }
  var subtitle: String { get }
  var imageUrl: URL? { get }
  var downloaded: Bool { get }
  var visible: Bool? { get }
}

protocol GuidesGalleryCellDelegate: AnyObject {
  func onShowButton(_ cell: GuidesGalleryCell)
}

class GuidesGalleryCell: UICollectionViewCell {
  @IBOutlet private var imageView: UIImageView!
  @IBOutlet private var checkmarkImageView: UIImageView!
  @IBOutlet private var checkmarkImageViewBg: UIImageView!
  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var subtitleLabel: UILabel!
  @IBOutlet private var infoContainerView: UIView!
  @IBOutlet private var buttonContainerView: UIView!
  @IBOutlet private var visibilityButton: UIButton!

  weak var delegate: GuidesGalleryCellDelegate?

  func config(_ item: IGuidesGalleryItemViewModel) {
    if let imageUrl = item.imageUrl {
      imageView.wi_setImage(with: imageUrl, transitionDuration: 0) { [weak self] (image, error) in
        guard let _ = image else { return }
        self?.imageView.contentMode = .scaleAspectFill
      }
    }
    titleLabel.text = item.title
    subtitleLabel.text = item.subtitle
    infoContainerView.isHidden = item.downloaded
    buttonContainerView.isHidden = !item.downloaded
    checkmarkImageView.isHidden = !item.downloaded
    checkmarkImageViewBg.isHidden = !item.downloaded
    if let visible = item.visible {
      visibilityButton.setTitle(L(visible ? "hide" : "show"), for: .normal)
    }
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    imageView.wi_cancelImageRequest()
    imageView.image = UIImage(named: "routes_gallery_cell_image")
    imageView.contentMode = .center
    titleLabel.text = nil
    subtitleLabel.text = nil
    delegate = nil
  }

  @IBAction func onShowButton(_ sender: UIButton) {
    delegate?.onShowButton(self)
  }
}
