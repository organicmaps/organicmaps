import UIKit

@objc(MWMStoryboard)
enum Storyboard: Int {

  case Authorization
  case LaunchScreen
  case Main
  case SearchFilters
  case Settings
  case Welcome
}


extension UIStoryboard {

  static func instance(_ id: Storyboard) -> UIStoryboard {
    let name: String
    switch id {
    case .Authorization: name = "Authorization"
    case .LaunchScreen: name = "LaunchScreen"
    case .Main: name = "Main"
    case .SearchFilters: name = "SearchFilters"
    case .Settings: name = "Settings"
    case .Welcome: name = "Welcome"
    }
    return UIStoryboard(name: name, bundle: nil)
  }
}
