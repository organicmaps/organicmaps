#monkey patch cucumber table class to reorder output.
#we always want failed rows to be shown right below the expected row.

class Cucumber::Ast::Table
  def routing_diff!(other_table, options={})
    options = {:missing_row => true, :surplus_row => true, :missing_col => true, :surplus_col => false}.merge(options)

    other_table = ensure_table(other_table)
    other_table.convert_headers!
    other_table.convert_columns!
    ensure_green!

    convert_headers!
    convert_columns!

    original_width = cell_matrix[0].length
    other_table_cell_matrix = pad!(other_table.cell_matrix)
    padded_width = cell_matrix[0].length

    missing_col = cell_matrix[0].detect{|cell| cell.status == :undefined}
    surplus_col = padded_width > original_width

    require_diff_lcs
    cell_matrix.extend(Diff::LCS)
    changes = cell_matrix.diff(other_table_cell_matrix).flatten

    inserted = 0
    missing  = 0

    row_indices = Array.new(other_table_cell_matrix.length) {|n| n}

    last_change = nil
    missing_row_pos = nil
    insert_row_pos  = nil

    changes.each do |change|
      if(change.action == '-')
        missing_row_pos = change.position + inserted
        cell_matrix[missing_row_pos].each{|cell| cell.status = :undefined}
        row_indices.insert(missing_row_pos, nil)
        missing += 1
      else # '+'
        #change index so we interleave instead
        insert_row_pos = change.position + inserted + 1
        #insert_row_pos = change.position + missing     #original

        inserted_row = change.element
        inserted_row.each{|cell| cell.status = :comment}
        cell_matrix.insert(insert_row_pos, inserted_row)
        row_indices[insert_row_pos] = nil
        inspect_rows(cell_matrix[missing_row_pos], inserted_row) if last_change && last_change.action == '-'
        inserted += 1
      end
      last_change = change
    end

    other_table_cell_matrix.each_with_index do |other_row, i|
      row_index = row_indices.index(i)
      row = cell_matrix[row_index] if row_index
      if row
        (original_width..padded_width).each do |col_index|
          surplus_cell = other_row[col_index]
          row[col_index].value = surplus_cell.value if row[col_index]
        end
      end
    end

    clear_cache!
    should_raise =
    missing_row_pos && options[:missing_row] ||
    insert_row_pos  && options[:surplus_row] ||
    missing_col     && options[:missing_col] ||
    surplus_col     && options[:surplus_col]
    raise Different.new(self) if should_raise
  end
end