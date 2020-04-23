import UIKit

final class GuidesGalleryOutdoorCell: UICollectionViewCell {
  @IBOutlet private var imageView: UIImageView!
  @IBOutlet private var checkmarkImageView: UIImageView!
  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var subtitleLabel: UILabel!
  @IBOutlet private var timeLabel: UILabel!
  @IBOutlet private var timeIcon: UIImageView!
  @IBOutlet private var distanceLabel: UILabel!
  @IBOutlet private var ascentLabel: UILabel!
  @IBOutlet private var infoContainerView: UIView!
  @IBOutlet private var buttonContainerView: UIView!

  func config(_ item: IGuidesGalleryOutdoorItemViewModel) {
    if let imageUrl = item.imageUrl {
      imageView.wi_setImage(with: imageUrl, transitionDuration: 0) { [weak self] (image, error) in
        guard let _ = image else { return }
        self?.imageView.contentMode = .scaleAspectFill
      }
    }
    titleLabel.text = item.title
    subtitleLabel.text = item.subtitle
    if !item.downloaded {
      if let duration = item.duration {
        timeLabel.text = duration
      } else {
        timeLabel.isHidden = true
        timeIcon.isHidden = true
      }
      distanceLabel.text = item.distance
      ascentLabel.text = item.ascent
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
    timeLabel.text = nil
    timeLabel.isHidden = false
    timeIcon.isHidden = false
    distanceLabel.text = nil
    ascentLabel.text = nil
  }
}
