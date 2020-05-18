import UIKit

protocol IGuidesGalleryCityItemViewModel: IGuidesGalleryItemViewModel {
  var info: String { get }
}

final class GuidesGalleryCityCell: GuidesGalleryCell {
  @IBOutlet private var infoLabel: UILabel!

  func config(_ item: IGuidesGalleryCityItemViewModel) {
    super.config(item)

    if !item.downloaded {
      infoLabel.text = item.info
    }
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    infoLabel.text = nil
  }
}
