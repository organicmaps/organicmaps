@objc(MWMPPPSearchSimilarButton)
final class PPPSearchSimilarButton: MWMTableViewCell {
  typealias TapCallback = () -> Void

  private var callback: TapCallback?

  @objc func config(tap: @escaping TapCallback) {
    callback = tap
  }

  @IBAction private func tap() {
    callback?()
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    let inset = width / 2
    separatorInset = UIEdgeInsets.init(top: 0, left: inset, bottom: 0, right: inset)
  }
}
