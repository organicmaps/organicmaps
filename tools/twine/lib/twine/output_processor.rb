module Twine
  module Processors

    class OutputProcessor
      def initialize(twine_file, options)
        @twine_file = twine_file
        @options = options
      end

      def default_language
        @options[:developer_language] || @twine_file.language_codes[0]
      end

      def fallback_languages(language)
        fallback_mapping = {
          'zh-TW' => 'zh-Hant' # if we don't have a zh-TW translation, try zh-Hant before en
        }

        [fallback_mapping[language], default_language].flatten.compact
      end

      def process(language)
        result = TwineFile.new

        result.language_codes.concat @twine_file.language_codes
        @twine_file.sections.each do |section|
          new_section = TwineSection.new section.name

          section.definitions.each do |definition|
            next unless definition.matches_tags?(@options[:tags], @options[:untagged])

            value = definition.translation_for_lang(language)

            next if value && @options[:include] == :untranslated

            if value.nil? && @options[:include] != :translated
              value = definition.translation_for_lang(fallback_languages(language))
            end

            next unless value

            new_definition = definition.dup
            new_definition.translations[language] = value

            new_section.definitions << new_definition
            result.definitions_by_key[new_definition.key] = new_definition
          end

          result.sections << new_section
        end

        return result
      end
    end

  end
end
