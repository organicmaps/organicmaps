#|
exec /usr/bin/env sbcl --noinform --quit --load "$0" --end-toplevel-options "$@"
|#

;;; This script clusterizes postcodes from the taginfo database and
;;; prints information about clusters.

;;; Silently loads sqlite.
(with-open-file (*standard-output* "/dev/null"
                                   :direction :output
                                   :if-exists :supersede)
  (ql:quickload "sqlite"))

(defstruct cluster
  "A cluster of postcodes with the same pattern, i.e.  all six-digirs
   postcodes or all four-digits-two-letters postcodes."
  (key "") (num-samples 0) (samples (list)))

(defun add-sample (cluster sample)
  "Adds a postcode sample to a cluster of samples."
  (push sample (slot-value cluster 'samples))
  (incf (slot-value cluster 'num-samples)))

(defparameter *seps* '(#\Space #\Tab #\Newline #\Backspace #\Return #\Rubout #\Linefeed #\"))

(defun trim (string)
  "Removes leading and trailing garbage from a string."
  (string-trim *seps* string))

(defun get-postcode-pattern (postcode)
  "Simplifies postcode in a following way:
   * all letters are replaced by 'a'
   * all digits are replaced by '0'
   * other characters are left as-is
  "
  (map 'string #'(lambda (c) (cond ((alpha-char-p c) #\a)
                                   ((digit-char-p c) #\0)
                                   (T c)))
       postcode))

(defun get-pattern-clusters (postcodes)
  "Constructs a list of clusters by a list of postcodes."
  (let ((table (make-hash-table :test #'equal))
        (clusters (list)))
    (dolist (postcode postcodes)
      (let* ((trimmed-postcode (trim postcode))
             (pattern (get-postcode-pattern trimmed-postcode)))
        (unless (gethash pattern table)
          (setf (gethash pattern table) (make-cluster :key pattern)))
        (add-sample (gethash pattern table) trimmed-postcode)))
    (maphash #'(lambda (pattern cluster)
                 (declare (ignore pattern))
                 (push cluster clusters))
             table)
    clusters))

(when (/= 2 (length *posix-argv*))
  (format t "Usage: ./clusterize-postcodes.lisp path-to-taginfo-db.db~%")
  (exit :code -1))

(defparameter *db-path* (second *posix-argv*))

(defparameter *postcodes*
  (sqlite:with-open-database (db *db-path*)
    (let ((rows (sqlite:execute-to-list db "select value from tags where key=\"addr:postcode\";"))
          (result (list)))
      ; Flattens rows into a single list.
      (dolist (row rows result)
        (dolist (value row)
          (push value result))))))

(defparameter *clusters* (get-pattern-clusters *postcodes*))
(setf *clusters* (sort *clusters*
                       #'(lambda (lhs rhs) (> (slot-value lhs 'num-samples)
                                              (slot-value rhs 'num-samples)))))

(defparameter *total*
  (loop for cluster in *clusters*
     summing (slot-value cluster 'num-samples)))

(format t "Total: ~a~%" *total*)
(loop for cluster in *clusters*
   for prev-prefix-sum = 0 then curr-prefix-sum
   for curr-prefix-sum = (+ prev-prefix-sum (slot-value cluster 'num-samples))

   do (let ((key (slot-value cluster 'key))
            (num-samples (slot-value cluster 'num-samples))
            (samples (slot-value cluster 'samples)))
        ; Prints number of postcodes in a cluster, accumulated
        ; percent of postcodes clustered so far, simplified version
        ; of a postcode and examples of postcodes.
        (format t "~a (~2$%) ~a [~{~a~^, ~}]~%"
                num-samples
                (coerce (* 100 (/ curr-prefix-sum *total*)) 'double-float)
                key
                (subseq samples 0 (min num-samples 5)))))
