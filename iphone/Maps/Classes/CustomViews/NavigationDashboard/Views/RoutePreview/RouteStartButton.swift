@objc(MWMRouteStartButton)
final class RouteStartButton: UIButton {
  @objc func statePrepare() {
    isHidden = false
    isEnabled = false
  }

  @objc func stateError() {
    isHidden = alternative(iPhone: true, iPad: false)
    isEnabled = false
  }

  @objc func stateReady() {
    isHidden = false
    isEnabled = true
  }

  override func mwm_refreshUI() {
    super.mwm_refreshUI()
    setBackgroundColor(UIColor.linkBlue(), for: .normal)
    setBackgroundColor(UIColor.linkBlueHighlighted(), for: .highlighted)
  }
}
