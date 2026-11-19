private enum Layout {
  /// Start-side legend column: the line bar, walk dots and point icons share this vertical axis.
  static let legendWidth: CGFloat = 28
  static let barWidth: CGFloat = 8
  static let barCornerRadius: CGFloat = 4
  /// Break in the line beside a transfer stop, between the incoming and the outgoing line.
  static let transferGap: CGFloat = 6
  static let dotSize: CGFloat = 5
  static let dotSpacing: CGFloat = 4
  static let walkIconSpacing: CGFloat = 1.5
  static let walkMinHeight: CGFloat = 64
  static let walkIconSize: CGFloat = 28
  static let waypointIconSize: CGFloat = 24
  static let finishIconSize: CGFloat = 28
  static let contentSpacing: CGFloat = 8
  static let lineSpacing: CGFloat = 6
  /// Above and below every stop name, so transfers and plain boarding stops share one rhythm.
  static let stopNameSpacing: CGFloat = 8
  static let badgeHeight: CGFloat = 24
  static let badgeInsets = NSDirectionalEdgeInsets(top: 2, leading: 4, bottom: 2, trailing: 6)
  static let expandAnimationDuration: TimeInterval = 0.15
}

/// Lays out a legend column at the start and the row's content beside it, filling the view.
private func addRow(to view: UIView, legend: UIView, content: UIView,
                    alignment: UIStackView.Alignment = .fill) {
  legend.widthAnchor.constraint(equalToConstant: Layout.legendWidth).isActive = true
  let row = UIStackView(arrangedSubviews: [legend, content])
  row.axis = .horizontal
  row.alignment = alignment
  row.spacing = Layout.contentSpacing
  view.addSubview(row)
  row.alignToSuperview()
}

private func makeLabel(_ text: String?, font: FontStyleSheet, color: TextColorStyleSheet) -> UILabel {
  let label = UILabel()
  label.text = text
  label.numberOfLines = 0
  label.setFontStyle(font, color: color)
  return label
}

/// "first · duration", the duration last on every leg.
private func makeSummaryText(_ first: String?, timeInSec: Int) -> String {
  [first, DurationFormatter.routingDurationString(from: TimeInterval(timeInSec))]
    .compactMap { $0 }
    .joined(separator: " · ")
}

/// A line bar in the legend's center, with only the given ends rounded.
private func addLineBar(to legend: UIView, color: UIColor, roundTop: Bool, roundBottom: Bool) -> UIView {
  let bar = UIView()
  bar.backgroundColor = color
  bar.layer.cornerRadius = Layout.barCornerRadius
  var corners: CACornerMask = []
  if roundTop {
    corners.formUnion([.layerMinXMinYCorner, .layerMaxXMinYCorner])
  }
  if roundBottom {
    corners.formUnion([.layerMinXMaxYCorner, .layerMaxXMaxYCorner])
  }
  bar.layer.maskedCorners = corners
  legend.addSubview(bar)
  bar.translatesAutoresizingMaskIntoConstraints = false
  NSLayoutConstraint.activate([
    bar.widthAnchor.constraint(equalToConstant: Layout.barWidth),
    bar.centerXAnchor.constraint(equalTo: legend.centerXAnchor),
  ])
  return bar
}

/// The selected line's badge; without a line number it shows the transport icon alone.
final class TransitLineBadgeView: UIView {
  init(line: TransitDetailsLine) {
    super.init(frame: .zero)
    backgroundColor = line.color
    layer.setCornerRadius(.buttonSmall)
    let foreground = TransportTransitTrain.foregroundColor(for: line.color)

    let icon = UIImageView(image: line.type.icon)
    icon.tintColor = foreground
    let label = UILabel()
    label.text = line.number
    // Only the font is styled, so it follows Dynamic Type; the color contrasts with the line color.
    label.setFontStyle(.bold14)
    label.textColor = foreground
    label.isHidden = line.number.isEmpty

    let stack = UIStackView(arrangedSubviews: [icon, label])
    stack.spacing = 2
    stack.alignment = .center
    stack.isLayoutMarginsRelativeArrangement = true
    stack.directionalLayoutMargins = Layout.badgeInsets
    addSubview(stack)
    stack.alignToSuperview()
    heightAnchor.constraint(greaterThanOrEqualToConstant: Layout.badgeHeight).isActive = true

    isAccessibilityElement = true
    accessibilityLabel = line.type.accessibilityLabel(number: line.number)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}

/// A ride: boarding stop, line badge, "N stops · duration" (expanding to the stops in between) and exit stop,
/// beside a bar in the line's color.
final class TransitDetailsRideView: UIView {
  private let intermediatesLabel: UILabel
  private let summaryChevron = UIImageView()
  private let canExpand: Bool
  private(set) var isExpanded = false

  init(ride: TransitDetailsRide) {
    canExpand = ride.canExpand
    intermediatesLabel = makeLabel(ride.intermediateStops.joined(separator: "\n"),
                                   font: .regular16, color: .blackSecondary)
    super.init(frame: .zero)

    let content = UIStackView()
    content.axis = .vertical
    content.alignment = .leading
    content.spacing = Layout.lineSpacing
    if let boardStop = ride.boardStop {
      let label = makeLabel(boardStop, font: .medium20, color: .blackPrimary)
      content.addArrangedSubview(label)
      content.setCustomSpacing(Layout.stopNameSpacing, after: label)
    }
    let disclosure = makeDisclosure(ride: ride)
    content.addArrangedSubview(disclosure)
    disclosure.widthAnchor.constraint(equalTo: content.widthAnchor).isActive = true
    intermediatesLabel.isHidden = true
    content.addArrangedSubview(intermediatesLabel)
    content.setCustomSpacing(Layout.stopNameSpacing, after: disclosure)
    content.setCustomSpacing(Layout.stopNameSpacing, after: intermediatesLabel)
    if let exitStop = ride.exitStop {
      content.addArrangedSubview(makeLabel(exitStop, font: .medium20, color: .blackPrimary))
    }

    // At a transfer the line runs on square into the transfer row, which draws the break.
    let legend = UIView()
    let bar = addLineBar(to: legend, color: ride.line.color, roundTop: !ride.startsAtTransfer,
                         roundBottom: !ride.endsAtTransfer)
    NSLayoutConstraint.activate([
      bar.topAnchor.constraint(equalTo: legend.topAnchor),
      bar.bottomAnchor.constraint(equalTo: legend.bottomAnchor),
    ])
    addRow(to: self, legend: legend, content: content)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  /// The line badge over "N stops · duration". As on Android, the whole full-width block expands the stops.
  private func makeDisclosure(ride: TransitDetailsRide) -> UIView {
    let badge = TransitLineBadgeView(line: ride.line)
    let stops = ride.stopCount > 0 ? String(format: L("transit_stops_count"), ride.stopCount) : nil
    let label = makeLabel(makeSummaryText(stops, timeInSec: ride.timeInSec), font: .regular16, color: .blackSecondary)

    // Only a ride with intermediate stops is an expandable disclosure.
    summaryChevron.tintColor = .blackSecondaryText
    summaryChevron.isHidden = !canExpand
    updateChevron()
    let summary = UIStackView(arrangedSubviews: [summaryChevron, label])
    summary.spacing = 4
    summary.alignment = .center

    let disclosure = UIStackView(arrangedSubviews: [badge, summary])
    disclosure.axis = .vertical
    disclosure.alignment = .leading
    disclosure.spacing = Layout.lineSpacing
    if canExpand {
      disclosure.addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(toggleIntermediates)))
      disclosure.isAccessibilityElement = true
      disclosure.accessibilityLabel = [badge.accessibilityLabel, label.text].compactMap { $0 }.joined(separator: ", ")
      disclosure.accessibilityTraits = .button
    }
    return disclosure
  }

  @objc
  func toggleIntermediates() {
    guard canExpand else { return }
    isExpanded.toggle()
    updateChevron()
    UIView.animate(withDuration: Layout.expandAnimationDuration) {
      self.intermediatesLabel.isHidden = !self.isExpanded
      self.intermediatesLabel.alpha = self.isExpanded ? 1 : 0
      self.superview?.layoutIfNeeded()
    }
  }

  private func updateChevron() {
    summaryChevron.image = UIImage(systemName: isExpanded ? "chevron.up" : "chevron.down")
  }
}

/// A same-stop transfer: the shared stop named once, beside a break between the incoming and the outgoing line.
final class TransitDetailsTransferView: UIView {
  init(stop: String, from incomingColor: UIColor, to outgoingColor: UIColor) {
    super.init(frame: .zero)
    let name = makeLabel(stop, font: .medium20, color: .blackPrimary)
    let content = UIStackView(arrangedSubviews: [name])
    content.isLayoutMarginsRelativeArrangement = true
    content.directionalLayoutMargins = NSDirectionalEdgeInsets(top: Layout.stopNameSpacing, leading: 0,
                                                               bottom: Layout.stopNameSpacing, trailing: 0)

    let legend = UIView()
    let incoming = addLineBar(to: legend, color: incomingColor, roundTop: false, roundBottom: true)
    let outgoing = addLineBar(to: legend, color: outgoingColor, roundTop: true, roundBottom: false)
    // The bars align to the name, so both must be in the row first.
    addRow(to: self, legend: legend, content: content)
    NSLayoutConstraint.activate([
      incoming.topAnchor.constraint(equalTo: legend.topAnchor),
      incoming.bottomAnchor.constraint(equalTo: name.centerYAnchor, constant: -Layout.transferGap / 2),
      outgoing.topAnchor.constraint(equalTo: name.centerYAnchor, constant: Layout.transferGap / 2),
      outgoing.bottomAnchor.constraint(equalTo: legend.bottomAnchor),
    ])
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}

/// A walking leg: dots with a walk icon in the legend, "Walking" and "distance · duration" beside them.
final class TransitDetailsWalkView: UIView {
  init(walk: TransitDetailsWalk) {
    super.init(frame: .zero)

    let walkIcon = UIImageView(image: UIImage(resource: .icWalk))
    walkIcon.tintColor = .blackSecondaryText
    walkIcon.contentMode = .scaleAspectFit
    walkIcon.widthAnchor.constraint(equalToConstant: Layout.walkIconSize).isActive = true
    walkIcon.heightAnchor.constraint(equalToConstant: Layout.walkIconSize).isActive = true
    let legend = UIStackView(arrangedSubviews: [Self.dot(), Self.dot(), walkIcon, Self.dot(), Self.dot()])
    legend.axis = .vertical
    legend.alignment = .center
    legend.spacing = Layout.dotSpacing
    legend.setCustomSpacing(Layout.walkIconSpacing, after: legend.arrangedSubviews[1])
    legend.setCustomSpacing(Layout.walkIconSpacing, after: walkIcon)

    let distance = walk.distance.isEmpty ? nil : walk.distance
    let content = UIStackView(arrangedSubviews: [
      makeLabel(L("routing_options_walking"), font: .medium16, color: .blackPrimary),
      makeLabel(makeSummaryText(distance, timeInSec: walk.timeInSec), font: .regular16, color: .blackSecondary),
    ])
    content.axis = .vertical
    content.spacing = Layout.lineSpacing / 2

    addRow(to: self, legend: legend, content: content, alignment: .center)
    heightAnchor.constraint(greaterThanOrEqualToConstant: Layout.walkMinHeight).isActive = true
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private static func dot() -> UIView {
    let dot = UIView()
    dot.backgroundColor = .blackSecondaryText
    dot.layer.cornerRadius = Layout.dotSize / 2
    dot.widthAnchor.constraint(equalToConstant: Layout.dotSize).isActive = true
    dot.heightAnchor.constraint(equalToConstant: Layout.dotSize).isActive = true
    return dot
  }
}

/// A user-added intermediate point with its numbered marker, or the route's destination with the finish flag.
final class TransitDetailsPointView: UIView {
  /// Markers exist for the first 20 points, matching the summary strip.
  private static let maxNumberedMarker = 20

  convenience init(waypointNumber: Int, title: String) {
    let marker = "route-point-\(min(waypointNumber, Self.maxNumberedMarker))"
    self.init(icon: UIImage(named: marker), iconSize: Layout.waypointIconSize, title: title)
  }

  convenience init(destination: String) {
    self.init(icon: UIImage(named: "route_point_finish"), iconSize: Layout.finishIconSize, title: destination)
  }

  private init(icon: UIImage?, iconSize: CGFloat, title: String) {
    super.init(frame: .zero)
    // Aspect fit centers the icon in the legend column's width.
    let iconView = UIImageView(image: icon)
    iconView.contentMode = .scaleAspectFit
    iconView.heightAnchor.constraint(equalToConstant: iconSize).isActive = true
    addRow(to: self, legend: iconView, content: makeLabel(title, font: .medium20, color: .blackPrimary),
           alignment: .center)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}
