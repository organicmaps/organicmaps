require 'tmpdir'
require 'fileutils'

Twine::Plugin.new # Initialize plugins first in Runner.

module Twine
  class Runner
    def self.run(args)
      options = CLI.parse(args)
      
      twine_file = TwineFile.new
      twine_file.read options[:twine_file]
      runner = new(options, twine_file)

      case options[:command]
      when 'generate-localization-file'
        runner.generate_localization_file
      when 'generate-all-localization-files'
        runner.generate_all_localization_files
      when 'consume-localization-file'
        runner.consume_localization_file
      when 'consume-all-localization-files'
        runner.consume_all_localization_files
      when 'generate-localization-archive'
        runner.generate_localization_archive
      when 'consume-localization-archive'
        runner.consume_localization_archive
      when 'validate-twine-file'
        runner.validate_twine_file
      end
    end

    def initialize(options = {}, twine_file = TwineFile.new)
      @options = options
      @twine_file = twine_file
    end

    def write_twine_data(path)
      if @options[:developer_language]
        @twine_file.set_developer_language_code(@options[:developer_language])
      end
      @twine_file.write(path)
    end

    def generate_localization_file
      validate_twine_file if @options[:validate]

      lang = nil
      lang = @options[:languages][0] if @options[:languages]

      formatter, lang = prepare_read_write(@options[:output_path], lang)
      output = formatter.format_file(lang)

      raise Twine::Error.new "Nothing to generate! The resulting file would not contain any translations." unless output

      IO.write(@options[:output_path], output, encoding: output_encoding)
    end

    def generate_all_localization_files
      validate_twine_file if @options[:validate]

      if !File.directory?(@options[:output_path])
        if @options[:create_folders]
          FileUtils.mkdir_p(@options[:output_path])
        else
          raise Twine::Error.new("Directory does not exist: #{@options[:output_path]}")
        end
      end

      formatter_for_directory = find_formatter { |f| f.can_handle_directory?(@options[:output_path]) }
      formatter = formatter_for_format(@options[:format]) || formatter_for_directory
      
      unless formatter
        raise Twine::Error.new "Could not determine format given the contents of #{@options[:output_path]}"
      end

      file_name = @options[:file_name] || formatter.default_file_name
      if @options[:create_folders]
        @twine_file.language_codes.each do |lang|
          output_path = File.join(@options[:output_path], formatter.output_path_for_language(lang))

          FileUtils.mkdir_p(output_path)

          file_path = File.join(output_path, file_name)

          output = formatter.format_file(lang)
          unless output
            Twine::stderr.puts "Skipping file at path #{file_path} since it would not contain any translations."
            next
          end

          IO.write(file_path, output, encoding: output_encoding)
        end
      else
        language_found = false
        Dir.foreach(@options[:output_path]) do |item|
          next if item == "." or item == ".."

          output_path = File.join(@options[:output_path], item)
          next unless File.directory?(output_path)

          lang = formatter.determine_language_given_path(output_path)
          next unless lang

          language_found = true

          file_path = File.join(output_path, file_name)
          output = formatter.format_file(lang)
          unless output
            Twine::stderr.puts "Skipping file at path #{file_path} since it would not contain any translations."
            next
          end

          IO.write(file_path, output, encoding: output_encoding)
        end

        unless language_found
          raise Twine::Error.new("Failed to generate any files: No languages found at #{@options[:output_path]}")
        end
      end

    end

    def generate_localization_archive
      validate_twine_file if @options[:validate]
      
      require_rubyzip

      if File.file?(@options[:output_path])
        File.delete(@options[:output_path])
      end

      Dir.mktmpdir do |temp_dir|
        Zip::File.open(@options[:output_path], Zip::File::CREATE) do |zipfile|
          zipfile.mkdir('Locales')

          formatter = formatter_for_format(@options[:format])
          @twine_file.language_codes.each do |lang|
            if @options[:languages] == nil || @options[:languages].length == 0 || @options[:languages].include?(lang)
              file_name = lang + formatter.extension
              temp_path = File.join(temp_dir, file_name)
              zip_path = File.join('Locales', file_name)

              output = formatter.format_file(lang)
              unless output
                Twine::stderr.puts "Skipping file #{file_name} since it would not contain any translations."
                next
              end
              
              IO.write(temp_path, output, encoding: output_encoding)
              zipfile.add(zip_path, temp_path)
            end
          end
        end
      end
    end

    def consume_localization_file
      lang = nil
      if @options[:languages]
        lang = @options[:languages][0]
      end

      read_localization_file(@options[:input_path], lang)
      output_path = @options[:output_path] || @options[:twine_file]
      write_twine_data(output_path)
    end

    def consume_all_localization_files
      if !File.directory?(@options[:input_path])
        raise Twine::Error.new("Directory does not exist: #{@options[:input_path]}")
      end

      Dir.glob(File.join(@options[:input_path], "**/*")) do |item|
        if File.file?(item)
          begin
            read_localization_file(item)
          rescue Twine::Error => e
            Twine::stderr.puts "#{e.message}"
          end
        end
      end

      output_path = @options[:output_path] || @options[:twine_file]
      write_twine_data(output_path)
    end

    def consume_localization_archive
      require_rubyzip

      if !File.file?(@options[:input_path])
        raise Twine::Error.new("File does not exist: #{@options[:input_path]}")
      end

      Dir.mktmpdir do |temp_dir|
        Zip::File.open(@options[:input_path]) do |zipfile|
          zipfile.each do |entry|
            next if entry.name.end_with? '/' or File.basename(entry.name).start_with? '.'

            real_path = File.join(temp_dir, entry.name)
            FileUtils.mkdir_p(File.dirname(real_path))
            zipfile.extract(entry.name, real_path)
            begin
              read_localization_file(real_path)
            rescue Twine::Error => e
              Twine::stderr.puts "#{e.message}"
            end
          end
        end
      end

      output_path = @options[:output_path] || @options[:twine_file]
      write_twine_data(output_path)
    end

    def validate_twine_file
      total_definitions = 0
      all_keys = Set.new
      duplicate_keys = Set.new
      keys_without_tags = Set.new
      invalid_keys = Set.new
      valid_key_regex = /^[A-Za-z0-9_]+$/

      @twine_file.sections.each do |section|
        section.definitions.each do |definition|
          total_definitions += 1

          duplicate_keys.add(definition.key) if all_keys.include? definition.key
          all_keys.add(definition.key)

          keys_without_tags.add(definition.key) if definition.tags == nil or definition.tags.length == 0

          invalid_keys << definition.key unless definition.key =~ valid_key_regex
        end
      end

      errors = []
      join_keys = lambda { |set| set.map { |k| "  " + k }.join("\n") }

      unless duplicate_keys.empty?
        errors << "Found duplicate key(s):\n#{join_keys.call(duplicate_keys)}"
      end

      if @options[:pedantic]
        if keys_without_tags.length == total_definitions
          errors << "None of your definitions have tags."
        elsif keys_without_tags.length > 0
          errors << "Found definitions without tags:\n#{join_keys.call(keys_without_tags)}"
        end
      end

      unless invalid_keys.empty?
        errors << "Found key(s) with invalid characters:\n#{join_keys.call(invalid_keys)}"
      end

      raise Twine::Error.new errors.join("\n\n") unless errors.empty?

      Twine::stdout.puts "#{@options[:twine_file]} is valid."
    end

    private

    def output_encoding
      @options[:encoding] || 'UTF-8'
    end

    def require_rubyzip
      begin
        require 'zip'
      rescue LoadError
        raise Twine::Error.new "You must run 'gem install rubyzip' in order to create or consume localization archives."
      end
    end

    def determine_language_given_path(path)
      code = File.basename(path, File.extname(path))
      return code if @twine_file.language_codes.include? code
    end

    def formatter_for_format(format)
      find_formatter { |f| f.format_name == format }
    end

    def find_formatter(&block)
      formatter = Formatters.formatters.find &block
      return nil unless formatter
      formatter.twine_file = @twine_file
      formatter.options = @options
      formatter
    end

    def read_localization_file(path, lang = nil)
      unless File.file?(path)
        raise Twine::Error.new("File does not exist: #{path}")
      end

      formatter, lang = prepare_read_write(path, lang)

      external_encoding = @options[:encoding] || Twine::Encoding.encoding_for_path(path)

      IO.open(IO.sysopen(path, 'rb'), 'rb', external_encoding: external_encoding, internal_encoding: 'UTF-8') do |io|
        io.read(2) if Twine::Encoding.has_bom?(path)
        formatter.read(io, lang)
      end
    end

    def prepare_read_write(path, lang)
      formatter_for_path = find_formatter { |f| f.extension == File.extname(path) }
      formatter = formatter_for_format(@options[:format]) || formatter_for_path
      
      unless formatter
        raise Twine::Error.new "Unable to determine format of #{path}"
      end      

      lang = lang || determine_language_given_path(path) || formatter.determine_language_given_path(path)
      unless lang
        raise Twine::Error.new "Unable to determine language for #{path}"
      end

      @twine_file.language_codes << lang unless @twine_file.language_codes.include? lang

      return formatter, lang
    end
  end
end
