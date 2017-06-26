extension UILabel {
  var numberOfVisibleLines: Int {
    let textSize = CGSize(width: frame.size.width, height: CGFloat(MAXFLOAT))
    let rowHeight = sizeThatFits(textSize).height.rounded()
    let charHeight = font.pointSize.rounded()
    return Int((rowHeight / charHeight).rounded())
  }
}
