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

        Dir.foreach(path) do |item|
          lang = determine_language_given_path(item)
          if lang
            write_file(File.join(path, item, default_file_name), lang)
          end
        end
      end
    end
  end
end
