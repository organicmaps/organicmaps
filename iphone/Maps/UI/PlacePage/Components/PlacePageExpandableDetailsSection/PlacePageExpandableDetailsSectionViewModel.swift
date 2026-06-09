enum ExpandableText: Equatable {
  case html(String)
  case plain(String)

  var string: String {
    switch self {
    case .html(let string), .plain(let string):
      return string
    }
  }

  func isSameCase(as other: ExpandableText) -> Bool {
    switch (self, other) {
    case (.html, .html), (.plain, .plain):
      return true
    default:
      return false
    }
  }
}

struct PlacePageExpandableDetailsSectionViewModel {
  enum ExpandedState {
    case collapsed
    case expanded
  }

  var title: String = ""
  var style: InfoItemView.Style = .regular
  var icon: UIImage?
  var accessory: UIImage?
  var expandableText: ExpandableText?
  var expandedState: ExpandedState = .collapsed
}
