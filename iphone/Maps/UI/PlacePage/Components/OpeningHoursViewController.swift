import UIKit

final class OpeningHoursTodayView: UIView {
  var onAccessibilityActivate: (() -> Void)?

  override func accessibilityActivate() -> Bool {
    guard let onAccessibilityActivate else { return false }
    onAccessibilityActivate()
    return true
  }
}

class OpeningHoursTodayViewController: UIViewController {
  @IBOutlet var todayLabel: UILabel!
  @IBOutlet var scheduleLabel: UILabel!
  @IBOutlet var breaksLabel: UILabel!
  @IBOutlet var closedLabel: UILabel!
  @IBOutlet var arrowImageView: UIImageView!

  var workingDay: WorkingDay!
  var closedNow = false
  var onExpand: MWMVoidBlock? {
    didSet {
      (view as? OpeningHoursTodayView)?.onAccessibilityActivate = onExpand
      view.accessibilityTraits = onExpand == nil ? .staticText : .button
    }
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    todayLabel.text = workingDay.workingDays
    scheduleLabel.text = workingDay.workingTimes
    breaksLabel.text = workingDay.breaks
    closedLabel.isHidden = !closedNow
    view.isAccessibilityElement = true
    view.accessibilityTraits = .staticText
    view.accessibilityLabel = [todayLabel.text, scheduleLabel.text, breaksLabel.text, closedNow ? closedLabel.text : nil]
      .compactMap { $0?.isEmpty == false ? $0 : nil }
      .joined(separator: ", ")
    for label in [todayLabel, scheduleLabel, breaksLabel, closedLabel] {
      label?.isAccessibilityElement = false
    }
    arrowImageView.isAccessibilityElement = false
  }

  @IBAction func onTap(_: UITapGestureRecognizer) {
    onExpand?()
  }
}

class OpeningHoursDayViewController: UIViewController {
  @IBOutlet var todayLabel: UILabel!
  @IBOutlet var scheduleLabel: UILabel!
  @IBOutlet var breaksLabel: UILabel!

  var workingDay: WorkingDay!

  override func viewDidLoad() {
    super.viewDidLoad()

    todayLabel.text = workingDay.workingDays
    scheduleLabel.text = workingDay.workingTimes
    breaksLabel.text = workingDay.breaks
  }
}

class OpeningHoursViewController: UIViewController {
  @IBOutlet var stackView: UIStackView!

  private var otherDaysViews: [OpeningHoursDayViewController] = []

  private lazy var todayView: OpeningHoursTodayViewController = {
    let vc = storyboard!.instantiateViewController(ofType: OpeningHoursTodayViewController.self)
    vc.workingDay = openingHours.days[0]
    vc.closedNow = openingHours.isClosedNow
    return vc
  }()

  private var expanded = false

  var openingHours: OpeningHours!

  override func viewDidLoad() {
    super.viewDidLoad()

    addToStack(todayView)

    if openingHours.days.count == 1 {
      todayView.arrowImageView.isHidden = true
      return
    }

    for item in openingHours.days.suffix(from: 1) {
      let vc = createDayItem(item)
      otherDaysViews.append(vc)
      addToStack(vc)
    }

    todayView.onExpand = { [weak self] in
      guard let self else { return }
      self.expanded.toggle()
      self.todayView.view.accessibilityValue = L(self.expanded ? "placepage_hours_expanded" : "placepage_hours_collapsed")
      UIView.animate(withDuration: AppConstants.defaultAnimationDuration) { [weak self] in
        guard let self else { return }
        for vc in self.otherDaysViews {
          vc.view.isHidden = !self.expanded
        }
        self.todayView.arrowImageView.transform = self.expanded ? CGAffineTransform(rotationAngle: -CGFloat.pi + 0.01)
          : CGAffineTransform.identity
        self.view.layoutIfNeeded()
      }
    }
    todayView.view.accessibilityValue = L("placepage_hours_collapsed")
  }

  private func createDayItem(_ workingDay: WorkingDay) -> OpeningHoursDayViewController {
    let vc = storyboard!.instantiateViewController(ofType: OpeningHoursDayViewController.self)
    vc.workingDay = workingDay
    vc.view.isHidden = true
    return vc
  }

  private func addToStack(_ viewController: UIViewController) {
    addChild(viewController)
    stackView.addArrangedSubview(viewController.view)
    viewController.didMove(toParent: self)
  }
}
