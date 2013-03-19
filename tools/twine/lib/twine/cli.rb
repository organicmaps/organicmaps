require 'optparse'

module Twine
  class CLI
    def initialize(args, options)
      @options = options
      @args = args
    end

    def self.parse_args(args, options)
      new(args, options).parse_args
    end

    def parse_args
      parser = OptionParser.new do |opts|
        opts.banner = 'Usage: twine COMMAND STRINGS_FILE [INPUT_OR_OUTPUT_PATH] [--lang LANG1,LANG2...] [--tags TAG1,TAG2,TAG3...] [--format FORMAT]'
        opts.separator ''
        opts.separator 'The purpose of this script is to convert back and forth between multiple data formats, allowing us to treat our strings (and translations) as data stored in a text file. We can then use the data file to create drops for the localization team, consume similar drops returned by the localization team, generate reports on the strings, as well as create formatted string files to ship with your products. Twine currently supports iOS, OS X, Android, gettext, and jquery-localize string files.'
        opts.separator ''
        opts.separator 'Commands:'
        opts.separator ''
        opts.separator 'generate-string-file -- Generates a string file in a certain LANGUAGE given a particular FORMAT. This script will attempt to guess both the language and the format given the filename and extension. For example, "ko.xml" will generate a Korean language file for Android.'
        opts.separator ''
        opts.separator 'generate-all-string-files -- Generates all the string files necessary for a given project. The parent directory to all of the locale-specific directories in your project should be specified as the INPUT_OR_OUTPUT_PATH. This command will most often be executed by your build script so that each build always contains the most recent strings.'
        opts.separator ''
        opts.separator 'consume-string-file -- Slurps all of the strings from a translated strings file into the specified STRINGS_FILE. If you have some files returned to you by your translators you can use this command to incorporate all of their changes. This script will attempt to guess both the language and the format given the filename and extension. For example, "ja.strings" will assume that the file is a Japanese iOS strings file.'
        opts.separator ''
        opts.separator 'consume-all-string-files -- Slurps all of the strings from a directory into the specified STRINGS_FILE. If you have some files returned to you by your translators you can use this command to incorporate all of their changes. This script will attempt to guess both the language and the format given the filename and extension. For example, "ja.strings" will assume that the file is a Japanese iOS strings file.'
        opts.separator ''
        opts.separator 'generate-loc-drop -- Generates a zip archive of strings files in any format. The purpose of this command is to create a very simple archive that can be handed off to a translation team. The translation team can unzip the archive, translate all of the strings in the archived files, zip everything back up, and then hand that final archive back to be consumed by the consume-loc-drop command. This command assumes that --include-untranslated has been specified on the command line.'
        opts.separator ''
        opts.separator 'consume-loc-drop -- Consumes an archive of translated files. This archive should be in the same format as the one created by the generate-loc-drop command.'
        opts.separator ''
        opts.separator 'generate-report -- Generates a report containing data about your strings. For example, it will tell you if you have any duplicate strings or if any of your strings are missing tags. In addition, it will tell you how many strings you have and how many of those strings have been translated into each language.'
        opts.separator ''
        opts.separator 'General Options:'
        opts.separator ''
        opts.on('-l', '--lang LANGUAGES', Array, 'The language code(s) to use for the specified action.') do |langs|
          @options[:languages] = langs
        end
        opts.on('-t', '--tags TAGS', Array, 'The tag(s) to use for the specified action. Only strings with that tag will be processed. Do not specify any tags to match all strings in the strings data file.') do |tags|
          @options[:tags] = tags
        end
        opts.on('-u', '--untagged', 'If you have specified tags using the --tags flag, then only those tags will be selected. If you also want to select all strings that are untagged, then you can specify this option to do so.') do |u|
          @options[:untagged] = true
        end
        formats = []
        Formatters::FORMATTERS.each do |formatter|
          formats << formatter::FORMAT_NAME
        end
        opts.on('-f', '--format FORMAT', "The file format to read or write (#{formats.join(', ')}). Additional formatters can be placed in the formats/ directory.") do |format|
          lformat = format.downcase
          if !formats.include?(lformat)
            STDERR.puts "Invalid format: #{format}"
          end
          @options[:format] = lformat
        end
        opts.on('-a', '--consume-all', 'Normally, when consuming a string file, Twine will ignore any string keys that do not exist in your master file.') do |a|
          @options[:consume_all] = true
        end
        opts.on('-s', '--include-untranslated', 'This flag will cause any Android string files that are generated to include strings that have not yet been translated for the current language.') do |s|
          @options[:include_untranslated] = true
        end
        opts.on('-o', '--output-file OUTPUT_FILE', 'Write the new strings database to this file instead of replacing the original file. This flag is only useful when running the consume-string-file or consume-loc-drop commands.') do |o|
          @options[:output_path] = o
        end
        opts.on('-n', '--file-name FILE_NAME', 'When running the generate-all-string-files command, this flag may be used to overwrite the default file name of the format.') do |n|
          @options[:file_name] = n
        end
        opts.on('-d', '--developer-language LANG', 'When writing the strings data file, set the specified language as the "developer language". In practice, this just means that this language will appear first in the strings data file.') do |d|
          @options[:developer_language] = d
        end
        opts.on('-c', '--consume-comments', 'Normally, when consuming a string file, Twine will ignore all comments in the file. With this flag set, any comments encountered will be read and parsed into the strings data file. This is especially useful when creating your first strings data file from an existing project.') do |c|
          @options[:consume_comments] = true
        end
        opts.on('-e', '--encoding ENCODING', 'Twine defaults to encoding all output files in UTF-8. This flag will tell Twine to use an alternate encoding for these files. For example, you could use this to write Apple .strings files in UTF-16. This flag currently only works with Apple .strings files and is currently only supported in Ruby 1.9.3 or greater.') do |e|
          if !"".respond_to?(:encode)
            raise Twine::Error.new "The --encoding flag is only supported on Ruby 1.9.3 or greater."
          end
          @options[:output_encoding] = e
        end
        opts.on('-h', '--help', 'Show this message.') do |h|
          puts opts.help
          exit
        end
        opts.on('--version', 'Print the version number and exit.') do |x|
          puts "Twine version #{Twine::VERSION}"
          exit
        end
        opts.separator ''
        opts.separator 'Examples:'
        opts.separator ''
        opts.separator '> twine generate-string-file strings.txt ko.xml --tags FT'
        opts.separator '> twine generate-all-string-files strings.txt Resources/Locales/ --tags FT,FB'
        opts.separator '> twine consume-string-file strings.txt ja.strings'
        opts.separator '> twine consume-all-string-files strings.txt Resources/Locales/ --developer-language en'
        opts.separator '> twine generate-loc-drop strings.txt LocDrop5.zip --tags FT,FB --format android --lang de,en,en-GB,ja,ko'
        opts.separator '> twine consume-loc-drop strings.txt LocDrop5.zip'
        opts.separator '> twine generate-report strings.txt'
      end
      parser.parse! @args

      if @args.length == 0
        puts parser.help
        exit
      end

      @options[:command] = @args[0]

      if !VALID_COMMANDS.include? @options[:command]
        raise Twine::Error.new "Invalid command: #{@options[:command]}"
      end

      if @args.length == 1
        raise Twine::Error.new 'You must specify your strings file.'
      end

      @options[:strings_file] = @args[1]

      case @options[:command]
      when 'generate-string-file'
        if @args.length == 3
          @options[:output_path] = @args[2]
        elsif @args.length > 3
          raise Twine::Error.new "Unknown argument: #{@args[3]}"
        else
          raise Twine::Error.new 'Not enough arguments.'
        end
        if @options[:languages] and @options[:languages].length > 1
          raise Twine::Error.new 'Please only specify a single language for the generate-string-file command.'
        end
      when 'generate-all-string-files'
        if ARGV.length == 3
          @options[:output_path] = @args[2]
        elsif @args.length > 3
          raise Twine::Error.new "Unknown argument: #{@args[3]}"
        else
          raise Twine::Error.new 'Not enough arguments.'
        end
      when 'consume-string-file'
        if @args.length == 3
          @options[:input_path] = @args[2]
        elsif @args.length > 3
          raise Twine::Error.new "Unknown argument: #{@args[3]}"
        else
          raise Twine::Error.new 'Not enough arguments.'
        end
        if @options[:languages] and @options[:languages].length > 1
          raise Twine::Error.new 'Please only specify a single language for the consume-string-file command.'
        end
      when 'consume-all-string-files'
        if @args.length == 3
          @options[:input_path] = @args[2]
        elsif @args.length > 3
          raise Twine::Error.new "Unknown argument: #{@args[3]}"
        else
          raise Twine::Error.new 'Not enough arguments.'
        end
      when 'generate-loc-drop'
        @options[:include_untranslated] = true
        if @args.length == 3
          @options[:output_path] = @args[2]
        elsif @args.length > 3
          raise Twine::Error.new "Unknown argument: #{@args[3]}"
        else
          raise Twine::Error.new 'Not enough arguments.'
        end
        if !@options[:format]
          raise Twine::Error.new 'You must specify a format.'
        end
      when 'consume-loc-drop'
        if @args.length == 3
          @options[:input_path] = @args[2]
        elsif @args.length > 3
          raise Twine::Error.new "Unknown argument: #{@args[3]}"
        else
          raise Twine::Error.new 'Not enough arguments.'
        end
      when 'generate-report'
        if @args.length > 2
          raise Twine::Error.new "Unknown argument: #{@args[2]}"
        end
      end
    end
  end
end
