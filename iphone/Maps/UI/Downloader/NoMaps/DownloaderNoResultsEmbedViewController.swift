
@objc(MWMDownloaderNoResultsEmbedViewController)
final class DownloaderNoResultsEmbed: UINavigationController {

  @objc(MWMDownloaderNoResultsScreen)
  enum Screen: Int {

    case noMaps
    case noSearchResults
  }

  @objc var screen = Screen.noMaps {
    didSet {
      let controller: MWMViewController
      switch screen {
      case .noMaps: controller = MWMNoMapsViewController.controller()
      case .noSearchResults: controller = SearchNoResultsViewController.controller
      }
      setViewControllers([controller], animated: false)
    }
  }
}
