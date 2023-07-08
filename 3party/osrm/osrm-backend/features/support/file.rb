class File

  # read last n lines of a file (trailing newlines are ignored)
  def tail(n)
    return [] if size==0
    buffer = 1024
    str = nil

    if size>buffer
      chunks = []
      lines = 0
      idx = size
      begin
        idx -= buffer     # rewind
        if idx<0
          buffer += idx   # adjust last read to avoid negative index
          idx = 0
        end
        seek(idx)
        chunk = read(buffer)
        chunk.gsub!(/\n+\Z/,"") if chunks.empty? # strip newlines from end of file (first chunk)
        lines += chunk.count("\n")  # update total lines found
        chunks.unshift chunk        # prepend
      end while lines<(n) && idx>0  # stop when enough lines found or no more to read
      str = chunks.join('')
    else
      str = read(buffer)
    end

    # return last n lines of str
    lines = str.split("\n")
    lines.size>=n ? lines[-n,n] : lines
  end
end