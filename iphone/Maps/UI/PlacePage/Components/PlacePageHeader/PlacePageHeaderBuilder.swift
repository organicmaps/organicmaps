class PlacePageHeaderBuilder {    
  static func build(data: PlacePageData,
                    delegate: PlacePageHeaderViewControllerDelegate?,
                    headerType: PlacePageHeaderPresenter.HeaderType) -> PlacePageHeaderViewController {
    let storyboard = UIStoryboard.instance(.placePage)
    let viewController = storyboard.instantiateViewController(ofType: PlacePageHeaderViewController.self);
    let presenter = PlacePageHeaderPresenter(view: viewController,
                                             placePagePreviewData: data.previewData,
                                             objectType: data.objectType,
                                             delegate: delegate,
                                             headerType: headerType)

    viewController.presenter = presenter

    return viewController
  }
}
