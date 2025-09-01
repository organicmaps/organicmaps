protocol WikiDescriptionViewControllerDelegate: AnyObject {
  func didPressWikipedia()
  func didPressMore()
}

final class PlacePageWikipediaDetailsSectionInteractor: PlacePageExpandableDetailsSectionInteractor {

  let presenter: PlacePageExpandableDetailsSectionPresenter
  weak var delegate: WikiDescriptionViewControllerDelegate?

  init(presenter: PlacePageExpandableDetailsSectionPresenter, delegate: WikiDescriptionViewControllerDelegate) {
    self.presenter = presenter
    self.delegate = delegate
  }

  func handle(_ event: PlacePageExpandableDetailsSectionRequest) {
    let response = resolve(event)
    presenter.process(response)
  }

  private func resolve(_ event: PlacePageExpandableDetailsSectionRequest) -> PlacePageExpandableDetailsSectionResponse {
    switch event {
    case .viewDidLoad:
      return .initialize
    case .didTapIcon:
      return .none
    case .didTapTitle:
      return .none
    case .didLongPressTitle:
      return .none
    case .didTapAccessory:
      delegate?.didPressWikipedia()
      return .none
    case .didTapExpand:
      delegate?.didPressMore()
      return .none
    }
  }
}
