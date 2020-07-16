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
    get { return .custom }
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
    }
    
    guard let startDate = view.startDate else {
      view.startDate = date
      startDateLabel.text = dateFormatter.string(from: date).capitalized
      endDateLabel.text = nil
      return
    }

    if date > startDate && view.endDate == nil {
      view.endDate = date
      endDateLabel.text = dateFormatter.string(from: date).capitalized
      return
    }

    view.startDate = date
    startDateLabel.text = dateFormatter.string(from: date).capitalized
    endDateLabel.text = nil
  }
}
