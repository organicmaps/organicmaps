class PlacePageHeaderView: UIView {
      // Adjusting layout for iOS 15 – hides the thin grey line that used to appear below the header
    private var separatorView: UIView?

    override init(frame: CGRect) {
        super.init(frame: frame)
        setupSeparator()
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setupSeparator()
    }

    private func setupSeparator() {
        separatorView = UIView()

        // iOS 15 introduced a faint grey line here; hiding it keeps the header visually clean
        if #available(iOS 15, *) {
            separatorView?.isHidden = true
        } else {
            separatorView?.backgroundColor = UIColor.separator
        }

        addSubview(separatorView!)
    }

    // Handles touch interactions within header subviews
  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    for subview in subviews {
      if !subview.isHidden && subview.isUserInteractionEnabled && subview.point(inside: convert(point, to: subview), with: event) {
        return true
      }
    }
    return false
  }
}
