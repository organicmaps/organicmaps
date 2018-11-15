import UIKit

@objc(MWMStoryboard)
enum Storyboard: Int {

  case authorization
  case launchScreen
  case main
  case searchFilters
  case settings
  case welcome
  case sharing
  case categorySettings
}

extension UIStoryboard {
  @objc static func instance(_ id: Storyboard) -> UIStoryboard {
    let name: String
    switch id {
    case .authorization: name = "Authorization"
    case .launchScreen: name = "LaunchScreen"
    case .main: name = "Main"
    case .searchFilters: name = "SearchFilters"
    case .settings: name = "Settings"
    case .welcome: name = "Welcome"
    case .sharing: name = "BookmarksSharingFlow"
    case .categorySettings: name = "CategorySettings"
    }
    return UIStoryboard(name: name, bundle: nil)
  }
}
