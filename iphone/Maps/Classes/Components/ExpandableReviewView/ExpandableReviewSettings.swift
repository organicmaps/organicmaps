
struct ExpandableReviewSettings {
  var expandText: String
  var expandTextColor: UIColor
  var numberOfCompactLines: Int
  var textColor: UIColor
  var textFont: UIFont
  
  init(expandText: String = "…more",
       expandTextColor: UIColor = .blue,
       numberOfCompactLines: Int = 2,
       textColor: UIColor = .darkText,
       textFont: UIFont = .preferredFont(forTextStyle: .body)) {
    self.expandText = expandText
    self.expandTextColor = expandTextColor
    self.numberOfCompactLines = numberOfCompactLines
    self.textColor = textColor
    self.textFont = textFont
  }
}
