extension UIColor {
  private func convertToHEX(component: CGFloat) -> Int {
    return lroundf(Float(component * 255));
  }
  
  private var hexColorComponentTemplate: String {
    get {
      return "%02lX";
    }
  }
  
  private var hexColorTemplate: String {
    get {
      return "#\(hexColorComponentTemplate)\(hexColorComponentTemplate)\(hexColorComponentTemplate)";
    }
  }
  
  var hexString: String {
    get {
      let cgColorInRGB = cgColor.converted(to: CGColorSpace(name: CGColorSpace.sRGB)!, intent: .defaultIntent, options: nil)!
      let colorRef = cgColorInRGB.components
      let r = colorRef?[0] ?? 0
      let g = colorRef?[1] ?? 0
      let b = ((colorRef?.count ?? 0) > 2 ? colorRef?[2] : g) ?? 0
      let alpha = cgColor.alpha
      
      var color = String(
        format: hexColorTemplate,
        convertToHEX(component: r),
        convertToHEX(component: g),
        convertToHEX(component: b)
      )

      if alpha < 1 {
        color += String(format: hexColorComponentTemplate, convertToHEX(component: alpha))
      }

      return color
    }
  }
}
