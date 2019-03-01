struct BookmarkInfo: InfoMetadata {
  let categoryId: UInt64
  let bookmarkId: UInt64

  init(categoryId: UInt64, bookmarkId: UInt64) {
    self.categoryId = categoryId
    self.bookmarkId = bookmarkId
  }
}
