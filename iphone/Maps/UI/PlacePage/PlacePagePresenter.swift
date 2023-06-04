protocol PlacePagePresenterProtocol: AnyObject {
  func updatePreviewOffset()
  func layoutIfNeeded()
  func showNextStop()
  func closeAnimated()
  func updateTopBound(_ bound: CGFloat, duration: TimeInterval)
}

class PlacePagePresenter: NSObject {
  private weak var view: PlacePageViewProtocol!
  private let interactor: PlacePageInteractorProtocol
  private let layout: IPlacePageLayout

  init(view: PlacePageViewProtocol,
       interactor: PlacePageInteractorProtocol,
       layout: IPlacePageLayout) {
    self.view = view
    self.interactor = interactor
    self.layout = layout
    view.setLayout(layout)
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
    view.closeAnimated()
  }

  func updateTopBound(_ bound: CGFloat, duration: TimeInterval) {
    interactor.updateTopBound(bound, duration: duration)
  }
}
