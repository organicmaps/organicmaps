final class TagsDataSource: NSObject {
  
  private var tagGroups: [MWMTagGroup] = []
  
  func loadTags(onComplete: @escaping (Bool) -> Void) {
    MWMBookmarksManager.shared().loadTags { tags in
      if let tags = tags {
        self.tagGroups = tags
      }
      onComplete(!self.tagGroups.isEmpty)
    }
  }
  
  var tagGroupsCount: NSInteger {
    get {
      return tagGroups.count
    }
  }
  
  func tagGroup(at index: Int) -> MWMTagGroup {
    return tagGroups[index]
  }
  
  func tagsCount(in groupIndex: Int) -> Int {
    return tagGroups[groupIndex].tags.count
  }
  
  func tag(in groupIndex: Int, at index: Int) -> MWMTag {
    return tagGroups[groupIndex].tags[index]
  }
  
  func tags(for indexPaths: [IndexPath]) -> [MWMTag] {
    return indexPaths.map { self.tag(in: $0.section, at: $0.item) }
  }
}
