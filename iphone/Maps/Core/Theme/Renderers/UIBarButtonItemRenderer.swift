import UIKit
class UIBarButtonItemRenderer {
  class func render(_ control: UIBarButtonItem, style: Style) {
    if let backgroundImage = style.backgroundImage {
      control.setBackgroundImage(backgroundImage, for: .normal, barMetrics: .default)
    }
    if let tintColor = style.tintColor {
      control.tintColor = tintColor
    }
    if let backgroundColor = style.backgroundColor {
      let layer: CALayer = CALayer()
      layer.frame = CGRect(x: 0, y: 0, width: 30, height: 26)
      layer.masksToBounds = true
      layer.backgroundColor = backgroundColor.cgColor
    }
  }
}

