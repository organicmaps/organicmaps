extension UIImage {
  static func filled(with color: UIColor, size: CGSize = CGSize(width: 1, height: 1)) -> UIImage {
    let renderer = UIGraphicsImageRenderer(size: size)
    return renderer.image { context in
      color.setFill()
      context.fill(CGRect(origin: .zero, size: size))
    }
  }
}
