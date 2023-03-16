
extension UIView {
  @objc var snapshot: UIView {
    guard let contents = layer.contents else {
      return snapshotView(afterScreenUpdates: true)!
    }
    let snapshot: UIView
    if let view = self as? UIImageView {
      snapshot = UIImageView(image: view.image)
      snapshot.bounds = view.bounds
    } else {
      snapshot = UIView(frame: frame)
      snapshot.layer.contents = contents
      snapshot.layer.bounds = layer.bounds
    }
    snapshot.layer.cornerRadius = layer.cornerRadius
    snapshot.layer.masksToBounds = layer.masksToBounds
    snapshot.contentMode = contentMode
    snapshot.transform = transform
    return snapshot
  }
}
