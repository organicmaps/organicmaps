protocol PlacePageExpandableDetailsSectionInteractor: AnyObject {
  func handle(_ event: PlacePageExpandableDetailsSectionRequest)
}

enum PlacePageExpandableDetailsSectionRequest {
  case viewDidLoad
  case didTapIcon
  case didTapTitle
  case didLongPressTitle
  case didTapAccessory
  case didTapExpandableText
}

enum PlacePageExpandableDetailsSectionResponse {
  case expandText
  case updateTitle(String)
  case updateIcon(UIImage?)
  case updateAccessory(UIImage?)
  case updateExpandableText(String?, isHTML: Bool)
}
