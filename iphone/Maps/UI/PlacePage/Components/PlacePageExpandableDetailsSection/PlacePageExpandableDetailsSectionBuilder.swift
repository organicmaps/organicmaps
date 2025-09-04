struct PlacePageExpandableDetailsSectionBuilder {
  static func buildWikipediaSection(_ wikiDescriptionHtml: String, showLinkButton: Bool, delegate: WikiDescriptionViewControllerDelegate) -> PlacePageExpandableDetailsSectionViewController {
    let viewModel = PlacePageExpandableDetailsSectionViewModel(title: L("read_in_wikipedia"),
                                                               style: .header,
                                                               expandableText: wikiDescriptionHtml,
                                                               expandedState: .collapsed)

    let presenter = PlacePageExpandableDetailsSectionPresenter(viewModel: viewModel)
    let interactor = PlacePageWikipediaDetailsSectionInteractor(
      presenter: presenter,
      delegate: delegate,
      wikiDescriptionHtml: wikiDescriptionHtml,
      showLinkButton: showLinkButton
    )
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

  static func buildOSMDescriptionSection(_ osmDescription: String) -> PlacePageExpandableDetailsSectionViewController {
    let viewModel = PlacePageExpandableDetailsSectionViewModel(title: "OpenStreetMap",
                                                               style: .header,
                                                               expandableText: osmDescription,
                                                               expandedState: .collapsed)
    let presenter = PlacePageExpandableDetailsSectionPresenter(viewModel: viewModel)
    let interactor = PlacePageOSMDescriptionSectionInteractor(presenter: presenter, description: osmDescription)
    let viewController = PlacePageExpandableDetailsSectionViewController(interactor: interactor)
    presenter.view = viewController
    return viewController
  }

}
