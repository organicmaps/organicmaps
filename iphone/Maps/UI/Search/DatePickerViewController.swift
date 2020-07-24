import UIKit
import DatePicker

@objc protocol DatePickerViewControllerDelegate: AnyObject {
  func datePicker(_ datePicker: DatePickerViewController, didSelectStartDate startDate:Date, endDate:Date)
  func datePickerDidCancel(_ datePicker: DatePickerViewController)
}

@objc final class DatePickerViewController: UIViewController {
  private let transitioning = CoverVerticalModalTransitioning(presentationHeight: 550)

  @IBOutlet var checkInLabel: UILabel!
  @IBOutlet var startDateLabel: UILabel!
  @IBOutlet var endDateLabel: UILabel!
  @IBOutlet var numberOfDaysLabel: UILabel!
  @IBOutlet var doneButton: UIButton!
  @IBOutlet var cancelButton: UIButton!
  @IBOutlet var datePickerView: DatePickerView!

  @objc var delegate: DatePickerViewControllerDelegate?

  lazy var dateFormatter: DateFormatter = {
    let formatter = DateFormatter()
    formatter.setLocalizedDateFormatFromTemplate("EEE, MMMMd")
    return formatter
  }()

  override func viewDidLoad() {
    super.viewDidLoad()
    datePickerView.delegate = self
    doneButton.isEnabled = false
  }

  override var transitioningDelegate: UIViewControllerTransitioningDelegate? {
    get { return transitioning }
    set { }
  }

  override var modalPresentationStyle: UIModalPresentationStyle {
    get { return alternativeSizeClass(iPhone: .custom, iPad: .popover) }
    set { }
  }

  @IBAction func onDone(_ sender: UIButton) {
    guard let startDate = datePickerView.startDate, let endDate = datePickerView.endDate else { fatalError() }
    delegate?.datePicker(self, didSelectStartDate: startDate, endDate: endDate)
  }

  @IBAction func onCancel(_ sender: UIButton) {
    delegate?.datePickerDidCancel(self)
  }
}

extension DatePickerViewController: DatePickerViewDelegate {
  func datePickerView(_ view: DatePickerView, didSelect date: Date) {
    defer {
      doneButton.isEnabled = view.endDate != nil
      if let startDate = view.startDate, let endDate = view.endDate {
        let days = Calendar.current.dateComponents([.day], from: startDate, to: endDate).day!
        numberOfDaysLabel.text = String(format: L("date_picker_amout_of_days"), days)
      } else {
        numberOfDaysLabel.text = nil
      }
    }
    
    guard let startDate = view.startDate else {
      view.startDate = date
      startDateLabel.text = dateFormatter.string(from: date).capitalized
      endDateLabel.text = nil
      return
    }

    if date > startDate && view.endDate == nil {
      guard Calendar.current.dateComponents([.day], from: startDate, to: date).day! <= 30 else {
        MWMAlertViewController.activeAlert().presentDefaultAlert(withTitle: "TODO: can't select more than 30 days", message: nil, rightButtonTitle: L("ok"), leftButtonTitle: nil, rightButtonAction: nil)
        return
      }
      view.endDate = date
      endDateLabel.text = dateFormatter.string(from: date).capitalized
      return
    }

    view.startDate = date
    startDateLabel.text = dateFormatter.string(from: date).capitalized
    endDateLabel.text = nil
  }
}
