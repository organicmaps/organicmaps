protocol PlacePagePresenterProtocol: AnyObject {
  func updatePreviewOffset()
  func layoutIfNeeded()
  func showNextStop()
  func closeAnimated()
  func updateTopBound(_ bound: CGFloat, duration: TimeInterval)
  func showAlert(_ alert: UIAlertController)
}

class PlacePagePresenter: NSObject {
  private weak var view: PlacePageViewProtocol!
  private let interactor: PlacePageInteractorProtocol

  init(view: PlacePageViewProtocol,
       interactor: PlacePageInteractorProtocol) {
    self.view = view
    self.interactor = interactor
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

  func updateTopBound(_ bound: CGFloat, duration: TimeInterval) {
    interactor.updateTopBound(bound, duration: duration)
  }

  func showAlert(_ alert: UIAlertController) {
    view.showAlert(alert)
  }
}
