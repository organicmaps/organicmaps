@objc class SearchActionBarView: SolidTouchView {
  @IBOutlet private var mapButton: MWMButton!
  @IBOutlet private var listButton: MWMButton!

  @IBOutlet private var stackView: UIStackView!

  override func awakeFromNib() {
    super.awakeFromNib()

    hideView(mapButton, isHidden: true, animated: false)
    hideView(listButton, isHidden: true, animated: false)
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
        case .hidden: fallthrough
        case .result: fallthrough
        @unknown default:
        break
      }
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
