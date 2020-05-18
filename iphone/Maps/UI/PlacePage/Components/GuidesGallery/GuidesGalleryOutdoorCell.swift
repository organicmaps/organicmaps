import UIKit

protocol IGuidesGalleryOutdoorItemViewModel: IGuidesGalleryItemViewModel {
  var distance: String { get }
  var duration: String? { get }
  var ascent: String { get }
}

final class GuidesGalleryOutdoorCell: GuidesGalleryCell {
  @IBOutlet private var timeLabel: UILabel!
  @IBOutlet private var timeIcon: UIImageView!
  @IBOutlet private var distanceLabel: UILabel!
  @IBOutlet private var ascentLabel: UILabel!

  func config(_ item: IGuidesGalleryOutdoorItemViewModel) {
    super.config(item)

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
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    timeLabel.text = nil
    timeLabel.isHidden = false
    timeIcon.isHidden = false
    distanceLabel.text = nil
    ascentLabel.text = nil
  }
}
