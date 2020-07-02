@objc(MWMRouteStartButton)
final class RouteStartButton: UIButton {
  @objc func statePrepare() {
    isHidden = false
    isEnabled = false
  }

  @objc func stateError() {
    isHidden = true
    isEnabled = false
  }

  @objc func stateReady() {
    isHidden = false
    isEnabled = true
  }

  override func applyTheme() {
    super.applyTheme()
    setBackgroundImage(UIColor.linkBlue().getImage(), for: .normal)
    setBackgroundImage(UIColor.linkBlueHighlighted().getImage(), for: .highlighted)
  }
}
