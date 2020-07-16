@objc class SearchActionBarView: SolidTouchView {
  @IBOutlet private var filterButton: MWMButton!
  @IBOutlet private var changeFilterButton: MWMButton!
  @IBOutlet private var changeFilterView: UIView!
  @IBOutlet private var filterAppliedLabel: UILabel!
  @IBOutlet private var mapButton: MWMButton!
  @IBOutlet private var listButton: MWMButton!
  @IBOutlet private var filterDivider: UIView!
  @IBOutlet private var stackView: UIStackView!

  override func awakeFromNib() {
    super.awakeFromNib()

    hideView(filterButton, isHidden: true, animated: false)
    hideView(changeFilterView, isHidden: true, animated: false)
    hideView(mapButton, isHidden: true, animated: false)
    hideView(listButton, isHidden: true, animated: false)
    hideView(filterDivider, isHidden: true, animated: false)
  }

  @objc func updateForState(_ state: MWMSearchManagerState) {
    hideView(mapButton, isHidden: true, animated: false)
    hideView(listButton, isHidden: true, animated: false)

    iPhoneSpecific {
      switch state {
      case .tableSearch:
        hideView(mapButton, isHidden: false, animated: true)
      case .mapSearch:
        hideView(listButton, isHidden: false, animated: true)
      case .default:
        break
      @unknown default:
        break
      }
    }
  }

  @objc func updateFilterButton(showFilter: Bool, filterCount: Int) {
    filterAppliedLabel.text = "\(filterCount)"
    if showFilter {
      hideView(filterButton, isHidden: filterCount > 0, animated: true)
      hideView(changeFilterView, isHidden: filterCount == 0, animated: true)
      hideView(filterDivider, isHidden: listButton.isHidden && mapButton.isHidden, animated: false)
    } else {
      hideView(filterButton, isHidden: true, animated: false)
      hideView(changeFilterView, isHidden: true, animated: false)
      hideView(filterDivider, isHidden: true, animated: false)
    }
  }

  private func hideView(_ view: UIView, isHidden: Bool, animated: Bool) {
    view.isHidden = isHidden
    if animated {
      UIView.animate(withDuration: kDefaultAnimationDuration / 2,
                     delay: 0,
                     options: [.beginFromCurrentState],
                     animations: {
                      self.layoutIfNeeded()
      }, completion: { complete in
        if complete {
          UIView.animate(withDuration: kDefaultAnimationDuration / 2,
                         delay: 0, options: [.beginFromCurrentState],
                         animations: {
                          view.alpha = isHidden ? 0 : 1
          }, completion: nil)
        }
      })
    } else {
      view.alpha = isHidden ? 0 : 1
      view.isHidden = isHidden
    }
  }
}
