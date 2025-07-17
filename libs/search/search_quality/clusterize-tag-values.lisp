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

(defun starts-with (text prefix)
  "Returns non-nil if text starts with prefix."
  (and (>= (length text) (length prefix))
       (loop for u being the element of text
          for v being the element of prefix
          always (char= u v))))

(defun get-postcode-pattern (postcode fn)
  "Simplifies postcode in the following way:
   * all latin letters are replaced by 'A'
   * all digits are replaced by 'N'
   * hyphens and dots are replaced by a space
   * other characters are capitalized

   This format follows https://en.wikipedia.org/wiki/List_of_postal_codes.
  "
  (let ((pattern (map 'string #'(lambda (c) (cond ((latin-char-p c) #\A)
                                                  ((digit-char-p c) #\N)
                                                  ((or (char= #\- c) (char= #\. c)) #\Space)
                                                  (T c)))
                      (string-upcase postcode))))
    (funcall fn postcode pattern)))


(defun get-phone-or-flat-pattern (phone fn)
  "Simplifies phone or flat numbers in the following way:
   * all letters are replaced by 'A'
   * all digits are replaced by 'N'
   * other characters are capitalized
  "
  (let ((pattern (map 'string #'(lambda (c) (cond ((alpha-char-p c) #\A)
                                                  ((digit-char-p c) #\N)
                                                  (T c)))
                      (string-upcase phone))))
    (funcall fn phone pattern)))

(defun group-by (cmp list)
  "cmp -> [a] -> [[a]]

   Groups equal adjacent elements of the list. Equality is checked with cmp.
  "
  (let ((buckets
           (reduce #'(lambda (buckets cur)
                       (cond ((null buckets) (cons (list cur) nil))
                             ((funcall cmp (caar buckets) cur)
                              (cons (cons cur (car buckets)) (cdr buckets)))
                             (T (cons (list cur) buckets))))
                   list :initial-value nil)))
    (reverse (mapcar #'reverse buckets))))

(defun split-by (fn list)
  "fn -> [a] -> [[a]]

   Splits list by separators, where separators are defined by fn
   predicate.
  "
  (loop for e in list
     with buckets = nil
     for prev-sep = T then cur-sep
     for cur-sep = (funcall fn e)
     do (cond (cur-sep T)
              (prev-sep (push (list e) buckets))
              (T (push e (car buckets))))
     finally (return (reverse (mapcar #'reverse buckets)))))

(defun split-string-by (fn string)
  "fn -> string -> [string]

   Splits string by separators, where separators are defined by fn
   predicate.
  "
  (mapcar #'(lambda (list) (concatenate 'string list))
          (split-by fn (concatenate 'list string))))

(defun drop-while (fn list)
  (cond ((null list) nil)
        ((funcall fn (car list)) (drop-while fn (cdr list)))
        (T list)))

(defun take-while (fn list)
  (if (null list)
      nil
      (loop for value in list
         while (funcall fn value)
         collecting value)))

(defparameter *building-synonyms*
  '("building" "bldg" "bld" "bl" "unit" "block" "blk"
    "корпус" "корп" "кор" "литер" "лит" "строение" "стр" "блок" "бл"))

(defparameter *house-number-seps* '(#\Space #\Tab #\" #\\ #\( #\) #\. #\# #\~))
(defparameter *house-number-groups-seps* '(#\, #\| #\; #\+))

(defun building-synonym-p (s)
  (find s *building-synonyms* :test #'string=))

(defun short-building-synonym-p (s)
  (or (string= "к" s) (string= "с" s)))

(defstruct token value type)

(defun get-char-type (c)
  (cond ((digit-char-p c) :number)
        ((find c *house-number-seps* :test #'char=) :separator)
        ((find c *house-number-groups-seps* :test #'char=) :group-separator)
        ((char= c #\-) :hyphen)
        ((char= c #\/) :slash)
        (T :string)))

(defun transform-string-token (fn value)
  "Transforms building token value into one or more tokens in
   accordance to its value.  For example, 'литA' is transformed to
   tokens 'лит' (building part) and 'А' (letter).
  "
  (flet ((emit (value type) (funcall fn value type)))
    (cond ((building-synonym-p value)
           (emit value :building-part))
          ((and (= 4 (length value))
                (starts-with value "лит"))
           (emit (subseq value 0 3) :building-part)
           (emit (subseq value 3) :letter))
          ((and (= 2 (length value))
                (short-building-synonym-p (subseq value 0 1)))
           (emit (subseq value 0 1) :building-part)
           (emit (subseq value 1) :letter))
          ((= 1 (length value))
           (emit value (if (short-building-synonym-p value)
                           :letter-or-building-part
                           :letter)))
          (T (emit value :string)))))

(defun tokenize-house-number (house-number)
  "house-number => [token]"
  (let ((parts (group-by #'(lambda (lhs rhs)
                             (eq (get-char-type lhs) (get-char-type rhs)))
                         (string-downcase house-number)))
        (tokens nil))
    (flet ((add-token (value type) (push (make-token :value value :type type) tokens)))
      (dolist (part parts)
        (let ((value (concatenate 'string part))
              (type (get-char-type (car part))))
          (case type
            (:string (transform-string-token #'add-token value))
            (:separator T)
            (otherwise (add-token value type)))))
      (loop for prev = nil then curr
         for curr in tokens
         do (when (eq :letter-or-building-part (token-type curr))
              (cond ((null prev)
                     (setf (token-type curr) :letter))
                    ((eq :number (token-type prev))
                     (setf (token-type curr) :building-part)))))
      (reverse tokens))))

(defun house-number-with-optional-suffix-p (tokens)
  (case (length tokens)
    (1 (eq (token-type (first tokens)) :number))
    (2 (let ((first-type (token-type (first tokens)))
             (second-type (token-type (second tokens))))
         (and (eq first-type :number)
              (or (eq second-type :string)
                  (eq second-type :letter)
                  (eq second-type :letter-or-building-part)))))
    (otherwise nil)))

(defun get-house-number-sub-numbers (house-number)
  "house-number => [[token]]

   As house-number can be actually a collection of separated house
   numbers, this function returns a list of possible house numbers.
   Current implementation splits house number if and only if
   house-number matches the following rule:

   NUMBERS ::= (NUMBER STRING-SUFFIX?) | (NUMBER STRING-SUFFIX?) SEP NUMBERS
  "
  (let* ((tokens (tokenize-house-number house-number))
         (groups (split-by #'(lambda (token) (eq :group-separator (token-type token))) tokens)))
    (if (every #'house-number-with-optional-suffix-p groups)
        groups
        (list tokens))))

(defun join-house-number-tokens (tokens)
  "Joins token values with spaces."
  (format nil "~{~a~^ ~}" (mapcar #'token-value tokens)))

(defun join-house-number-parse (tokens)
  "Joins parsed house number tokens with spaces."
  (format nil "~{~a~^ ~}"
          (mapcar #'(lambda (token)
                      (let ((token-type (token-type token))
                            (token-value (token-value token)))
                        (case token-type
                          (:number "N")
                          (:building-part "B")
                          (:letter "L")
                          (:letter-or-building-part "U")
                          (:string "S")
                          ((:hyphen :slash :group-separator) token-value)
                          (otherwise (assert NIL NIL (format nil "Unknown token type: ~a"
                                                             token-type))))))
                  tokens)))

(defun get-house-number-pattern (house-number fn)
  (dolist (number (get-house-number-sub-numbers house-number))
    (let ((house-number (join-house-number-tokens number))
          (pattern (join-house-number-parse number)))
      (funcall fn house-number pattern))))

(defun get-house-number-strings (house-number fn)
  "Returns all strings from the house number."
  (dolist (number (get-house-number-sub-numbers house-number))
    (dolist (string (mapcar #'token-value
                            (remove-if-not #'(lambda (token)
                                               (case (token-type token)
                                                 ((:string
                                                   :letter
                                                   :letter-or-building-part
                                                   :building-part) T)
                                                 (otherwise nil)))
                                           number)))
      (funcall fn string string))))

(defstruct type-settings
  pattern-simplifier
  field-name)

(defparameter *value-type-settings*
  `(:postcode ,(make-type-settings :pattern-simplifier #'get-postcode-pattern
                                   :field-name "addr:postcode")
              :phone ,(make-type-settings :pattern-simplifier #'get-phone-or-flat-pattern
                                          :field-name "contact:phone")
              :flat ,(make-type-settings :pattern-simplifier #'get-phone-or-flat-pattern
                                         :field-name "addr:flats")
              :house-number ,(make-type-settings :pattern-simplifier #'get-house-number-pattern
                                                 :field-name "addr:housenumber")
              :house-number-strings ,(make-type-settings
                                      :pattern-simplifier #'get-house-number-strings
                                      :field-name "addr:housenumber")))

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
       do (funcall simplifier (trim value)
                   #'(lambda (value pattern)
                       (let ((cluster (gethash pattern table (make-cluster :key pattern))))
                         (add-sample cluster value count)
                         (setf (gethash pattern table) cluster)))))
    (maphash #'(lambda (pattern cluster)
                 (declare (ignore pattern))
                 (push cluster clusters))
             table)
    clusters))

(defun make-keyword (name) (values (intern (string-upcase name) "KEYWORD")))

(when (/= 3 (length *posix-argv*))
  (format t "Usage: ~a value path-to-taginfo-db.db~%" *script-name*)
  (format t "~%value can be one of the following:~%")
  (format t "~{    ~a~%~}"
          (loop for field in *value-type-settings* by #'cddr collecting (string-downcase field)))
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
