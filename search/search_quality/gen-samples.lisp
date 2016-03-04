#|
exec /usr/local/bin/sbcl --noinform --quit --load $0 --end-toplevel-options "$@"
|#

; Silently loads :cl-json.
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
         (result (rad-to-deg (* 0.5 (log (/ (+ 1.0 sinx) (- 1.0 sinx)))))))
    (clamp-y result)))

(defclass pos () ((x :initarg :x)
                  (y :initarg :y)))
(defclass viewport () ((minx :initarg :minx)
                       (miny :initarg :miny)
                       (maxx :initarg :maxx)
                       (maxy :initarg :maxy)))

(defun position-lat-lon (lat lon)
  (make-instance 'pos :x (lon-to-x lon) :y (lat-to-y lat)))

(defun viewport (&key minx miny maxx maxy)
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

(defmacro defsample (&rest args)
  `(push (make-sample ,@args) *samples*))

(load (merge-pathnames "samples.lisp" *load-truename*))

(format *error-output* "Num samples: ~a~%" (length *samples*))
(format t "~a~%" (json:encode-json-to-string *samples*))
