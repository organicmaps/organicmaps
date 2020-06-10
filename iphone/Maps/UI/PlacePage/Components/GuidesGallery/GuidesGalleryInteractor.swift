import Foundation

protocol IGuidesGalleryInteractor {
  func galleryItems() -> [GuidesGalleryItem]
  func toggleItemVisibility(_ item: GuidesGalleryItem)
  func isGalleryItemVisible(_ item: GuidesGalleryItem) -> Bool
  func setActiveItem(_ item: GuidesGalleryItem)
  func activeItemId() -> String
  func setGalleryChangedCallback(_ callback: @escaping GalleryChangedBlock)
  func resetGalleryChangedCallback()
}

final class GuidesGalleryInteractor {

}

extension GuidesGalleryInteractor: IGuidesGalleryInteractor {
  func galleryItems() -> [GuidesGalleryItem] {
    GuidesManager.shared().galleryItems()
  }

  func toggleItemVisibility(_ item: GuidesGalleryItem) {
    let groupId = BookmarksManager.shared().getGroupId(item.guideId)
    let visible = BookmarksManager.shared().isCategoryVisible(groupId)
    BookmarksManager.shared().setCategory(groupId, isVisible: !visible)
    Statistics.logEvent(kStatBookmarkVisibilityChange, withParameters: [kStatFrom : kStatMapGallery,
                                                                        kStatAction : visible ? kStatHide : kStatShow,
                                                                        kStatServerId : item.guideId])
  }

  func isGalleryItemVisible(_ item: GuidesGalleryItem) -> Bool {
    let groupId = BookmarksManager.shared().getGroupId(item.guideId)
    return BookmarksManager.shared().isCategoryVisible(groupId)
  }

  func setActiveItem(_ item: GuidesGalleryItem) {
    GuidesManager.shared().setActiveGuide(item.guideId)
  }

  func activeItemId() -> String {
    GuidesManager.shared().activeGuideId()
  }

  func setGalleryChangedCallback(_ callback: @escaping (Bool) -> Void) {
    GuidesManager.shared().setGalleryChangedCallback(callback)
  }

  func resetGalleryChangedCallback() {
    GuidesManager.shared().resetGalleryChangedCallback()
  }
}
