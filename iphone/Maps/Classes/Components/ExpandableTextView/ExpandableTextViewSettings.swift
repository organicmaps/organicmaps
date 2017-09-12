import UIKit

struct ExpandableTextViewSettings {
  enum Default {
    static let expandText = "…more"
    static let expandTextColor = UIColor.blue
    static let numberOfCompactLines = 2
    static let textColor = UIColor.darkText
    static let textFont = UIFont.preferredFont(forTextStyle: .body)
  }

  init() {}

  var expandText = Default.expandText
  var expandTextColor = Default.expandTextColor
  var numberOfCompactLines = Default.numberOfCompactLines
  var textColor = Default.textColor
  var textFont = Default.textFont
}
