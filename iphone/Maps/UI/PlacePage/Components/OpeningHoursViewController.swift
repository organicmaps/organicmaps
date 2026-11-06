import UIKit

/// Whether the place is open now and when that changes, as the place page header and the opening hours show it.
struct OpeningHoursStatus {
  let state: String
  let stateColor: UIColor
  let details: String?

  /// Nil when the state is unknown.
  init?(_ schedule: PlacePageDataSchedule) {
    let now = time_t(Date().timeIntervalSince1970)

    func stringFromTime(_ time: Int) -> String {
      DateTimeFormatter.dateString(from: Date(timeIntervalSince1970: TimeInterval(time)),
                                   dateStyle: .none,
                                   timeStyle: .short)
    }

    switch schedule.state {
    case .unknown:
      return nil

    case .allDay:
      state = L("twentyfour_seven")
      stateColor = UIColor.systemGreen
      details = nil

    case .open:
      let nextTimeClosed = schedule.nextTimeClosed
      let minutesUntilClosed = (nextTimeClosed - now) / 60
      let stringTimeInterval = Self.timeIntervalString(minutes: minutesUntilClosed)
      let stringTime = stringFromTime(nextTimeClosed)

      if minutesUntilClosed < 3 * 60 // Less than 3 hours
      {
        details = String(format: L("closes_in"), stringTimeInterval) + " • " + stringTime
      } else if minutesUntilClosed < 24 * 60 // Less than 24 hours
      {
        details = String(format: L("closes_at"), stringTime)
      } else {
        details = nil
      }

      state = L("editor_time_open")
      stateColor = UIColor.systemGreen

    case .closed:
      let nextTimeOpen = schedule.nextTimeOpen
      let nextTimeOpenDate = Date(timeIntervalSince1970: TimeInterval(nextTimeOpen))

      let minutesUntilOpen = (nextTimeOpen - now) / 60
      let stringTimeInterval = Self.timeIntervalString(minutes: minutesUntilOpen)
      let stringTime = stringFromTime(nextTimeOpen)

      if minutesUntilOpen < 3 * 60 // Less than 3 hours
      {
        details = String(format: L("opens_in"), stringTimeInterval) + " • " + stringTime
      } else if Calendar.current.isDateInToday(nextTimeOpenDate) // Today
      {
        details = String(format: L("opens_at"), stringTime)
      } else if minutesUntilOpen < 24 * 60 // Less than 24 hours
      {
        details = String(format: L("opens_tomorrow_at"), stringTime)
      } else if minutesUntilOpen < 7 * 24 * 60 // Less than 1 week
      {
        let dayOfWeek = DateTimeFormatter.dateString(from: nextTimeOpenDate, format: "EEEE")
        details = String(format: L("opens_dayoftheweek_at"), dayOfWeek, stringTime)
      } else {
        details = nil
      }

      state = L("closed_now")
      stateColor = UIColor.systemRed

    @unknown default:
      fatalError()
    }
  }

  private static func timeIntervalString(minutes: Int) -> String {
    var str = ""
    if minutes >= 60 {
      str = String(minutes / 60) + " " + L("hour") + " "
    }
    str += String(minutes % 60) + " " + L("minute")
    return str
  }
}

class OpeningHoursStatusViewController: UIViewController {
  @IBOutlet var stateLabel: UILabel!
  @IBOutlet var detailsLabel: UILabel!
  @IBOutlet var arrowImageView: UIImageView!

  var status: OpeningHoursStatus!
  var onExpand: MWMVoidBlock?

  override func viewDidLoad() {
    super.viewDidLoad()

    stateLabel.text = status.state
    stateLabel.textColor = status.stateColor
    detailsLabel.text = status.details
    detailsLabel.isHidden = status.details == nil
  }

  @IBAction func onTap(_: UITapGestureRecognizer) {
    onExpand?()
  }
}

class OpeningHoursDayViewController: UIViewController {
  @IBOutlet var todayLabel: UILabel!
  @IBOutlet var scheduleLabel: UILabel!

  var workingDay: WorkingDay!
  var isToday = false

  override func viewDidLoad() {
    super.viewDidLoad()

    todayLabel.text = workingDay.workingDays
    scheduleLabel.text = workingDay.workingTimes
    if isToday {
      todayLabel.setFontStyleAndApply(.regular16, color: .blackPrimary)
      scheduleLabel.setFontStyleAndApply(.regular16, color: .blackPrimary)
    }
  }
}

class OpeningHoursViewController: UIViewController {
  @IBOutlet var stackView: UIStackView!

  private var dayViews: [OpeningHoursDayViewController] = []

  private lazy var statusView: OpeningHoursStatusViewController = {
    let vc = storyboard!.instantiateViewController(ofType: OpeningHoursStatusViewController.self)
    vc.status = status
    return vc
  }()

  private var expanded = false

  var openingHours: OpeningHours!
  var status: OpeningHoursStatus!

  override func viewDidLoad() {
    super.viewDidLoad()

    addToStack(statusView)

    // Open 24/7: the status is all there is to show.
    if openingHours.days.isEmpty {
      statusView.arrowImageView.isHidden = true
      return
    }

    for (index, item) in openingHours.days.enumerated() {
      let vc = createDayItem(item, isToday: index == 0)
      dayViews.append(vc)
      addToStack(vc)
    }

    statusView.onExpand = { [weak self] in
      guard let self else { return }
      self.expanded.toggle()
      UIView.animate(withDuration: AppConstants.defaultAnimationDuration) { [weak self] in
        guard let self else { return }
        for vc in self.dayViews {
          vc.view.isHidden = !self.expanded
        }
        self.statusView.arrowImageView.transform = self.expanded ? CGAffineTransform(rotationAngle: -CGFloat.pi + 0.01)
          : CGAffineTransform.identity
        self.view.layoutIfNeeded()
      }
    }
  }

  private func createDayItem(_ workingDay: WorkingDay, isToday: Bool) -> OpeningHoursDayViewController {
    let vc = storyboard!.instantiateViewController(ofType: OpeningHoursDayViewController.self)
    vc.workingDay = workingDay
    vc.isToday = isToday
    vc.view.isHidden = true
    return vc
  }

  private func addToStack(_ viewController: UIViewController) {
    addChild(viewController)
    stackView.addArrangedSubview(viewController.view)
    viewController.didMove(toParent: self)
  }
}
