module Twine
  class StringsSection
    attr_reader :name
    attr_reader :rows

    def initialize(name)
      @name = name
      @rows = []
    end
  end

  class StringsRow
    attr_reader :key
    attr_accessor :comment
    attr_accessor :tags
    attr_reader :translations

    def initialize(key)
      @key = key
      @comment = nil
      @tags = nil
      @translations = {}
    end

    def matches_tags?(tags, include_untagged)
      if tags == nil || tags.length == 0
        # The user did not specify any tags. Everything passes.
        return true
      elsif @tags == nil || @tags.length == 0
        # This row has no tags.
        return (include_untagged) ? true : false
      else
        tags.each do |tag|
          if @tags.include? tag
            return true
          end
        end
      end

      return false
    end

    def translated_string_for_lang(lang, default_lang=nil)
      if @translations[lang]
        return @translations[lang]
      elsif default_lang.respond_to?("each")
        default_lang.each do |def_lang|
          if @translations[def_lang]
            return @translations[def_lang]
          end
        end
        return nil
      else
        return @translations[default_lang]
      end
    end
  end

  class StringsFile
    attr_reader :sections
    attr_reader :strings_map
    attr_reader :language_codes

    def initialize
      @sections = []
      @strings_map = {}
      @language_codes = []
    end

    def read(path)
      if !File.file?(path)
        raise Twine::Error.new("File does not exist: #{path}")
      end

      File.open(path, 'r:UTF-8') do |f|
        line_num = 0
        current_section = nil
        current_row = nil
        while line = f.gets
          parsed = false
          line.strip!
          line_num += 1

          if line.length == 0
            next
          end

          if line.length > 4 && line[0, 2] == '[['
            match = /^\[\[(.+)\]\]$/.match(line)
            if match
              current_section = StringsSection.new(match[1])
              @sections << current_section
              parsed = true
            end
          elsif line.length > 2 && line[0, 1] == '['
            match = /^\[(.+)\]$/.match(line)
            if match
              current_row = StringsRow.new(match[1])
              @strings_map[current_row.key] = current_row
              if !current_section
                current_section = StringsSection.new('')
                @sections << current_section
              end
              current_section.rows << current_row
              parsed = true
            end
          else
            match = /^([^=]+)=(.*)$/.match(line)
            if match
              key = match[1].strip
              value = match[2].strip
              if value[0,1] == '`' && value[-1,1] == '`'
                value = value[1..-2]
              end

              case key
              when "comment"
                current_row.comment = value
              when 'tags'
                current_row.tags = value.split(',')
              else
                if !@language_codes.include? key
                  add_language_code(key)
                end
                current_row.translations[key] = value
              end
              parsed = true
            end
          end

          if !parsed
            raise Twine::Error.new("Unable to parse line #{line_num} of #{path}: #{line}")
          end
        end
      end
    end

    def write(path)
      dev_lang = @language_codes[0]

      File.open(path, 'w:UTF-8') do |f|
        @sections.each do |section|
          if f.pos > 0
            f.puts ''
          end

          f.puts "[[#{section.name}]]"

          section.rows.each do |row|
            f.puts "\t[#{row.key}]"
            value = row.translations[dev_lang]
            if !value
              puts "Warning: #{row.key} does not exist in developer language '#{dev_lang}'"
            else
              if value[0,1] == ' ' || value[-1,1] == ' ' || (value[0,1] == '`' && value[-1,1] == '`')
                value = '`' + value + '`'
              end
              f.puts "\t\t#{dev_lang} = #{value}"
            end

            if row.tags && row.tags.length > 0
              tag_str = row.tags.join(',')
              f.puts "\t\ttags = #{tag_str}"
            end
            if row.comment && row.comment.length > 0
              f.puts "\t\tcomment = #{row.comment}"
            end
            @language_codes[1..-1].each do |lang|
              value = row.translations[lang]
              if value
                if value[0,1] == ' ' || value[-1,1] == ' ' || (value[0,1] == '`' && value[-1,1] == '`')
                  value = '`' + value + '`'
                end
                f.puts "\t\t#{lang} = #{value}"
              end
            end
          end
        end
      end
    end

    def add_language_code(code)
      if @language_codes.length == 0
        @language_codes << code
      elsif !@language_codes.include?(code)
        dev_lang = @language_codes[0]
        @language_codes << code
        @language_codes.delete(dev_lang)
        @language_codes.sort!
        @language_codes.insert(0, dev_lang)
      end
    end

    def set_developer_language_code(code)
      if @language_codes.include?(code)
        @language_codes.delete(code)
      end
      @language_codes.insert(0, code)
    end
  end
end
