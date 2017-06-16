require 'fileutils'

module Twine
  module Formatters
    class Abstract
      attr_accessor :twine_file
      attr_accessor :options

      def initialize
        @twine_file = TwineFile.new
        @options = {}
      end

      def format_name
        raise NotImplementedError.new("You must implement format_name in your formatter class.")
      end

      def extension
        raise NotImplementedError.new("You must implement extension in your formatter class.")
      end

      def can_handle_directory?(path)
        Dir.entries(path).any? { |item| /^.+#{Regexp.escape(extension)}$/.match(item) }
      end

      def default_file_name
        raise NotImplementedError.new("You must implement default_file_name in your formatter class.")
      end

      def set_translation_for_key(key, lang, value)
        value = value.gsub("\n", "\\n")

        if @twine_file.definitions_by_key.include?(key)
          definition = @twine_file.definitions_by_key[key]
          reference = @twine_file.definitions_by_key[definition.reference_key] if definition.reference_key

          if !reference or value != reference.translations[lang]
            definition.translations[lang] = value
          end
        elsif @options[:consume_all]
          Twine::stderr.puts "Adding new definition '#{key}' to twine file."
          current_section = @twine_file.sections.find { |s| s.name == 'Uncategorized' }
          unless current_section
            current_section = TwineSection.new('Uncategorized')
            @twine_file.sections.insert(0, current_section)
          end
          current_definition = TwineDefinition.new(key)
          current_section.definitions << current_definition
          
          if @options[:tags] && @options[:tags].length > 0
            current_definition.tags = @options[:tags]            
          end
          
          @twine_file.definitions_by_key[key] = current_definition
          @twine_file.definitions_by_key[key].translations[lang] = value
        else
          Twine::stderr.puts "Warning: '#{key}' not found in twine file."
        end
        if !@twine_file.language_codes.include?(lang)
          @twine_file.add_language_code(lang)
        end
      end

      def set_comment_for_key(key, comment)
        return unless @options[:consume_comments]
        
        if @twine_file.definitions_by_key.include?(key)
          definition = @twine_file.definitions_by_key[key]
          
          reference = @twine_file.definitions_by_key[definition.reference_key] if definition.reference_key

          if !reference or comment != reference.raw_comment
            definition.comment = comment
          end
        end
      end

      def determine_language_given_path(path)
        raise NotImplementedError.new("You must implement determine_language_given_path in your formatter class.")
      end

      def output_path_for_language(lang)
        lang
      end

      def read(io, lang)
        raise NotImplementedError.new("You must implement read in your formatter class.")
      end

      def format_file(lang)
        output_processor = Processors::OutputProcessor.new(@twine_file, @options)
        processed_twine_file = output_processor.process(lang)

        return nil if processed_twine_file.definitions_by_key.empty?

        header = format_header(lang)
        result = ""
        result += header + "\n" if header
        result += format_sections(processed_twine_file, lang)
      end

      def format_header(lang)
      end

      def format_sections(twine_file, lang)
        sections = twine_file.sections.map { |section| format_section(section, lang) }
        sections.compact.join("\n")
      end

      def format_section_header(section)
      end

      def should_include_definition(definition, lang)
        return !definition.translation_for_lang(lang).nil?
      end

      def format_section(section, lang)
        definitions = section.definitions.select { |definition| should_include_definition(definition, lang) }
        return if definitions.empty?

        result = ""

        if section.name && section.name.length > 0
          section_header = format_section_header(section)
          result += "\n#{section_header}" if section_header
        end

        definitions.map! { |definition| format_definition(definition, lang) }
        definitions.compact! # remove nil definitions
        definitions.map! { |definition| "\n#{definition}" }  # prepend newline
        result += definitions.join
      end

      def format_definition(definition, lang)
        [format_comment(definition, lang), format_key_value(definition, lang)].compact.join
      end

      def format_comment(definition, lang)
      end

      def format_key_value(definition, lang)
        value = definition.translation_for_lang(lang)
        key_value_pattern % { key: format_key(definition.key.dup), value: format_value(value.dup) }
      end

      def key_value_pattern
        raise NotImplementedError.new("You must implement key_value_pattern in your formatter class.")
      end

      def format_key(key)
        key
      end

      def format_value(value)
        value
      end

      def escape_quotes(text)
        text.gsub('"', '\\\\"')
      end
    end
  end
end
