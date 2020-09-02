@objc enum SearchBarState: Int {
  case ready
  case searching
  case back
}

final class SearchBar: SolidTouchView {
  @IBOutlet private var searchIcon: UIImageView!
  @IBOutlet private var activityIndicator: UIActivityIndicatorView!
  @IBOutlet private var backButton: UIButton!
  @IBOutlet private var searchTextField: SearchTextField!
  @IBOutlet private var stackView: UIStackView!
  @IBOutlet private var bookingSearchView: UIView!
  @IBOutlet private var bookingGuestCountLabel: UILabel!
  @IBOutlet private var bookingDatesLabel: UILabel!

  override var visibleAreaAffectDirections: MWMAvailableAreaAffectDirections { return alternative(iPhone: .top, iPad: .left) }

  override var placePageAreaAffectDirections: MWMAvailableAreaAffectDirections { return alternative(iPhone: [], iPad: .left) }

  override var widgetsAreaAffectDirections: MWMAvailableAreaAffectDirections { return alternative(iPhone: [], iPad: .left) }

  override var trafficButtonAreaAffectDirections: MWMAvailableAreaAffectDirections { return alternative(iPhone: .top, iPad: .left) }

  override var tabBarAreaAffectDirections: MWMAvailableAreaAffectDirections { return alternative(iPhone: [], iPad: .left) }

  @objc var state: SearchBarState = .ready {
    didSet {
      if state != oldValue {
        updateLeftView()
      }
    }
  }

  @objc var isBookingSearchViewHidden: Bool = true {
    didSet {
      if oldValue != isBookingSearchViewHidden {
        if isBookingSearchViewHidden {
          Statistics.logEvent(kStatSearchQuickFilterOpen, withParameters: [kStatCategory: kStatHotel,
                                                                           kStatNetwork: Statistics.connectionTypeString()])
        }
        UIView.animate(withDuration: kDefaultAnimationDuration / 2,
                       delay: 0,
                       options: [.beginFromCurrentState],
                       animations: {
                        if self.isBookingSearchViewHidden {
                          self.bookingSearchView.alpha = 0
                        } else {
                          self.bookingSearchView.isHidden = false
                        }
        }, completion: { complete in
          if complete {
            UIView.animate(withDuration: kDefaultAnimationDuration / 2,
                           delay: 0, options: [.beginFromCurrentState],
                           animations: {
                            if self.isBookingSearchViewHidden {
                              self.bookingSearchView.isHidden = true
                            } else {
                              self.bookingSearchView.alpha = 1
                            }
            }, completion: nil)
          } else {
            self.bookingSearchView.isHidden = self.isBookingSearchViewHidden
            self.bookingSearchView.alpha = self.isBookingSearchViewHidden ? 0 : 1
          }
        })
      }
    }
  }

  private lazy var dateFormatter: DateFormatter = {
    let formatter = DateFormatter()
    formatter.setLocalizedDateFormatFromTemplate("EEE, MMMd")
    return formatter
  }()

  override func awakeFromNib() {
    super.awakeFromNib()
    updateLeftView()
    searchTextField.leftViewMode = UITextField.ViewMode.always
    searchTextField.leftView = UIView(frame: CGRect(x: 0, y: 0, width: 32, height: 32))
    bookingSearchView.isHidden = true
    bookingSearchView.alpha = 0
  }

  private func updateLeftView() {
    searchIcon.isHidden = true
    activityIndicator.isHidden = true
    backButton.isHidden = true

    switch state {
    case .ready:
      searchIcon.isHidden = false
    case .searching:
      activityIndicator.isHidden = false
      activityIndicator.startAnimating()
    case .back:
      backButton.isHidden = false
    }
  }

  @objc func resetGuestCount() {
    bookingGuestCountLabel.text = "?"
  }

  @objc func setGuestCount(_ count: Int) {
    bookingGuestCountLabel.text = "\(count)"
  }

  @objc func resetDates() {
    bookingDatesLabel.text = L("date_picker_сhoose_dates_cta")
  }

  @objc func setDates(checkin: Date, checkout: Date) {
    bookingDatesLabel.text = "\(dateFormatter.string(from: checkin)) – \(dateFormatter.string(from: checkout))".capitalized
  }
}
