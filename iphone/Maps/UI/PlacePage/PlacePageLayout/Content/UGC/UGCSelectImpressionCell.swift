@objc(MWMUGCSelectImpressionCell)
final class UGCSelectImpressionCell: MWMTableViewCell {
  @IBOutlet private var buttons: [UIButton]!
  private weak var delegate: MWMPlacePageButtonsProtocol?

  @objc func configWith(delegate: MWMPlacePageButtonsProtocol?) {
    self.delegate = delegate
  }

  @IBAction private func tap(on: UIButton) {
    buttons.forEach { $0.isSelected = false }
    on.isSelected = true
    delegate?.review(on: on.tag)
  }
}
