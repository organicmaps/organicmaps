import UIKit
import CountryPickerView

func getCountryPickerView() -> CountryPickerView {
  let cpv = CountryPickerView()
  cpv.translatesAutoresizingMaskIntoConstraints = false
  cpv.textColor = .white
  cpv.showCountryNameInView = true
  cpv.showPhoneCodeInView = false
  cpv.showCountryCodeInView = false
  return cpv
}
