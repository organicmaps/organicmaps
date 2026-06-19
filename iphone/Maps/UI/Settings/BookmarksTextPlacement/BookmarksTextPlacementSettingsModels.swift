enum BookmarksTextPlacementSettingsSection: String {
  case options
}

extension BookmarksTextPlacementSettingsSection {
  var footer: String? {
    switch self {
    case .options: return L("bookmarks_text_placement_description")
    }
  }
}

extension Placement {
  static let allCases: [Placement] = [.none, .right, .bottom]
}

struct BookmarksTextPlacementSettingsState: Equatable {
  var placement: Placement
}

typealias BookmarksTextPlacementSettingsViewController =
  SettingsViewController<BookmarksTextPlacementSettingsSection, Placement>
typealias BookmarksTextPlacementSettingsSectionViewModel =
  SettingsSectionViewModel<BookmarksTextPlacementSettingsSection, Placement>
typealias BookmarksTextPlacementSettingsItemViewModel = SettingsItemViewModel<Placement>
