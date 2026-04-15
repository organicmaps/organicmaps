struct BMCDefaultViewModel: BMCViewModel {
  private var sections: [BMCSection] = [.permissions, .categoriesList, .creation]

  func numberOfSections() -> Int {
    sections.count
  }

  func sectionType(section: Int) -> BMCSection {
    sections[section]
  }

  func sectionIndex(section: BMCSection) -> Int {
    sections.index(of: section)!
  }

  func numberOfRows(section _: Int) -> Int {
    1
  }

  func item(indexPath: IndexPath) -> BMCModel {
    switch sectionType(section: indexPath.section) {
    case .permissions: return BMCPermission.signup
    case .categoriesList: return BMCCategory()
    case .creation: return BMCAction.create
    }
  }
}
