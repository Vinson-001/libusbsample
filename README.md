# libusbsample

# using
	CLibusbControl usbControl;
	usbControl.setusbid(iVid, iPid);
	usbControl.control_write(cmd, NULL, len, 0);

