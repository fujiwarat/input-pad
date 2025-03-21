AC_DEFUN([DEFINE_INPUT_PAD_LOCALEDIR], [
input_pad_save_prefix="$prefix"
input_pad_save_datarootdir="$datarootdir"
input_pad_save_datadir="$datadir"
input_pad_save_localedir="$localedir"
test "x$prefix" = xNONE && prefix=$ac_default_prefix
datarootdir=`eval echo "$datarootdir"`
datadir=`eval echo "$datadir"`
test "x$localedir" = xNONE && localedir="${datadir}/locale"
input_pad_localedir=`eval echo "$localedir"`
input_pad_datadir=`eval echo "$datadir"`
localedir="$input_pad_save_localedir"
datadir="$input_pad_save_datadir"
datarootdir="$input_pad_save_datarootdir"
prefix="$input_pad_save_prefix"
])
