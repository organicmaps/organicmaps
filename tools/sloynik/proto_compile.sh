MY_PATH=`echo $0 | grep -o '.*/'`
CODE_PATH=$MY_PATH../../utils/
$MY_PATH./protoc --proto_path=$CODE_PATH --cpp_out=$CODE_PATH $CODE_PATH./config.proto