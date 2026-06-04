extension UIAlertController {
  @objc(importedBookmarkCategoriesWithCategoryIds:categoryNames:selectCategory:)
  static func importedBookmarkCategories(categoryIds: [NSNumber],
                                         categoryNames: [String],
                                         selectCategory: @escaping (NSNumber) -> Void) -> UIAlertController {
    let alert = UIAlertController(title: L("load_kmz_title"),
                                  message: L("load_kmz_successful"),
                                  preferredStyle: .alert)

    for (categoryId, categoryName) in zip(categoryIds, categoryNames) {
      alert.addAction(UIAlertAction(title: categoryName, style: .default) { _ in
        selectCategory(categoryId)
      })
    }

    alert.addAction(UIAlertAction(title: L("ok"), style: .cancel, handler: nil))
    return alert
  }
}
