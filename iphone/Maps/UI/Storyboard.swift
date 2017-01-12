import UIKit

enum Storyboard {

  case LaunchScreen
  case Mapsme
  case Welcome

  var instance: UIStoryboard {
    switch self {
    case .LaunchScreen: return UIStoryboard(name: "LaunchScreen", bundle: Bundle.main)
    case .Mapsme: return UIStoryboard(name: "Mapsme", bundle: Bundle.main)
    case .Welcome: return UIStoryboard(name: "Welcome", bundle: Bundle.main)
    }
  }
}
