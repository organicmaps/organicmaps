#|
exec /usr/bin/env sbcl --noinform --quit --eval "(defparameter *script-name* \"$0\")" --load "$0" --end-toplevel-options "$@"
|#

;;; This script clusterizes values from the taginfo database and
;;; prints information about clusters.

;;; Silently loads sqlite.
(with-open-file (*standard-output* "/dev/null"
                                   :direction :output
                                   :if-exists :supersede)
  (ql:quickload "sqlite"))

(defun latin-char-p (char)
  (or (and (char>= char #\a) (char<= char #\z))
      (and (char>= char #\A) (char<= char #\Z))))

(defun get-postcode-pattern (postcode)
  "Simplifies postcode in the following way:
   * all latin letters are replaced by 'A'
   * all digits are replaced by 'N'
   * hyphens and dots are replaced by a space
   * other characters are capitalized

   This format follows https://en.wikipedia.org/wiki/List_of_postal_codes.
  "
  (map 'string #'(lambda (c) (cond ((latin-char-p c) #\A)
                                   ((digit-char-p c) #\N)
                                   ((or (char= #\- c) (char= #\. c)) #\Space)
                                   (T c)))
       (string-upcase postcode)))


(defun get-phone-or-flat-pattern (phone)
  "Simplifies phone or flat numbers in the following way:
   * all letters are replaced by 'A'
   * all digits are replaced by 'N'
   * other characters are capitalized
  "
  (map 'string #'(lambda (c) (cond ((alpha-char-p c) #\A)
                                   ((digit-char-p c) #\N)
                                   (T c)))
       (string-upcase phone)))

(defstruct type-settings
  pattern-simplifier
  field-name)

(defparameter *value-type-settings*
  `(:postcode ,(make-type-settings :pattern-simplifier #'get-postcode-pattern
                                   :field-name "addr:postcode")
              :phone ,(make-type-settings :pattern-simplifier #'get-phone-or-flat-pattern
                                          :field-name "contact:phone")
              :flat ,(make-type-settings :pattern-simplifier #'get-phone-or-flat-pattern
                                         :field-name "addr:flats")))

(defstruct cluster
  "A cluster of values with the same pattern, i.e.  all six-digits
   series or all four-digits-two-letters series."
  (key "") (num-samples 0) (samples nil))

(defun add-sample (cluster sample &optional (count 1))
  "Adds a value sample to a cluster of samples."
  (push sample (cluster-samples cluster))
  (incf (cluster-num-samples cluster) count))

(defparameter *seps* '(#\Space #\Tab #\Newline #\Backspace #\Return #\Rubout #\Linefeed #\"))

(defun trim (string)
  "Removes leading and trailing garbage from a string."
  (string-trim *seps* string))

(defun get-pattern-clusters (values simplifier)
  "Constructs a list of clusters by a list of values."
  (let ((table (make-hash-table :test #'equal))
        (clusters nil))
    (loop for (value count) in values
       do (let* ((trimmed-value (trim value))
                 (pattern (funcall simplifier trimmed-value))
                 (cluster (gethash pattern table (make-cluster :key pattern))))
            (add-sample cluster trimmed-value count)
            (setf (gethash pattern table) cluster)))
    (maphash #'(lambda (pattern cluster)
                 (declare (ignore pattern))
                 (push cluster clusters))
             table)
    clusters))

(defun make-keyword (name) (values (intern (string-upcase name) "KEYWORD")))

(when (/= 3 (length *posix-argv*))
  (format t "Usage: ~a ~{~a~^|~} path-to-taginfo-db.db~%"
          *script-name*
          (loop for field in *value-type-settings* by #'cddr collecting field))
  (exit :code -1))

(defparameter *value-type* (second *posix-argv*))
(defparameter *db-path* (third *posix-argv*))

(defparameter *type-settings* (getf *value-type-settings* (make-keyword *value-type*)))

(defparameter *values*
  (sqlite:with-open-database (db *db-path*)
    (let ((query (format nil "select value, count_all from tags where key=\"~a\";"
                         (type-settings-field-name *type-settings*))))
      (sqlite:execute-to-list db query))))

(defparameter *clusters*
  (sort (get-pattern-clusters *values* (type-settings-pattern-simplifier *type-settings*))
        #'(lambda (lhs rhs) (> (cluster-num-samples lhs)
                               (cluster-num-samples rhs)))))

(defparameter *total*
  (loop for cluster in *clusters*
     summing (cluster-num-samples cluster)))

(format t "Total: ~a~%" *total*)
(loop for cluster in *clusters*
   for prev-prefix-sum = 0 then curr-prefix-sum
   for curr-prefix-sum = (+ prev-prefix-sum (cluster-num-samples cluster))
   do (let ((key (cluster-key cluster))
            (num-samples (cluster-num-samples cluster))
            (samples (cluster-samples cluster)))
        ; Prints number of values in a cluster, accumulated
        ; percent of values clustered so far, simplified version
        ; of a value and examples of values.
        (format t "~a (~2$%) ~a [~{~a~^, ~}~:[~;, ...~]]~%"
                num-samples
                (* 100 (/ curr-prefix-sum *total*))
                key
                (subseq samples 0 (min (length samples) 5))
                (> num-samples 5))))
