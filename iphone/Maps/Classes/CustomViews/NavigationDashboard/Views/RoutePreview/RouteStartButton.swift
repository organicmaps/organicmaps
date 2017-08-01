@objc(MWMRouteStartButton)
final class RouteStartButton: UIButton {
  func statePrepare() {
    isHidden = false
    isEnabled = false
  }

  func stateError() {
    isHidden = alternative(iPhone: true, iPad: false)
    isEnabled = false
  }

  func stateReady() {
    isHidden = false
    isEnabled = true
  }

  override func mwm_refreshUI() {
    super.mwm_refreshUI()
    setBackgroundColor(UIColor.linkBlue(), for: .normal)
    setBackgroundColor(UIColor.linkBlueHighlighted(), for: .highlighted)
  }
}
