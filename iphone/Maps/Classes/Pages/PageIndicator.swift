
fileprivate let kDotWidth: CGFloat = 6.0
fileprivate let kExtraDotWidth: CGFloat = kDotWidth * 4

class PageIndicator: UIView {
  private var pageViews: [UIView] = []
  var pageCount = 0 {
    didSet {
      pageViews.removeAll()
      for _ in 0..<pageCount {
        let v = UIView()
        v.layer.cornerRadius = kDotWidth / 2.0
        v.clipsToBounds = true
        pageViews.append(v)
        addSubview(v)
      }
    }
  }

  var currentPage: CGFloat = 0.0 {
    didSet {
      updateLayout()
    }
  }

  var color = UIColor(white: 1.0, alpha: 0.2) {
    didSet {
      updateLayout()
    }
  }

  var activeColor = UIColor(white: 1.0, alpha: 0.7) {
    didSet {
      updateLayout()
    }
  }

  private func updateLayout() {
    for i in 0..<pageCount {
      let v = pageViews[i]

      let d = CGFloat(i) - currentPage
      let ad = abs(d)
      let x = kDotWidth * CGFloat(i * 2) + kExtraDotWidth * min(max(0, d), 1)
      let w = kDotWidth + kExtraDotWidth * max(0, (1 - ad))
      v.frame = CGRect(x: x, y: 0, width: w, height: kDotWidth)

      if ad >= 1 {
        v.backgroundColor = color
      } else {
        v.backgroundColor = UIColor.intermediateColor(color1: activeColor, color2: color, ad)
      }
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    updateLayout()
  }

  override var intrinsicContentSize: CGSize {
    let w = (pageCount > 0) ? (kExtraDotWidth + kDotWidth * CGFloat((pageCount - 1) * 2 + 1)) : 0
    return CGSize(width: w, height: kDotWidth)
  }
}
