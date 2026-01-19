struct PlacePageExpandableDetailsSectionViewModel {
  enum ExpandedState {
    case hidden
    case collapsed
    case expanded
  }

  var title: String = ""
  var style: InfoItemView.Style = .regular
  var icon: UIImage?
  var accessory: UIImage?
  var expandableText: String?
  var expandableAttributedText: NSAttributedString?
  var expandedState: ExpandedState
}
