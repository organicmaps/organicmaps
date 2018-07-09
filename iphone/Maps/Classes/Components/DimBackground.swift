@objc(MWMDimBackground)
final class DimBackground: SolidTouchView {
  private let mainView: UIView
  private var tapAction: () -> Void

  @objc init(mainView: UIView, tapAction: @escaping () -> Void) {
    self.mainView = mainView
    self.tapAction = tapAction
    super.init(frame: mainView.superview!.bounds)
    backgroundColor = UIColor.fadeBackground()
    autoresizingMask = [.flexibleWidth, .flexibleHeight]
    addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(onTap)))
  }

  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  @objc func setVisible(_ visible: Bool, completion: (() -> Void)?) {
    if visible {
      let sv = mainView.superview!
      frame = sv.bounds
      sv.insertSubview(self, belowSubview: mainView)
      alpha = 0
    } else {
      alpha = 0.8
    }
    UIView.animate(withDuration: kDefaultAnimationDuration,
                   animations: { self.alpha = visible ? 0.8 : 0 },
                   completion: { _ in
                     if !visible {
                       self.removeFromSuperview()
                     }
                     completion?()
    })
  }

  @objc
  private func onTap() {
    tapAction()
  }
}
