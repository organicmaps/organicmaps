
final class TagsDataSource: NSObject {
  private var tagGroups: [MWMTagGroup] = []
  private var maxTagsNumber: Int = 0
  
  func loadTags(onComplete: @escaping (Bool) -> Void) {
    BookmarksManager.shared().loadTags(withLanguage: AppInfo.shared().twoLetterLanguageId) { tags, maxTagsNumber in
      if let tags = tags {
        self.tagGroups = tags
        self.maxTagsNumber = maxTagsNumber
      }
      onComplete(!self.tagGroups.isEmpty)
    }
  }
  
  var tagGroupsCount: Int {
    get {
      return tagGroups.count
    }
  }

  var maxNumberOfTagsToSelect: Int {
    get {
      return maxTagsNumber
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
