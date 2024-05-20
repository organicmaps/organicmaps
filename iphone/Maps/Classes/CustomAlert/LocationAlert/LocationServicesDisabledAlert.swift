import UIKit

final class LocationServicesDisabledAlert: MWMAlert {

  @objc
  class func alert() -> LocationServicesDisabledAlert? {
    guard let alert = Bundle.main.loadNibNamed("LocationServicesDisabledAlert", owner: nil)?.first as? LocationServicesDisabledAlert else {
      assertionFailure("Error: LocationServicesDisabledAlert failed lo load from nib.")
      return nil
    }
    alert.setNeedsCloseAfterEnterBackground()
    return alert
  }

  @IBAction func okButtonDidTap(_ sender: Any) {
    close(nil)
  }
}
