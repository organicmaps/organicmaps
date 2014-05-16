module Twine
  module Formatters
    class Abstract
      attr_accessor :strings
      attr_accessor :options

      def self.can_handle_directory?(path)
        return false
      end

      def initialize(strings, options)
        @strings = strings
        @options = options
      end

      def iosify_substitutions(str)
        # use "@" instead of "s" for substituting strings
        str.gsub!(/%([0-9\$]*)s/, '%\1@')
        return str
      end

      def androidify_substitutions(str)
        # 1) use "s" instead of "@" for substituting strings
        str.gsub!(/%([0-9\$]*)@/, '%\1s')

        # 1a) escape strings that begin with a lone "@"
        str.sub!(/^@ /, '\\@ ')

        # 2) if there is more than one substitution in a string, make sure they are numbered
        substituteCount = 0
        startFound = false
        str.each_char do |c|
          if startFound
            if c == "%"
              # ignore as this is a literal %
            elsif c.match(/\d/)
              # leave the string alone if it already has numbered substitutions
              return str
            else
              substituteCount += 1
            end
            startFound = false
          elsif c == "%"
            startFound = true
          end
        end

        if substituteCount > 1
          currentSub = 1
          startFound = false
          newstr = ""
          str.each_char do |c|
            if startFound
              if !(c == "%")
                newstr = newstr + "#{currentSub}$"
                currentSub += 1
              end
              startFound = false
            elsif c == "%"
              startFound = true
            end
            newstr = newstr + c
          end
          return newstr
        else
          return str
        end
      end

      def set_translation_for_key(key, lang, value)
        if @strings.strings_map.include?(key)
          @strings.strings_map[key].translations[lang] = value
        elsif @options[:consume_all]
          STDERR.puts "Adding new string '#{key}' to strings data file."
          arr = @strings.sections.select { |s| s.name == 'Uncategorized' }
          current_section = arr ? arr[0] : nil
          if !current_section
            current_section = StringsSection.new('Uncategorized')
            @strings.sections.insert(0, current_section)
          end
          current_row = StringsRow.new(key)
          current_section.rows << current_row
          @strings.strings_map[key] = current_row
          @strings.strings_map[key].translations[lang] = value
        else
          STDERR.puts "Warning: '#{key}' not found in strings data file."
        end
        if !@strings.language_codes.include?(lang)
          @strings.add_language_code(lang)
        end
      end

      def set_comment_for_key(key, comment)
        if @strings.strings_map.include?(key)
          @strings.strings_map[key].comment = comment
        end
      end

      def default_file_name
        raise NotImplementedError.new("You must implement default_file_name in your formatter class.")
      end

      def determine_language_given_path(path)
        raise NotImplementedError.new("You must implement determine_language_given_path in your formatter class.")
      end

      def read_file(path, lang)
        raise NotImplementedError.new("You must implement read_file in your formatter class.")
      end

      def write_file(path, lang)
        raise NotImplementedError.new("You must implement write_file in your formatter class.")
      end

      def write_all_files(path)
        if !File.directory?(path)
          raise Twine::Error.new("Directory does not exist: #{path}")
        end

        file_name = @options[:file_name] || default_file_name
        langs_written = []
        Dir.foreach(path) do |item|
          if item == "." or item == ".."
            next
          end
          item = File.join(path, item)
          if File.directory?(item)
            lang = determine_language_given_path(item)
            if lang
              write_file(File.join(item, file_name), lang)
              langs_written << lang
            end
          end
        end
        if langs_written.empty?
          raise Twine::Error.new("Failed to genertate any files: No languages found at #{path}")
        end
      end
    end
  end
end
