@objc(MWMStoryboard)
enum Storyboard: Int {
  case main
  case drivingOptions
  case carPlay
  case placePage
}

extension UIStoryboard {
  @objc static func instance(_ id: Storyboard) -> UIStoryboard {
    let name: String
    switch id {
    case .main: name = "Main"
    case .drivingOptions: name = "DrivingOptions"
    case .carPlay: name = "CarPlay"
    case .placePage: name = "PlacePage"
    }
    return UIStoryboard(name: name, bundle: nil)
  }

  func instantiateViewController<T: UIViewController>(ofType: T.Type) -> T {
    let name = String(describing: ofType)
    return instantiateViewController(withIdentifier: name) as! T
  }
}
