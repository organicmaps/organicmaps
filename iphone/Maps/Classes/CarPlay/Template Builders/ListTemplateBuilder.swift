import CarPlay

final class ListTemplateBuilder {
  enum ListTemplateType {
    case history
    case bookmarkLists
    case bookmarks(category: BookmarkGroup)
    case searchResults(results: [MWMCarPlaySearchResultObject])
  }
  
  enum BarButtonType {
    case bookmarks
    case search
  }
  
  // MARK: - CPListTemplate bilder
  class func buildListTemplate(for type: ListTemplateType) -> CPListTemplate {
    var title = ""
    var trailingNavigationBarButtons = [CPBarButton]()
    switch type {
    case .history:
      title = L("pick_destination")
      let bookmarksButton = buildBarButton(type: .bookmarks) { _ in
        let listTemplate = ListTemplateBuilder.buildListTemplate(for: .bookmarkLists)
        CarPlayService.shared.pushTemplate(listTemplate, animated: true)
      }
      let searchButton = buildBarButton(type: .search) { _ in
        if CarPlayService.shared.isKeyboardLimited {
          CarPlayService.shared.showKeyboardAlert()
        } else {
          let searchTemplate = SearchTemplateBuilder.buildSearchTemplate()
          CarPlayService.shared.pushTemplate(searchTemplate, animated: true)
        }
      }
      trailingNavigationBarButtons = [searchButton, bookmarksButton]
    case .bookmarkLists:
      title = L("bookmarks")
    case .searchResults:
      title = L("search_results")
    case .bookmarks(let category):
      title = category.title
    }
    let template = CPListTemplate(title: title, sections: [])
    template.trailingNavigationBarButtons = trailingNavigationBarButtons
    obtainResources(for: type, template: template)
    return template
  }
  
  private class func obtainResources(for type: ListTemplateType, template: CPListTemplate) {
    switch type {
    case .history:
      obtainHistory(template: template)
    case .bookmarks(let category):
      obtainBookmarks(template: template, categoryId: category.categoryId)
    case .bookmarkLists:
      obtainCategories(template: template)
    case .searchResults(let results):
      convertSearchResults(results, template: template)
    }
  }
  
  private class func obtainHistory(template: CPListTemplate) {
    let searchQueries = FrameworkHelper.obtainLastSearchQueries()
    let items = searchQueries.map({ (text) -> CPListItem in
      let item = CPListItem(text: text, detailText: nil, image: UIImage(named: "ic_carplay_recent"))
      item.userInfo = ListItemInfo(type: CPConstants.ListItemType.history,
                                   metadata: nil)
      return item
    })
    let section = CPListSection(items: items)
    template.updateSections([section])
  }
  
  private class func obtainCategories(template: CPListTemplate) {
    let bookmarkManager = BookmarksManager.shared()
    let categories = bookmarkManager.userCategories()
    let items: [CPListItem] = categories.compactMap({ category in
      if category.bookmarksCount == 0 { return nil }
      let placesString = category.placesCountTitle()
      let item = CPListItem(text: category.title, detailText: placesString)
      item.userInfo = ListItemInfo(type: CPConstants.ListItemType.bookmarkLists,
                                   metadata: CategoryInfo(category: category))
      return item
    })
    let section = CPListSection(items: items)
    template.updateSections([section])
  }
  
  private class func obtainBookmarks(template: CPListTemplate, categoryId: MWMMarkGroupID) {
    let bookmarkManager = BookmarksManager.shared()
    let bookmarks = bookmarkManager.bookmarks(forCategory: categoryId)
    let items = bookmarks.map({ (bookmark) -> CPListItem in
      let item = CPListItem(text: bookmark.prefferedName, detailText: bookmark.address)
      item.userInfo = ListItemInfo(type: CPConstants.ListItemType.bookmarks,
                                   metadata: BookmarkInfo(categoryId: categoryId,
                                                          bookmarkId: bookmark.bookmarkId))
      return item
    })
    let section = CPListSection(items: items)
    template.updateSections([section])
  }
  
  private class func convertSearchResults(_ results: [MWMCarPlaySearchResultObject], template: CPListTemplate) {
    var items = [CPListItem]()
    for object in results {
      let item = CPListItem(text: object.title, detailText: object.address)
      item.userInfo = ListItemInfo(type: CPConstants.ListItemType.searchResults,
                                   metadata: SearchResultInfo(originalRow: object.originalRow))
      items.append(item)
    }
    let section = CPListSection(items: items)
    template.updateSections([section])
  }
  
  // MARK: - CPBarButton builder
  private class func buildBarButton(type: BarButtonType, action: ((CPBarButton) -> Void)?) -> CPBarButton {
    switch type {
    case .bookmarks:
      let button = CPBarButton(type: .image, handler: action)
      button.image = UIImage(named: "ic_carplay_bookmark")
      return button
    case .search:
      let button = CPBarButton(type: .image, handler: action)
      button.image = UIImage(named: "ic_carplay_keyboard")
      return button
    }
  }
}
