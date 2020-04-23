import UIKit

final class GuidesGalleryCityCell: UICollectionViewCell {
  @IBOutlet private var imageView: UIImageView!
  @IBOutlet private var checkmarkImageView: UIImageView!
  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var subtitleLabel: UILabel!
  @IBOutlet private var infoLabel: UILabel!
  @IBOutlet private var infoContainerView: UIView!
  @IBOutlet private var buttonContainerView: UIView!

  func config(_ item: IGuidesGalleryCityItemViewModel) {
    if let imageUrl = item.imageUrl {
      imageView.wi_setImage(with: imageUrl, transitionDuration: 0) { [weak self] (image, error) in
        guard let _ = image else { return }
        self?.imageView.contentMode = .scaleAspectFill
      }
    }
    titleLabel.text = item.title
    subtitleLabel.text = item.subtitle
    if !item.downloaded {
      infoLabel.text = item.info
    }
    infoContainerView.isHidden = item.downloaded
    buttonContainerView.isHidden = !item.downloaded
    checkmarkImageView.isHidden = !item.downloaded
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    imageView.wi_cancelImageRequest()
    imageView.image = UIImage(named: "routes_gallery_cell_image")
    imageView.contentMode = .center
    titleLabel.text = nil
    subtitleLabel.text = nil
    infoLabel.text = nil
  }
}
