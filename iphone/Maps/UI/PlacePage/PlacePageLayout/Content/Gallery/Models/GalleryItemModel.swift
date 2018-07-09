@objc(MWMGalleryItemModel)
final class GalleryItemModel: NSObject {
  let imageURL: URL
  let previewURL: URL

  @objc init(imageURL: URL, previewURL: URL) {
    self.imageURL = imageURL
    self.previewURL = previewURL
  }
}
