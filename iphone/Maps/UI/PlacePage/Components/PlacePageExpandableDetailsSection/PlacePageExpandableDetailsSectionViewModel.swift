struct PlacePageExpandableDetailsSectionViewModel {
  enum ExpandedState {
    case hidden
    case collapsed
    case expanded
  }

  var title: String = ""
  var style: InfoItemView.Style = .regular
  var icon: UIImage? = nil
  var accessory: UIImage? = nil
  var expandableText: String?
  var expandableAttributedText: NSAttributedString?
  var expandedState: ExpandedState
}
