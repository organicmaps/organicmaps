@objc protocol GuestsPickerViewControllerDelegate: AnyObject {
  func guestsPicker(_ guestsPicker: GuestsPickerViewController,
                    didSelectRooms rooms: Int,
                    adults: Int,
                    children: Int,
                    infants: Int)
  func guestsPickerDidCancel(_ guestsPicker: GuestsPickerViewController)
}

@objc final class GuestsPickerViewController: UIViewController {
  private let transitioning = CoverVerticalModalTransitioning(presentationHeight: 320)

  @IBOutlet var roomsStepper: ValueStepperView!
  @IBOutlet var adultsStepper: ValueStepperView!
  @IBOutlet var childrenStepper: ValueStepperView!
  @IBOutlet var infantsStepper: ValueStepperView!

  @objc weak var delegate: GuestsPickerViewControllerDelegate?

  override func viewDidLoad() {
    super.viewDidLoad()
    roomsStepper.minValue = 1
    roomsStepper.maxValue = 10
    adultsStepper.minValue = 1
    adultsStepper.maxValue = 20
    adultsStepper.value = 2
    childrenStepper.maxValue = 20
    infantsStepper.maxValue = 20
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    preferredContentSize = view.systemLayoutSizeFitting(UIView.layoutFittingCompressedSize,
                                                        withHorizontalFittingPriority: .required,
                                                        verticalFittingPriority: .fittingSizeLevel)
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
    delegate?.guestsPicker(self,
                           didSelectRooms: roomsStepper.value,
                           adults: adultsStepper.value,
                           children: childrenStepper.value,
                           infants: infantsStepper.value)
  }

  @IBAction func onCancel(_ sender: UIButton) {
    delegate?.guestsPickerDidCancel(self)
  }
}
