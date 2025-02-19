protocol PlacePagePresenterProtocol: AnyObject {
  func updatePreviewOffset()
  func layoutIfNeeded()
  func showNextStop()
  func closeAnimated()
  func showAlert(_ alert: UIAlertController)
  func showShareTrackMenu()
}

final class PlacePagePresenter: NSObject {
  private weak var view: PlacePageViewProtocol!
  private weak var headerView: PlacePageHeaderViewProtocol!

  init(view: PlacePageViewProtocol, headerView: PlacePageHeaderViewProtocol) {
    self.view = view
    self.headerView = headerView
  }
}

// MARK: - PlacePagePresenterProtocol

extension PlacePagePresenter: PlacePagePresenterProtocol {
  func updatePreviewOffset() {
    view.updatePreviewOffset()
  }

  func layoutIfNeeded() {
    view.layoutIfNeeded()
  }

  func showNextStop() {
    view.showNextStop()
  }

  func closeAnimated() {
    view.closeAnimated(completion: nil)
  }

  func showAlert(_ alert: UIAlertController) {
    view.showAlert(alert)
  }

  func showShareTrackMenu() {
    headerView.showShareTrackMenu()
  }
}
