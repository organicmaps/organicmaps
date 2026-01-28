final class PlacePageOSMDescriptionSectionInteractor: PlacePageExpandableDetailsSectionInteractor {

  private let presenter: PlacePageExpandableDetailsSectionPresenter
  private let description: String
  private let isTranslationAllowed: Bool

  init(presenter: PlacePageExpandableDetailsSectionPresenter, description: String) {
    self.presenter = presenter
    self.description = description

    if #available(iOS 18.0, *), !ProcessInfo.processInfo.isiOSAppOnMac {
      self.isTranslationAllowed = true
    } else {
      self.isTranslationAllowed = false
    }
  }

  func handle(_ event: PlacePageExpandableDetailsSectionRequest) {
    let responses = resolve(event)
    presenter.process(responses)
  }

  private func resolve(_ event: PlacePageExpandableDetailsSectionRequest) -> [PlacePageExpandableDetailsSectionResponse] {
    switch event {
    case .viewDidLoad:
      return [.updateAccessory(isTranslationAllowed ? UIImage(resource: .icPlacepageTranslate) : nil)]
    case .didTapIcon, .didTapTitle, .didLongPressTitle:
      return []
    case .didTapAccessory:
      if #available(iOS 18.0, *), let view = presenter.view {
        DefaultTranslationController.shared.presentTranslation(for: description, from: view)
      }
      return []
    case .didTapExpandableText:
      return [.expandText]
    }
  }
}
