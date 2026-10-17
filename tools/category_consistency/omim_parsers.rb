module OmimParsers

  LANGUAGE_CODE = /[a-z]{2,3}(?:-[A-Za-z0-9]+)*/

  class AbstractParser
    def initialize(keys)
      @keys = keys
    end

    def parse_line(line)
      raise NotImplementedError, 'You must implement parse_line.'
    end

    def match_category(line, result)
      category_match = category.match(line)
      if !category_match.nil?
        category_name = category_match[1]
        if @keys.include? category_name
          result[category_name] ||= {}
        end
      end
    end

    def parse_file(filename)
      current_string = nil
      result = {}
      File.foreach(filename, encoding: 'UTF-8') do |line|
        line.strip!
        next if line.start_with? '#'

        # If line is empty -> next category block started
        if line.empty?
          current_string = nil
          next
        end

        current_string ||= match_category(line, result)

        parsed = parse_line(line)
        if !parsed.nil? and !current_string.nil?
          lang, translation = parsed
          current_string[lang] = translation
        end
      end
      result
    end

    def category
      raise NotImplementedError.new("You must implement category.")
    end

  end

  class CategoriesParser < AbstractParser
    def parse_line(line)
      match = /\A(#{LANGUAGE_CODE}):(.+)\z/.match(line)
      return unless match

      [match[1], match[2].split('|').map { |token| token.sub(/\A[1-9]/, '') }]
    end

    def category
      # Only group definitions can supply the displayed category translations.
      /\A@([A-Za-z0-9_]+)\z/
    end
  end

  class StringsParser < AbstractParser
    def parse_line(line)
      match = /\A(#{LANGUAGE_CODE})\s*=(.*)\z/.match(line)
      [match[1], match[2].strip] if match
    end

    def category
      /\A\[(.+)\]\z/
    end
  end
end
