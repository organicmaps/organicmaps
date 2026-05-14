extension UILabel {
  @objc func configureSingleLineAutoScaling() {
    numberOfLines = 1
    allowsDefaultTighteningForTruncation = true
    adjustsFontSizeToFitWidth = true
    minimumScaleFactor = 0.5
  }
}
