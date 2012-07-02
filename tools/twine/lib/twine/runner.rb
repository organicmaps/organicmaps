require 'tmpdir'

module Twine
  VALID_COMMANDS = ['generate-string-file', 'generate-all-string-files', 'consume-string-file', 'consume-all-string-files', 'generate-loc-drop', 'consume-loc-drop', 'generate-report']

  class Runner
    def initialize(args)
      @options = {}
      @args = args
    end

    def self.run(args)
      new(args).run
    end

    def run
      # Parse all CLI arguments.
      CLI::parse_args(@args, @options)
      read_strings_data
      execute_command
    end

    def read_strings_data
      @strings = StringsFile.new
      @strings.read @options[:strings_file]
    end

    def write_strings_data(path)
      if @options[:developer_language]
        @strings.set_developer_language_code(@options[:developer_language])
      end
      @strings.write(path)
    end

    def execute_command
      case @options[:command]
      when 'generate-string-file'
        generate_string_file
      when 'generate-all-string-files'
        generate_all_string_files
      when 'consume-string-file'
        consume_string_file
      when 'consume-all-string-files'
        consume_all_string_files
      when 'generate-loc-drop'
        generate_loc_drop
      when 'consume-loc-drop'
        consume_loc_drop
      when 'generate-report'
        generate_report
      end
    end

    def generate_string_file
      lang = nil
      if @options[:languages]
        lang = @options[:languages][0]
      end

      read_write_string_file(@options[:output_path], false, lang)
    end

    def generate_all_string_files
      if !File.directory?(@options[:output_path])
        raise Twine::Error.new("Directory does not exist: #{@options[:output_path]}")
      end

      format = @options[:format]
      if !format
        format = determine_format_given_directory(@options[:output_path])
      end
      if !format
        raise Twine::Error.new "Could not determine format given the contents of #{@options[:output_path]}"
      end

      formatter = formatter_for_format(format)

      formatter.write_all_files(@options[:output_path])
    end

    def consume_string_file
      lang = nil
      if @options[:languages]
        lang = @options[:languages][0]
      end

      read_write_string_file(@options[:input_path], true, lang)
      output_path = @options[:output_path] || @options[:strings_file]
      write_strings_data(output_path)
    end

    def consume_all_string_files
      if !File.directory?(@options[:input_path])
        raise Twine::Error.new("Directory does not exist: #{@options[:output_path]}")
      end

      Dir.glob(File.join(@options[:input_path], "**/*")) do |item|
        if File.file?(item)
          begin
            read_write_string_file(item, true, nil)
          rescue Twine::Error => e
            STDERR.puts "#{e.message}"
          end
        end
      end

      output_path = @options[:output_path] || @options[:strings_file]
      write_strings_data(output_path)
    end

    def read_write_string_file(path, is_read, lang)
      if is_read && !File.file?(path)
        raise Twine::Error.new("File does not exist: #{path}")
      end

      format = @options[:format]
      if !format
        format = determine_format_given_path(path)
      end
      if !format
        raise Twine::Error.new "Unable to determine format of #{path}"
      end

      formatter = formatter_for_format(format)

      if !lang
        lang = determine_language_given_path(path)
      end
      if !lang
        lang = formatter.determine_language_given_path(path)
      end
      if !lang
        raise Twine::Error.new "Unable to determine language for #{path}"
      end

      if !@strings.language_codes.include? lang
        @strings.language_codes << lang
      end

      if is_read
        formatter.read_file(path, lang)
      else
        formatter.write_file(path, lang)
      end
    end

    def generate_loc_drop
      begin
        require 'zip/zip'
      rescue LoadError
        raise Twine::Error.new "You must run 'gem install rubyzip' in order to create or consume localization drops."
      end

      if File.file?(@options[:output_path])
        File.delete(@options[:output_path])
      end

      Dir.mktmpdir do |dir|
        Zip::ZipFile.open(@options[:output_path], Zip::ZipFile::CREATE) do |zipfile|
          zipfile.mkdir('Locales')

          formatter = formatter_for_format(@options[:format])
          @strings.language_codes.each do |lang|
            if @options[:languages] == nil || @options[:languages].length == 0 || @options[:languages].include?(lang)
              file_name = lang + formatter.class::EXTENSION
              real_path = File.join(dir, file_name)
              zip_path = File.join('Locales', file_name)
              formatter.write_file(real_path, lang)
              zipfile.add(zip_path, real_path)
            end
          end
        end
      end
    end

    def consume_loc_drop
      if !File.file?(@options[:input_path])
        raise Twine::Error.new("File does not exist: #{@options[:input_path]}")
      end

      begin
        require 'zip/zip'
      rescue LoadError
        raise Twine::Error.new "You must run 'gem install rubyzip' in order to create or consume localization drops."
      end

      Dir.mktmpdir do |dir|
        Zip::ZipFile.open(@options[:input_path]) do |zipfile|
          zipfile.each do |entry|
            if !entry.name.end_with?'/' and !File.basename(entry.name).start_with?'.'
              real_path = File.join(dir, entry.name)
              FileUtils.mkdir_p(File.dirname(real_path))
              zipfile.extract(entry.name, real_path)
              begin
                read_write_string_file(real_path, true, nil)
              rescue Twine::Error => e
                STDERR.puts "#{e.message}"
              end
            end
          end
        end
      end

      output_path = @options[:output_path] || @options[:strings_file]
      write_strings_data(output_path)
    end

    def generate_report
      total_strings = 0
      strings_per_lang = {}
      all_keys = Set.new
      duplicate_keys = Set.new
      keys_without_tags = Set.new
      @strings.language_codes.each do |code|
        strings_per_lang[code] = 0
      end

      @strings.sections.each do |section|
        section.rows.each do |row|
          total_strings += 1

          if all_keys.include? row.key
            duplicate_keys.add(row.key)
          else
            all_keys.add(row.key)
          end

          row.translations.each_key do |code|
            strings_per_lang[code] += 1
          end

          if row.tags == nil || row.tags.length == 0
            keys_without_tags.add(row.key)
          end
        end
      end

      # Print the report.
      puts "Total number of strings = #{total_strings}"
      @strings.language_codes.each do |code|
        puts "#{code}: #{strings_per_lang[code]}"
      end

      if duplicate_keys.length > 0
        puts "\nDuplicate string keys:"
        duplicate_keys.each do |key|
          puts key
        end
      end

      if keys_without_tags.length == total_strings
        puts "\nNone of your strings have tags."
      elsif keys_without_tags.length > 0
        puts "\nStrings without tags:"
        keys_without_tags.each do |key|
          puts key
        end
      end
    end

    def determine_language_given_path(path)
      code = File.basename(path, File.extname(path))
      if !@strings.language_codes.include? code
        code = nil
      end

      code
    end

    def determine_format_given_path(path)
      ext = File.extname(path)
      Formatters::FORMATTERS.each do |formatter|
        if formatter::EXTENSION == ext
          return formatter::FORMAT_NAME
        end
      end

      return
    end

    def determine_format_given_directory(directory)
      Formatters::FORMATTERS.each do |formatter|
        if formatter.can_handle_directory?(directory)
          return formatter::FORMAT_NAME
        end
      end

      return
    end

    def formatter_for_format(format)
      Formatters::FORMATTERS.each do |formatter|
        if formatter::FORMAT_NAME == format
          return formatter.new(@strings, @options)
        end
      end

      return
    end
  end
end
