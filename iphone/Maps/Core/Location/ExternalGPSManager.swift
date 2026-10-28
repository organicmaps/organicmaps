import CoreLocation
import ExternalAccessory
import Foundation

// Manages connections to external Bluetooth Classic GPS receivers (e.g. Bad Elf / Dual XGPS series).
// Reads NMEA 0183 sentences from the accessory stream and feeds parsed positions into LocationManager.
@objc final class ExternalGPSManager: NSObject {
  @objc static let shared = ExternalGPSManager()

  private static let supportedProtocols = ["com.dualav.xgps150"]

  private var sessions: [Int: EASession] = [:]  // keyed by EAAccessory.connectionID
  private var altCache: [Int: AltState] = [:]   // altitude + HDOP from most recent GGA

  private struct AltState {
    var altitude: Double = 0
    var hdop: Double = 5.0
  }

  private override init() { super.init() }

  @objc func start() {
    LOG(.info, "ExternalGPSManager: start — scanning for connected accessories")
    let manager = EAAccessoryManager.shared()
    manager.registerForLocalNotifications()
    NotificationCenter.default.addObserver(self,
                                           selector: #selector(accessoryConnected(_:)),
                                           name: .EAAccessoryDidConnect, object: nil)
    NotificationCenter.default.addObserver(self,
                                           selector: #selector(accessoryDisconnected(_:)),
                                           name: .EAAccessoryDidDisconnect, object: nil)
    let connected = manager.connectedAccessories
    LOG(.info, "ExternalGPSManager: \(connected.count) accessory(ies) already connected")
    for accessory in connected {
      LOG(.info, "ExternalGPSManager: found '\(accessory.name)' protocols: \(accessory.protocolStrings)")
      open(accessory)
    }
  }

  @objc func stop() {
    LOG(.info, "ExternalGPSManager: stop — closing \(sessions.count) session(s)")
    NotificationCenter.default.removeObserver(self)
    EAAccessoryManager.shared().unregisterForLocalNotifications()
    sessions.values.forEach { closeSession($0) }
    sessions.removeAll()
    altCache.removeAll()
    ExternalGPSManager.lineBuffers.removeAll()
    LocationManager.setExternalGPSActive(false)
  }

  @objc private func accessoryConnected(_ note: Notification) {
    guard let acc = note.userInfo?[EAAccessoryKey] as? EAAccessory else { return }
    LOG(.info, "ExternalGPSManager: accessory connected — '\(acc.name)' protocols: \(acc.protocolStrings)")
    open(acc)
  }

  @objc private func accessoryDisconnected(_ note: Notification) {
    guard let acc = note.userInfo?[EAAccessoryKey] as? EAAccessory else { return }
    LOG(.info, "ExternalGPSManager: accessory disconnected — '\(acc.name)' id:\(acc.connectionID)")
    let id = acc.connectionID
    sessions[id].map { closeSession($0) }
    sessions.removeValue(forKey: id)
    altCache.removeValue(forKey: id)
    ExternalGPSManager.lineBuffers.removeValue(forKey: id)
    if sessions.isEmpty {
      LocationManager.setExternalGPSActive(false)
    }
  }

  private func open(_ accessory: EAAccessory) {
    let id = accessory.connectionID
    guard sessions[id] == nil else {
      LOG(.debug, "ExternalGPSManager: '\(accessory.name)' id:\(id) already has an open session — skipping")
      return
    }
    guard let proto = accessory.protocolStrings.first(where: {
      ExternalGPSManager.supportedProtocols.contains($0)
    }) else {
      LOG(.warning, "ExternalGPSManager: '\(accessory.name)' — no supported protocol found. Device advertises: \(accessory.protocolStrings). Supported: \(ExternalGPSManager.supportedProtocols)")
      return
    }
    guard let session = EASession(accessory: accessory, forProtocol: proto) else {
      LOG(.warning, "ExternalGPSManager: '\(accessory.name)' — EASession init failed for protocol '\(proto)'")
      return
    }
    LOG(.info, "ExternalGPSManager: opened session for '\(accessory.name)' using protocol '\(proto)'")
    sessions[id] = session
    altCache[id] = AltState()
    LocationManager.setExternalGPSActive(true)
    session.inputStream?.delegate = self
    session.inputStream?.schedule(in: .main, forMode: .default)
    session.inputStream?.open()
    session.outputStream?.schedule(in: .main, forMode: .default)
    session.outputStream?.open()
  }

  private func closeSession(_ session: EASession) {
    LOG(.info, "ExternalGPSManager: closing session for '\(session.accessory?.name ?? "unknown")'")
    session.inputStream?.delegate = nil
    session.inputStream?.remove(from: .main, forMode: .default)
    session.inputStream?.close()
    session.outputStream?.remove(from: .main, forMode: .default)
    session.outputStream?.close()
  }
}

// MARK: - StreamDelegate

extension ExternalGPSManager: StreamDelegate {
  func stream(_ aStream: Stream, handle event: Stream.Event) {
    switch event {
    case .hasBytesAvailable:
      guard let input = aStream as? InputStream,
            let id = sessions.first(where: { $0.value.inputStream === input })?.key else { return }
      var raw = [UInt8](repeating: 0, count: 1024)
      let n = input.read(&raw, maxLength: raw.count)
      guard n > 0, let chunk = String(bytes: raw[0..<n], encoding: .ascii) else {
        // Binary status packet from device (proprietary, non-ASCII) — expected, not an error.
        // LOG(.debug, "ExternalGPSManager: dropped \(n)-byte binary packet")
        return
      }
      parseChunk(chunk, id: id)
    case .errorOccurred:
      LOG(.warning, "ExternalGPSManager: stream error — \(aStream.streamError?.localizedDescription ?? "unknown")")
    case .endEncountered:
      LOG(.info, "ExternalGPSManager: stream end encountered")
    default:
      break
    }
  }
}

// MARK: - NMEA parsing

extension ExternalGPSManager {
  private static var lineBuffers: [Int: String] = [:]

  private func parseChunk(_ chunk: String, id: Int) {
    ExternalGPSManager.lineBuffers[id, default: ""] += chunk
    var buf = ExternalGPSManager.lineBuffers[id]!
    var sentenceCount = 0

    // NMEA sentences have no guaranteed line terminator on this device; delimit by $…*XX.
    while let dollarIdx = buf.firstIndex(of: "$") {
      if dollarIdx > buf.startIndex {
        buf.removeSubrange(..<dollarIdx)  // discard anything before '$'
      }
      let afterDollar = buf.index(after: buf.startIndex)
      guard afterDollar < buf.endIndex,
            let starIdx = buf[afterDollar...].firstIndex(of: "*"),
            buf.distance(from: buf.index(after: starIdx), to: buf.endIndex) >= 2 else {
        break  // incomplete sentence — wait for more data
      }
      let checksumEnd = buf.index(buf.index(after: starIdx), offsetBy: 2)
      let sentence = String(buf[..<checksumEnd])
      var afterSentence = checksumEnd
      while afterSentence < buf.endIndex && (buf[afterSentence] == "\r" || buf[afterSentence] == "\n") {
        afterSentence = buf.index(after: afterSentence)
      }
      buf.removeSubrange(..<afterSentence)
      sentenceCount += 1
      parseSentence(sentence, id: id)
    }

    ExternalGPSManager.lineBuffers[id] = buf
  }

  private func parseSentence(_ sentence: String, id: Int) {
    guard checksumValid(sentence) else {
      LOG(.warning, "ExternalGPSManager: checksum FAILED for: \(sentence)")
      return
    }
    let body = String(sentence.dropFirst()).components(separatedBy: "*").first ?? ""
    let fields = body.components(separatedBy: ",")
    guard let type = fields.first else { return }
    switch type {
    case "GPRMC", "GNRMC", "GLRMC": parseRMC(fields, id: id)
    case "GPGGA", "GNGGA", "GLGGA": parseGGA(fields, id: id)
    default: break
    }
  }

  // RMC: recommended minimum — provides lat/lon/speed/course/timestamp
  private func parseRMC(_ f: [String], id: Int) {
    guard f.count >= 10 else {
      LOG(.warning, "ExternalGPSManager: RMC too short (\(f.count) fields)")
      return
    }
    guard f[2] == "A" else {
      LOG(.debug, "ExternalGPSManager: RMC status '\(f[2])' (not 'A') — no fix yet")
      return
    }
    guard let lat = nmealat(f[3], f[4]) else {
      LOG(.warning, "ExternalGPSManager: RMC failed to parse latitude '\(f[3])' '\(f[4])'")
      return
    }
    guard let lon = nmealon(f[5], f[6]) else {
      LOG(.warning, "ExternalGPSManager: RMC failed to parse longitude '\(f[5])' '\(f[6])'")
      return
    }
    guard let ts = nmeaDate(time: f[1], date: f[9]) else {
      LOG(.warning, "ExternalGPSManager: RMC failed to parse time '\(f[1])' date '\(f[9])'")
      return
    }

    let speed  = f.count > 7 ? (Double(f[7]).map { $0 * 0.514_444 } ?? -1) : -1  // kt → m/s
    let course = f.count > 8 ? (Double(f[8]) ?? -1) : -1

    let alt = altCache[id] ?? AltState()
    let hAcc = max(alt.hdop * 5.0, 1.0)
    let vAcc = max(alt.hdop * 8.0, 2.0)

    // LOG(.debug, "ExternalGPSManager: position lat=\(String(format: "%.6f", lat)) lon=\(String(format: "%.6f", lon)) alt=\(String(format: "%.1f", alt.altitude))m hAcc=\(String(format: "%.1f", hAcc))m speed=\(String(format: "%.2f", speed))m/s course=\(String(format: "%.1f", course))°")

    let location = CLLocation(
      coordinate: CLLocationCoordinate2D(latitude: lat, longitude: lon),
      altitude: alt.altitude,
      horizontalAccuracy: hAcc,
      verticalAccuracy: vAcc,
      course: course,
      speed: speed,
      timestamp: ts)
    LocationManager.processExternalGPSLocation(location)
  }

  // GGA: fix data — altitude and HDOP, cached for the next RMC
  private func parseGGA(_ f: [String], id: Int) {
    guard f.count >= 10 else {
      LOG(.warning, "ExternalGPSManager: GGA too short (\(f.count) fields)")
      return
    }
    guard let q = Int(f[6]), q > 0 else {
      LOG(.debug, "ExternalGPSManager: GGA fix quality '\(f[6])' — no fix")
      return
    }
    let altitude = Double(f[9]) ?? 0
    let hdop = Double(f[8]) ?? 5.0
    altCache[id] = AltState(altitude: altitude, hdop: hdop)
  }

  private func nmealat(_ s: String, _ hem: String) -> Double? {
    nmeaDeg(s).map { hem == "S" ? -$0 : $0 }
  }

  private func nmealon(_ s: String, _ hem: String) -> Double? {
    nmeaDeg(s).map { hem == "W" ? -$0 : $0 }
  }

  // Convert NMEA DDMM.MMMM / DDDMM.MMMM to decimal degrees.
  // The minute field is always exactly 2 digits before the decimal point.
  private func nmeaDeg(_ s: String) -> Double? {
    guard !s.isEmpty, let dotIdx = s.firstIndex(of: "."),
          dotIdx >= s.index(s.startIndex, offsetBy: 2) else { return nil }
    let minStart = s.index(dotIdx, offsetBy: -2)
    guard let deg = Double(s[..<minStart]),
          let min = Double(s[minStart...]) else { return nil }
    return deg + min / 60.0
  }

  // Parse HHMMSS.ss (UTC) + DDMMYY into a Date.
  private func nmeaDate(time t: String, date d: String) -> Date? {
    guard t.count >= 6, d.count >= 6 else { return nil }
    let ti = t.startIndex
    let di = d.startIndex
    guard let h   = Int(t[ti..<t.index(ti, offsetBy: 2)]),
          let m   = Int(t[t.index(ti, offsetBy: 2)..<t.index(ti, offsetBy: 4)]),
          let sec = Double(t[t.index(ti, offsetBy: 4)...]),
          let day = Int(d[di..<d.index(di, offsetBy: 2)]),
          let mon = Int(d[d.index(di, offsetBy: 2)..<d.index(di, offsetBy: 4)]),
          let yy  = Int(d[d.index(di, offsetBy: 4)...]) else { return nil }

    let year = yy < 80 ? 2000 + yy : 1900 + yy
    var cal = Calendar(identifier: .gregorian)
    cal.timeZone = TimeZone(identifier: "UTC")!
    return cal.date(from: DateComponents(
      year: year, month: mon, day: day,
      hour: h, minute: m, second: Int(sec),
      nanosecond: Int((sec - Double(Int(sec))) * 1_000_000_000)))
  }

  // Validate NMEA checksum: XOR of bytes between '$' (excl.) and '*' (excl.)
  private func checksumValid(_ s: String) -> Bool {
    guard let dollarIdx = s.firstIndex(of: "$"),
          let starIdx   = s.firstIndex(of: "*"),
          starIdx > dollarIdx else { return false }
    let body     = s[s.index(after: dollarIdx)..<starIdx]
    let computed = body.utf8.reduce(UInt8(0)) { $0 ^ $1 }
    let hexStr   = String(s[s.index(after: starIdx)...].prefix(2))
    guard let expected = UInt8(hexStr, radix: 16) else { return false }
    return computed == expected
  }
}
