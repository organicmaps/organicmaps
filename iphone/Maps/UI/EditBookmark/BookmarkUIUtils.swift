import UIKit

func circleImageForColor(_ color: UIColor,
                         frameSize: CGFloat,
                         diameter: CGFloat? = nil,
                         iconName: String? = nil) -> UIImage {
  let renderer = UIGraphicsImageRenderer(size: CGSize(width: frameSize, height: frameSize))
  return renderer.image { context in
    let d = diameter ?? frameSize
    let rect = CGRect(x: (frameSize - d) / 2, y: (frameSize - d) / 2, width: d, height: d)
    context.cgContext.addEllipse(in: rect)
    context.cgContext.setFillColor(color.cgColor)
    context.cgContext.fillPath()

    guard let iconName = iconName, let image = UIImage(named: iconName) else { return }
    image.draw(in: rect.insetBy(dx: 3, dy: 3))
  }
}
