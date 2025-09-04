final class PlacePageOSMDescriptionSectionInteractor: PlacePageExpandableDetailsSectionInteractor {

  var description: String
  var presenter: PlacePageExpandableDetailsSectionPresenter

  init(description: String, presenter: PlacePageExpandableDetailsSectionPresenter) {
    self.description = description
    self.presenter = presenter
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
      if #available(iOS 18.0, *), let view = presenter.view {
        DefaultTranslationController.shared.presentTranslation(for: description,
                                                               from: view)
      }
      return .none
    case .didTapExpand:
      return .expandText
    }
  }
}
