class DBUtils {
  static func encodeToJsonString<T: Encodable>(_ body: T) -> String? {
    do {
      let encoder = JSONEncoder()
      encoder.outputFormatting = .withoutEscapingSlashes
      let encoded = try encoder.encode(body)
      return convertDataToString(encoded)
    } catch {
      return nil
    }
  }
  
  private static func convertDataToString(_ data: Data) -> String? {
    return String(data: data, encoding: .utf8)
  }
  
  static func decodeFromJsonString<T: Decodable>(_ jsonString: String, to type: T.Type) -> T? {
    guard let data = jsonString.data(using: .utf8) else {
      return nil
    }
    do {
      let decoder = JSONDecoder()
      return try decoder.decode(T.self, from: data)
    } catch {
      return nil
    }
  }
}
