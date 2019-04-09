#|
exec /usr/bin/env sbcl --noinform --quit --load "$0" --end-toplevel-options "$@"
|#

;;; Silently loads :cl-json.
(with-open-file (*standard-output* "/dev/null"
                                   :direction :output
                                   :if-exists :supersede)
  (ql:quickload "cl-json"))

(defparameter *samples* nil)

(defparameter *minx* -180)
(defparameter *maxx* 180)
(defparameter *miny* -180)
(defparameter *maxy* 180)

(defun deg-to-rad (deg) (/ (* deg pi) 180))
(defun rad-to-deg (rad) (/ (* rad 180) pi))

(defun clamp (value min max)
  (cond ((< value min) min)
        ((> value max) max)
        (t value)))

(defun clamp-x (x) (clamp x *minx* *maxx*))
(defun clamp-y (y) (clamp y *miny* *maxy*))

(defun lon-to-x (lon) lon)
(defun lat-to-y (lat)
  (let* ((sinx (sin (deg-to-rad (clamp lat -86.0 86.0))))
         (result (rad-to-deg (* 0.5
                                (log (/ (+ 1.0 sinx)
                                        (- 1.0 sinx)))))))
    (clamp-y result)))

(defclass pos () ((x :initarg :x)
                  (y :initarg :y)))
(defclass viewport () ((minx :initarg :minx)
                       (miny :initarg :miny)
                       (maxx :initarg :maxx)
                       (maxy :initarg :maxy)))

(defun position-x-y (x y)
  (assert (and (>= x *minx*) (<= x *maxx*)))
  (assert (and (>= y *miny*) (<= y *maxy*)))
  (make-instance 'pos :x x :y y))

(defun position-lat-lon (lat lon)
  (position-x-y (lon-to-x lon) (lat-to-y lat)))

(defun viewport (&key minx miny maxx maxy)
  (assert (<= minx maxx))
  (assert (<= miny maxy))
  (make-instance 'viewport :minx minx :maxx maxx :miny miny :maxy maxy))

(defclass result ()
  ((name :initarg :name)
   (relevancy :initarg :relevancy)
   (types :initarg :types)
   (position :initarg :position)
   (house-number :initarg :house-number)))

(defun make-result (relevancy name types position &key (house-number ""))
  (make-instance 'result
                 :name name
                 :relevancy relevancy
                 :types types
                 :position position
                 :house-number house-number))

(defmacro vital (&rest args)
  `(make-result 'vital ,@args))

(defmacro relevant (&rest args)
  `(make-result 'relevant ,@args))

(defmacro irrelevant (&rest args)
  `(make-result 'irrelevant ,@args))

(defmacro harmful (&rest args)
  `(make-result 'harmful ,@args))

(defclass sample ()
  ((query :initarg :query)
   (locale :initarg :locale)
   (position :initarg :position)
   (viewport :initarg :viewport)
   (results :initarg :results)))

(defun make-sample (query locale position viewport results)
  (make-instance 'sample
                 :query query
                 :locale locale
                 :position position
                 :viewport viewport
                 :results results))

(defmacro with-gensyms ((&rest syms) &rest body)
  `(let ,(loop for sym in syms
            collecting `(,sym (gensym)))
     ,@body))

(defmacro defsample (&rest args)
  `(push (make-sample ,@args) *samples*))

(defmacro scoped-samples ((locale position viewport) &rest body)
  (with-gensyms (ls ps vs)
    `(let ((,ls ,locale)
           (,ps ,position)
           (,vs ,viewport))
       (flet ((def (query results)
                (defsample query ,ls ,ps ,vs results)))
         ,@body))))

(defun power-set (seq)
  (unless seq (return-from power-set '(())))
  (let ((x (car seq))
        (ps (power-set (cdr seq))))
    (concatenate 'list ps (mapcar #'(lambda (xs) (cons x xs)) ps))))

(defun join-strings (strings)
  "Joins a list of strings with spaces between them."
  (with-output-to-string (s)
    (format s "~{~a~^ ~}" strings)))

;;; Loads samples specification from standard input.
(load *standard-input*)

(format *error-output* "Num samples: ~a~%" (length *samples*))
(format *error-output* "Num results: ~a~%"
        (loop for sample in *samples*
             summing (length (slot-value sample 'results))))

(format t "~{~a~%~}" (mapcar #'json:encode-json-to-string (reverse *samples*)))
