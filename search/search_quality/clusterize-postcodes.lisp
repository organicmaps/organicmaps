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
  "A cluster of postcodes with the same pattern, i.e.  all six-digits
   postcodes or all four-digits-two-letters postcodes."
  (key "") (num-samples 0) (samples nil))

(defun add-sample (cluster sample)
  "Adds a postcode sample to a cluster of samples."
  (push sample (cluster-samples cluster))
  (incf (cluster-num-samples cluster)))

(defparameter *seps* '(#\Space #\Tab #\Newline #\Backspace #\Return #\Rubout #\Linefeed #\"))

(defun trim (string)
  "Removes leading and trailing garbage from a string."
  (string-trim *seps* string))

(defun get-postcode-pattern (postcode)
  "Simplifies postcode in the following way:
   * all letters are replaced by 'A'
   * all digits are replaced by 'N'
   * hyphens and dots are replaced by a space
   * other characters are capitalized

   This format follows https://en.wikipedia.org/wiki/List_of_postal_codes.
  "
  (map 'string #'(lambda (c) (cond ((alpha-char-p c) #\A)
                                   ((digit-char-p c) #\N)
                                   ((or (char= #\- c) (char= #\. c)) #\Space)
                                   (T c)))
       (string-upcase postcode)))

(defun get-pattern-clusters (postcodes)
  "Constructs a list of clusters by a list of postcodes."
  (let ((table (make-hash-table :test #'equal))
        (clusters nil))
    (dolist (postcode postcodes)
      (let* ((trimmed-postcode (trim postcode))
             (pattern (get-postcode-pattern trimmed-postcode))
             (cluster (gethash pattern table (make-cluster :key pattern))))
        (add-sample cluster trimmed-postcode)
        (setf (gethash pattern table) cluster)))
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
          (result nil))
      ; Flattens rows into a single list.
      (dolist (row rows result)
        (dolist (value row)
          (push value result))))))

(defparameter *clusters*
  (sort (get-pattern-clusters *postcodes*)
        #'(lambda (lhs rhs) (> (cluster-num-samples lhs)
                               (cluster-num-samples rhs)))))

(defparameter *total*
  (loop for cluster in *clusters*
     summing (cluster-num-samples cluster)))

(format t "Total: ~a~%" *total*)
(loop for cluster in *clusters*
   for prev-prefix-sum = 0 then curr-prefix-sum
   for curr-prefix-sum = (+ prev-prefix-sum (cluster-num-samples cluster))

   do (let ((key (slot-value cluster 'key))
            (num-samples (slot-value cluster 'num-samples))
            (samples (slot-value cluster 'samples)))
        ; Prints number of postcodes in a cluster, accumulated
        ; percent of postcodes clustered so far, simplified version
        ; of a postcode and examples of postcodes.
        (format t "~a (~2$%) ~a [~{~a~^, ~}~:[~;, ...~]]~%"
                num-samples
                (* 100 (/ curr-prefix-sum *total*))
                key
                (subseq samples 0 (min num-samples 5))
                (> num-samples 5))))
