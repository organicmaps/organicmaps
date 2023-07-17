extension UIColor {
    var hexString: String {
      let cgColorInRGB = cgColor.converted(to: CGColorSpace(name: CGColorSpace.sRGB)!, intent: .defaultIntent, options: nil)!
              let colorRef = cgColorInRGB.components
              let r = colorRef?[0] ?? 0
              let g = colorRef?[1] ?? 0
              let b = ((colorRef?.count ?? 0) > 2 ? colorRef?[2] : g) ?? 0
              let a = cgColor.alpha

              var color = String(
                  format: "#%02lX%02lX%02lX",
                  lroundf(Float(r * 255)),
                  lroundf(Float(g * 255)),
                  lroundf(Float(b * 255))
              )

              if a < 1 {
                  color += String(format: "%02lX", lroundf(Float(a * 255)))
              }

              return color
    }
}
