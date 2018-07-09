extension MWMBookmarksManager {
  @nonobjc static func groupsIdList() -> [MWMMarkGroupID] {
    return groupsIdList().map { $0.uint64Value }
  }
}
