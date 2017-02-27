@objc(MWMGalleryModel)
final class GalleryModel: NSObject {
  let title: String
  let items: [GalleryItemModel]

  init(title: String, items: [GalleryItemModel]) {
    self.title = title
    self.items = items
  }
}
