# to threat ! as a char
set +H

# this array will hold all executed commands logs
declare -a FUNCTION_CHECKED_CALL_LOGS
FUNCTION_CHECKED_LOG_INDEX=0

FUNCTION_START_DATE=$(date -u)

LoggedCall() {
  echo "\$ $*"
  local RESULT
  RESULT=$($* 2>&1)
  if [ $? != 0 ]; then
    echo "ERROR: Command failed with code $?"
  fi
  echo "$RESULT"
}

Call() {
  FUNCTION_CHECKED_CALL_LOGS[FUNCTION_CHECKED_LOG_INDEX]=$(LoggedCall $*)
  let FUNCTION_CHECKED_LOG_INDEX++
}

PrintLogs() {
  echo "****** Started on $FUNCTION_START_DATE ******"
  for logLine in "${FUNCTION_CHECKED_CALL_LOGS[@]}"; do
    echo "$logLine"
  done
}

CD()
{
  echo "\$ cd $1"
  cd $1 || return
}
