extension MWMBookmarksManager {
  static func categoriesIdList() -> [MWMMarkGroupID] {
    return groupsIdList().map { $0.uint32Value }
  }
}
