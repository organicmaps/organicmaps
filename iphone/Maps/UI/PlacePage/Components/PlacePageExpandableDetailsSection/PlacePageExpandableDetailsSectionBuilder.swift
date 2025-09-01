struct PlacePageExpandableDetailsSectionBuilder {
  static func buildWikipediaSection(_ wikiDescriptionHtml: String, delegate: WikiDescriptionViewControllerDelegate) -> PlacePageExpandableDetailsSectionViewController {
    let viewModel = PlacePageExpandableDetailsSectionViewModel(title: L("read_in_wikipedia"),
                                                               style: .header,
                                                               accessory: UIImage(resource: .icPlacepageWiki),
                                                               expandableText: wikiDescriptionHtml,
                                                               expandedState: .collapsed)
    let presenter = PlacePageExpandableDetailsSectionPresenter(viewModel: viewModel, isHTML: true)
    let interactor = PlacePageWikipediaDetailsSectionInteractor(presenter: presenter, delegate: delegate)
    let viewController = PlacePageExpandableDetailsSectionViewController(interactor: interactor)
    presenter.view = viewController
    return viewController
  }

  static func buildEditBookmarkAndTrackSection(data: PlacePageEditData?, delegate: PlacePageEditBookmarkOrTrackViewControllerDelegate) -> PlacePageExpandableDetailsSectionViewController {
    let viewModel = PlacePageExpandableDetailsSectionViewModel(style: .link, accessory: .ic24PxEdit, expandedState: .hidden)
    let presenter = PlacePageExpandableDetailsSectionPresenter(viewModel: viewModel)
    let interactor = PlacePageEditBookmarkAndTrackSectionInteractor(presenter: presenter, data: data, delegate: delegate)
    let viewController = PlacePageExpandableDetailsSectionViewController(interactor: interactor)
    presenter.view = viewController
    return viewController
  }
}
