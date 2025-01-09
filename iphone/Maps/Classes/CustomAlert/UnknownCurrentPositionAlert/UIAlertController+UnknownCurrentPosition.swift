extension UIAlertController {
  static func unknownCurrentPosition() -> UIAlertController {
    let alert = UIAlertController(title: L("unknown_current_position"), message: nil, preferredStyle: .alert)
    alert.addAction(UIAlertAction(title: L("ok"), style: .default, handler: nil))
    return alert
  }
}
