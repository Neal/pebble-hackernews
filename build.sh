pebble build || { exit $?; }
if [ "$1" = "install" ]; then
	pebble install --logs
fi
