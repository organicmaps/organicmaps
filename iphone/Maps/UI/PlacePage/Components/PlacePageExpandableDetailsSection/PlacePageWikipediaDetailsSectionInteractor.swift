protocol WikiDescriptionViewControllerDelegate: AnyObject {
  func didPressWikipedia()
  func didPressMore()
  func didCopy(_ content: String)
}

final class PlacePageWikipediaDetailsSectionInteractor: PlacePageExpandableDetailsSectionInteractor {

  private let presenter: PlacePageExpandableDetailsSectionPresenter
  private let wikiDescriptionHtml: String
  private let showLinkButton: Bool
  weak var delegate: WikiDescriptionViewControllerDelegate?

  init(presenter: PlacePageExpandableDetailsSectionPresenter,
       delegate: WikiDescriptionViewControllerDelegate,
       wikiDescriptionHtml: String,
       showLinkButton: Bool) {
    self.presenter = presenter
    self.delegate = delegate
    self.wikiDescriptionHtml = wikiDescriptionHtml
    self.showLinkButton = showLinkButton
  }

  func handle(_ event: PlacePageExpandableDetailsSectionRequest) {
    let responses = resolve(event)
    presenter.process(responses)
  }

  private func resolve(_ event: PlacePageExpandableDetailsSectionRequest) -> [PlacePageExpandableDetailsSectionResponse] {
    switch event {
    case .viewDidLoad:
      var responses: [PlacePageExpandableDetailsSectionResponse] = []
      if showLinkButton {
        responses.append(.updateAccessory(UIImage(resource: .icPlacepageWiki)))
      }
      responses.append(.updateExpandableText(wikiDescriptionHtml, isHTML: true))
      return responses
    case .didTapIcon,
         .didTapTitle,
         .didLongPressTitle:
      return []
    case .didTapAccessory:
      delegate?.didPressWikipedia()
      return []
    case .didTapExpandableText:
      delegate?.didPressMore()
      return []
    }
  }
}
