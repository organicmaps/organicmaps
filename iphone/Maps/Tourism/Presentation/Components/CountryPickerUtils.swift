import UIKit
import SwiftUI
import CountryPickerView

func getCountryPickerView() -> CountryPickerView {
  let cpv = CountryPickerView()
  cpv.translatesAutoresizingMaskIntoConstraints = false
  cpv.textColor = UIKitColor.onBackground
  cpv.showCountryNameInView = true
  cpv.showPhoneCodeInView = false
  cpv.showCountryCodeInView = false
  return cpv
}

struct UICountryPickerView: UIViewRepresentable {
  @State var prevValue: String = ""
  let cpv = getCountryPickerView()
  let onCountryChanged: (String) -> Void
  
  init(code: String, onCountryChanged: @escaping (String) -> Void) {
    prevValue = code
    cpv.setCountryByCode(code)
    self.onCountryChanged = onCountryChanged
    observeCodeAndUpdate()
  }
  
  func observeCodeAndUpdate() {
    Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { _ in
      if cpv.selectedCountry.code != prevValue {
        self.prevValue = cpv.selectedCountry.code
        onCountryChanged(cpv.selectedCountry.code)
      }
    }
  }
  
  func makeUIView(context: Context) -> CountryPickerView {
    return cpv
  }
  
  func updateUIView(_ uiView: CountryPickerView, context: Context) {
    
  }
}


func getCountryAsLabel(code: String) -> CountryPickerView {
  let cpv = CountryPickerView()
  cpv.translatesAutoresizingMaskIntoConstraints = false
  cpv.textColor = UIKitColor.onBackground
  cpv.font = UIKitFont.h4.font
  cpv.showCountryNameInView = true
  cpv.showPhoneCodeInView = false
  cpv.showCountryCodeInView = false
  cpv.isUserInteractionEnabled = false
  cpv.setCountryByCode(code)
  return cpv
}

struct UICountryAsLabelView: UIViewRepresentable {
  let code: String
  init(code: String) {
    self.code = code
  }
  
  func makeUIView(context: Context) -> CountryPickerView {
    return getCountryAsLabel(code: code)
  }
  
  func updateUIView(_ uiView: CountryPickerView, context: Context) {
    // nothing, go home
  }
}
