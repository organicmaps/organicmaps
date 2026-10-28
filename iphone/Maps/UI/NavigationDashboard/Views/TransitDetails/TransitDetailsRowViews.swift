private enum Layout {
  /// Start-side legend column: the line bar, walk dots and point icons share this vertical axis.
  static let legendWidth: CGFloat = 28
  static let barWidth: CGFloat = 8
  static let barCornerRadius: CGFloat = 4
  /// Break in the line beside a transfer stop, between the incoming and the outgoing line.
  static let transferGap: CGFloat = 6
  static let dotSize: CGFloat = 5
  static let dotSpacing: CGFloat = 4
  static let walkIconSize: CGFloat = 24
  static let pointIconSize: CGFloat = 24
  static let contentSpacing: CGFloat = 8
  static let lineSpacing: CGFloat = 6
  static let badgeHeight: CGFloat = 24
  static let badgeInsets = NSDirectionalEdgeInsets(top: 2, leading: 4, bottom: 2, trailing: 6)
  static let expandAnimationDuration: TimeInterval = 0.15
}

/// Lays out a legend column at the start and the row's content beside it.
private func makeRow(legend: UIView, content: UIView) -> UIStackView {
  legend.widthAnchor.constraint(equalToConstant: Layout.legendWidth).isActive = true
  let row = UIStackView(arrangedSubviews: [legend, content])
  row.axis = .horizontal
  row.alignment = .fill
  row.spacing = Layout.contentSpacing
  return row
}

private func makeLabel(_ text: String?, font: FontStyleSheet, color: TextColorStyleSheet) -> UILabel {
  let label = UILabel()
  label.text = text
  label.numberOfLines = 0
  label.setFontStyle(font, color: color)
  return label
}

private func makeLineBar(color: UIColor, roundTop: Bool, roundBottom: Bool) -> UIView {
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
  return bar
}

/// The selected line's badge: the stops in a ride belong to that line only, unlike the summary strip's
/// parallel-line numbers.
final class TransitLineBadgeView: UIView {
  init(step: MWMRouterTransitStepInfo) {
    super.init(frame: .zero)
    backgroundColor = step.color
    layer.setCornerRadius(.buttonSmall)
    let foreground = TransportTransitTrain.foregroundColor(for: step.color)
    let number = step.lineNumber.isEmpty ? step.number : step.lineNumber

    let icon = UIImageView(image: step.type.icon)
    icon.tintColor = foreground
    let label = UILabel()
    label.text = number
    label.font = TransportTransitTrain.Config.labelTextFont
    label.textColor = foreground
    label.isHidden = number.isEmpty

    let stack = UIStackView(arrangedSubviews: [icon, label])
    stack.spacing = 2
    stack.alignment = .center
    stack.isLayoutMarginsRelativeArrangement = true
    stack.directionalLayoutMargins = Layout.badgeInsets
    addSubview(stack)
    stack.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      stack.topAnchor.constraint(equalTo: topAnchor),
      stack.bottomAnchor.constraint(equalTo: bottomAnchor),
      stack.leadingAnchor.constraint(equalTo: leadingAnchor),
      stack.trailingAnchor.constraint(equalTo: trailingAnchor),
      heightAnchor.constraint(greaterThanOrEqualToConstant: Layout.badgeHeight),
    ])

    isAccessibilityElement = true
    accessibilityLabel = [step.type.spokenName, number].compactMap { $0 }.filter { !$0.isEmpty }.joined(separator: " ")
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
  private let hasIntermediates: Bool
  private(set) var isExpanded = false

  init(ride: TransitDetailsRide) {
    let step = ride.step
    hasIntermediates = !step.intermediateStopNames.isEmpty
    intermediatesLabel = makeLabel(step.intermediateStopNames.joined(separator: "\n"),
                                   font: .regular16, color: .blackSecondary)
    super.init(frame: .zero)

    let content = UIStackView()
    content.axis = .vertical
    content.alignment = .leading
    content.spacing = Layout.lineSpacing
    // A transfer names the shared stop once, as this ride's heading.
    var headingLabel: UILabel?
    if let heading = ride.isTransfer ? step.startStopName : ride.boardStop {
      let label = makeLabel(heading, font: .bold17, color: .blackPrimary)
      content.addArrangedSubview(label)
      headingLabel = label
    }
    content.addArrangedSubview(TransitLineBadgeView(step: step))
    content.addArrangedSubview(makeSummary(step: step))
    intermediatesLabel.isHidden = true
    content.addArrangedSubview(intermediatesLabel)
    if let exitStop = ride.exitStop {
      content.addArrangedSubview(makeLabel(exitStop, font: .bold17, color: .blackPrimary))
    }

    let legend = UIView()
    let row = makeRow(legend: legend, content: content)
    addSubview(row)
    row.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      row.topAnchor.constraint(equalTo: topAnchor),
      row.bottomAnchor.constraint(equalTo: bottomAnchor),
      row.leadingAnchor.constraint(equalTo: leadingAnchor),
      row.trailingAnchor.constraint(equalTo: trailingAnchor),
    ])

    // A line runs on square into the next ride's transfer heading, where it ends rounded just above a small
    // break beside the shared stop; the next line starts rounded just below it.
    let bar = makeLineBar(color: step.color, roundTop: true, roundBottom: !ride.continuesIntoTransfer)
    addBar(bar, to: legend)
    bar.bottomAnchor.constraint(equalTo: legend.bottomAnchor).isActive = true
    if let incomingColor = ride.transferFromColor, let headingLabel {
      let incoming = makeLineBar(color: incomingColor, roundTop: false, roundBottom: true)
      addBar(incoming, to: legend)
      NSLayoutConstraint.activate([
        incoming.topAnchor.constraint(equalTo: legend.topAnchor),
        incoming.bottomAnchor.constraint(equalTo: headingLabel.centerYAnchor, constant: -Layout.transferGap / 2),
        bar.topAnchor.constraint(equalTo: headingLabel.centerYAnchor, constant: Layout.transferGap / 2),
      ])
    } else {
      bar.topAnchor.constraint(equalTo: legend.topAnchor).isActive = true
    }
  }

  private func addBar(_ bar: UIView, to legend: UIView) {
    legend.addSubview(bar)
    bar.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      bar.widthAnchor.constraint(equalToConstant: Layout.barWidth),
      bar.centerXAnchor.constraint(equalTo: legend.centerXAnchor),
    ])
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func makeSummary(step: MWMRouterTransitStepInfo) -> UIView {
    var parts: [String] = []
    if step.stopCount > 0 {
      parts.append(String(format: L("transit_stops_count"), step.stopCount))
    }
    parts.append(DurationFormatter.routingDurationString(from: TimeInterval(step.timeInSec)))
    let label = makeLabel(parts.joined(separator: " · "), font: .regular14, color: .blackSecondary)

    // Only a ride with intermediate stops is an expandable disclosure.
    summaryChevron.tintColor = .blackSecondaryText
    summaryChevron.isHidden = !hasIntermediates
    updateChevron()
    let summary = UIStackView(arrangedSubviews: [summaryChevron, label])
    summary.spacing = 4
    summary.alignment = .center
    if hasIntermediates {
      summary.addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(toggleIntermediates)))
      summary.isAccessibilityElement = true
      summary.accessibilityLabel = label.text
      summary.accessibilityTraits = .button
    }
    return summary
  }

  @objc
  func toggleIntermediates() {
    guard hasIntermediates else { return }
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

/// A walking leg: dots with a walk icon in the legend, "Walking" and "distance · duration" beside them.
final class TransitDetailsWalkView: UIView {
  init(step: MWMRouterTransitStepInfo) {
    super.init(frame: .zero)

    let walkIcon = UIImageView(image: UIImage(resource: .icWalk))
    walkIcon.tintColor = .blackSecondaryText
    walkIcon.contentMode = .scaleAspectFit
    walkIcon.widthAnchor.constraint(equalToConstant: Layout.walkIconSize).isActive = true
    walkIcon.heightAnchor.constraint(equalToConstant: Layout.walkIconSize).isActive = true
    let legendStack = UIStackView(arrangedSubviews: [Self.dot(), Self.dot(), walkIcon, Self.dot(), Self.dot()])
    legendStack.axis = .vertical
    legendStack.alignment = .center
    legendStack.spacing = Layout.dotSpacing
    let legend = UIView()
    legend.addSubview(legendStack)
    legendStack.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      legendStack.centerXAnchor.constraint(equalTo: legend.centerXAnchor),
      legendStack.centerYAnchor.constraint(equalTo: legend.centerYAnchor),
      legendStack.topAnchor.constraint(greaterThanOrEqualTo: legend.topAnchor),
    ])

    // Distance first, then duration, so the duration is last as on the rides ("N stops · duration").
    var parts: [String] = []
    if !step.distance.isEmpty {
      parts.append("\(step.distance) \(step.distanceUnits)")
    }
    parts.append(DurationFormatter.routingDurationString(from: TimeInterval(step.timeInSec)))
    let content = UIStackView(arrangedSubviews: [
      makeLabel(L("routing_options_walking"), font: .semibold16, color: .blackPrimary),
      makeLabel(parts.joined(separator: " · "), font: .regular14, color: .blackSecondary),
    ])
    content.axis = .vertical
    content.spacing = Layout.lineSpacing / 2
    let centered = UIView()
    centered.addSubview(content)
    content.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      content.leadingAnchor.constraint(equalTo: centered.leadingAnchor),
      content.trailingAnchor.constraint(equalTo: centered.trailingAnchor),
      content.centerYAnchor.constraint(equalTo: centered.centerYAnchor),
      content.topAnchor.constraint(greaterThanOrEqualTo: centered.topAnchor),
    ])

    let row = makeRow(legend: legend, content: centered)
    addSubview(row)
    row.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      row.topAnchor.constraint(equalTo: topAnchor),
      row.bottomAnchor.constraint(equalTo: bottomAnchor),
      row.leadingAnchor.constraint(equalTo: leadingAnchor),
      row.trailingAnchor.constraint(equalTo: trailingAnchor),
      legend.heightAnchor.constraint(greaterThanOrEqualTo: legendStack.heightAnchor),
    ])
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

  convenience init(waypointIndex: Int, name: String?) {
    let marker = "route-point-\(min(waypointIndex + 1, Self.maxNumberedMarker))"
    self.init(icon: UIImage(named: marker),
              title: name ?? String(format: L("transit_waypoint_label"), waypointIndex + 1))
  }

  convenience init(destination: String) {
    self.init(icon: UIImage(named: "route_point_finish"), title: destination)
  }

  private init(icon: UIImage?, title: String) {
    super.init(frame: .zero)
    let iconView = UIImageView(image: icon)
    iconView.contentMode = .scaleAspectFit
    let legend = UIView()
    legend.addSubview(iconView)
    iconView.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      iconView.widthAnchor.constraint(equalToConstant: Layout.pointIconSize),
      iconView.heightAnchor.constraint(equalToConstant: Layout.pointIconSize),
      iconView.centerXAnchor.constraint(equalTo: legend.centerXAnchor),
      iconView.centerYAnchor.constraint(equalTo: legend.centerYAnchor),
      legend.heightAnchor.constraint(greaterThanOrEqualTo: iconView.heightAnchor),
    ])

    let row = makeRow(legend: legend, content: makeLabel(title, font: .bold17, color: .blackPrimary))
    row.alignment = .center
    addSubview(row)
    row.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      row.topAnchor.constraint(equalTo: topAnchor),
      row.bottomAnchor.constraint(equalTo: bottomAnchor),
      row.leadingAnchor.constraint(equalTo: leadingAnchor),
      row.trailingAnchor.constraint(equalTo: trailingAnchor),
    ])
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}
