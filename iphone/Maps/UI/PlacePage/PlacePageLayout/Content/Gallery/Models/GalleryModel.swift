@objc(MWMGalleryModel)
final class GalleryModel: NSObject {
  let title: String
  @objc let items: [GalleryItemModel]

  @objc init(title: String, items: [GalleryItemModel]) {
    self.title = title
    self.items = items
  }
}
