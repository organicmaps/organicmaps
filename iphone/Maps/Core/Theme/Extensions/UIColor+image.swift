extension UIColor {
  func getImage() -> UIImage {
    let renderer = UIGraphicsImageRenderer(size: CGSize(width: 1, height: 1))
    return renderer.image { context in
      setFill()
      context.fill(CGRect(x: 0, y: 0, width: 1, height: 1))
    }
  }
}
