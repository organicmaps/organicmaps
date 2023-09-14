module OmimParsers

  # To update the list, run in root directory:
  # sed -nEe "s/ +([a-zA-Z]{2}(-[a-zA-Z]{2,})?) = .*$/\1/p" "data/strings/strings.txt" | sort -u | tr '\n' ' ' | sed -e 's/,$//' | fold -s -w48; echo
  LANGUAGES = %w(af ar be bg ca cs da de el en en-GB es es-MX et
                 eu fa fi fr fr-CA he hi hu id it ja ko lt mr nb
                 nl pl pt pt-BR ro ru sk sv sw th tr uk vi
                 zh-Hans zh-Hant)

  class AbstractParser
    def initialize(keys)
      @keys = keys
    end

    def parse_line(line)
      raise NotImplementedError.new("You must implement parse_file.")
    end

    def match_category(line, result)
      category_match = category.match(line)
      if !category_match.nil?
        category = category_match[1]
        if @keys.include? category
          result[category] ||= {}
        end
      end
    end

    def parse_file(filename)
      current_string = nil
      result = {}
      File.open(filename, 'r:UTF-8').each do |line|
        line.strip!
        next if should_exclude_line? line

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

    def should_exclude_line?(line)
      false
    end
  end

  class CategoriesParser < AbstractParser
    def parse_line(line)
      line_match = /^([^:]+):(\S+)$/u.match(line)
      return if !line_match

      lang = $1.strip
      return if !LANGUAGES.include? lang

      translation = $2.strip
      synonyms = []
      translation.split('|').each do |token|
        token_match  = /\d?\^?(.*)$/.match(token)
        synonyms.push(token_match[1]) if token_match
      end
      [lang, synonyms]
    end

    def should_exclude_line?(line)
      line.start_with? '#'
    end

    def category
      # We match only global categories ('food', 'bank'...)
      /^@([A-Za-z0-9]+)$/
    end
  end

  class StringsParser < AbstractParser
    def parse_line(line)
      line_match = /^([^=]+)=(.*)$/.match(line)
      if line_match
        lang = $1.strip
        if LANGUAGES.include? lang
          [lang, $2.strip]
        end
      end
    end

    def category
      /^\[(.+)\]/
    end
  end
end
