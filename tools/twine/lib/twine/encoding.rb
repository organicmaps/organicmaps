module Twine
  module Encoding
    def self.encoding_for_path path
      File.open(path, 'rb') do |f|
        begin
          a = f.readbyte
          b = f.readbyte
          if (a == 0xfe && b == 0xff)
            return 'UTF-16BE'
          elsif (a == 0xff && b == 0xfe)
            return 'UTF-16LE'
          end
        rescue EOFError
        end
      end

      'UTF-8'
    end
  end
end
