struct CategoryInfo: InfoMetadata {
  let category: BookmarkGroup
  
  init(category: BookmarkGroup) {
    self.category = category
  }
}
