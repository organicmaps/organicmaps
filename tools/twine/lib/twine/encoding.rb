module Twine
  module Encoding

    def self.bom(path)
      first_bytes = IO.binread(path, 2)
      return nil unless first_bytes
      first_bytes = first_bytes.codepoints.map.to_a
      return 'UTF-16BE' if first_bytes == [0xFE, 0xFF]
      return 'UTF-16LE' if first_bytes == [0xFF, 0xFE]
    rescue EOFError
      return nil
    end

    def self.has_bom?(path)
      !bom(path).nil?
    end

    def self.encoding_for_path(path)
      bom(path) || 'UTF-8'      
    end
  end
end
