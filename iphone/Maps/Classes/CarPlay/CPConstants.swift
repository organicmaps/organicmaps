import Foundation

struct CPConstants {
  struct TemplateKey {
    static let map = "map_type"
    static let alert = "alert_type"
    static let list = "list_type"
  }
  struct TemplateType {
    static let main = "main"
    static let navigation = "navigation"
    static let preview = "preview"
    static let previewAccepted = "preview_accepted"
    static let previewSettings = "preview_settings"
    static let redirectRoute = "redirect_route"
    static let restoreRoute = "restore_route"
    static let downloadMap = "download_map"
  }
  
  struct ListItemType {
    static let history = "history"
    static let bookmarks = "bookmarks"
    static let bookmarkLists = "bookmark_lists"
    static let searchResults = "search_results"
  }
  
  struct Maneuvers {
    static let primary = "primary"
    static let secondary = "secondary"
  }
  
  struct Trip {
    static let start = "start_point"
    static let end = "end_point"
    static let errorCode = "error_code"
    static let missedCountries = "countries"
  }
}
