protocol PlacePagePresenterProtocol: AnyObject {
  func layoutIfNeeded()
  func updateVisibleAreaInsets(_ insets: UIEdgeInsets, updatingViewport: Bool)
  func showNextStop()
  func openURL(_ path: String)
  func showActivity(_ activity: ActivityViewController, from sourceView: UIView)
  func showAlert(_ alert: UIAlertController)
  func showInfoAlert(title: String, message: String)
  func showShareSheet(for placePageData: PlacePageData, from sourceView: UIView)
  func showToast(_ message: String)
  func close()
}

final class PlacePagePresenter: NSObject {
  private weak var view: PlacePageViewProtocol!
  private weak var headerView: PlacePageHeaderViewProtocol!
  private weak var mapViewController: MapViewController!

  init(view: PlacePageViewProtocol,
       headerView: PlacePageHeaderViewProtocol,
       mapViewController: MapViewController) {
    self.view = view
    self.headerView = headerView
    self.mapViewController = mapViewController
  }
}

// MARK: - PlacePagePresenterProtocol

extension PlacePagePresenter: PlacePagePresenterProtocol {
  func layoutIfNeeded() {
    view.layoutIfNeeded()
  }

  func updateVisibleAreaInsets(_ insets: UIEdgeInsets, updatingViewport: Bool) {
    mapViewController.updateVisibleAreaInsets(for: self, insets: insets, updatingViewport: updatingViewport)
  }

  func showNextStop() {
    view.showNextStop()
  }

  func showActivity(_ activity: ActivityViewController, from sourceView: UIView) {
    activity.present(inParentViewController: mapViewController, anchorView: sourceView)
  }

  func showShareSheet(for placePageData: PlacePageData, from sourceView: UIView) {
    switch placePageData.objectType {
    case .POI, .bookmark:
      let shareViewController = ActivityViewController.share(forPlacePage: placePageData)
      shareViewController.present(inParentViewController: mapViewController, anchorView: sourceView)
    case .track:
      headerView.showShareTrackMenu()
    default:
      guard let coordinates = LocationManager.lastLocation()?.coordinate else {
        view?.showAlert(UIAlertController.unknownCurrentPosition())
        return
      }
      let activity = ActivityViewController.share(forMyPosition: coordinates)
      activity.present(inParentViewController: mapViewController, anchorView: sourceView)
    }
  }

  func showAlert(_ alert: UIAlertController) {
    guard let view else { return }
    iPadSpecific {
      alert.popoverPresentationController?.sourceView = view.view
      alert.popoverPresentationController?.sourceRect = view.view.frame
    }
    view.showAlert(alert)
  }

  func showInfoAlert(title: String, message: String) {
    MWMAlertViewController.activeAlert().presentInfoAlert(title, text: message)
  }

  func openURL(_ path: String) {
    mapViewController?.openUrl(path, externally: true)
  }

  func showToast(_ message: String) {
    Toast.show(withText: message, alignment: .bottom)
  }

  func close() {
    mapViewController?.dismissPlacePage()
  }
}
