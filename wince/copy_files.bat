@echo off

set PLATFORMNAME=%1
set CONFIGURATIONNAME=%2

REM strip off the trailing and leading " by dropping those characters
set PLATFORMNAME=%PLATFORMNAME:~1,-1%
set CONFIGURATIONNAME=%CONFIGURATIONNAME:~1,-1%

echo PLATFORMNAME=%PLATFORMNAME%
echo CONFIGURATIONNAME=%CONFIGURATIONNAME%

IF NOT EXIST dist md dist
IF NOT EXIST dist\include md dist\include
IF NOT EXIST dist\include\barry (
	echo Creating barry directory
	md dist\include\barry
)

REM install the headers

for %%H in (
	barry.h
	barrysync.h
	barrybackup.h
	barryalx.h
	dll.h
	builder.h
	common.h
	configfile.h
	controller.h
	xmlparser.h
	a_common.h
	a_codsection.h
	a_library.h
	a_application.h
	a_osloader.h
	a_alxparser.h
	m_mode_base.h
	m_desktop.h
	m_raw_channel.h
	m_desktoptmpl.h
	m_ipmodem.h
	m_serial.h
	m_javaloader.h
	m_jvmdebug.h
	data.h
	error.h
	ldif.h
	ldifio.h
	log.h
	parser.h
	pin.h
	probe.h
	protocol.h
	record.h
	recordtmpl.h
	modem.h
	r_recur_base.h
	r_calendar.h
	r_calllog.h
	r_bookmark.h
	r_contact.h
	r_cstore.h
	r_folder.h
	r_hhagent.h
	r_memo.h
	r_message_base.h
	r_message.h
	r_pin_message.h
	r_saved_message.h
	r_servicebook.h
	r_sms.h
	r_task.h
	r_timezone.h
	dataqueue.h
	router.h
	socket.h
	time.h
	threadwrap.h
	vsmartptr.h
	version.h
	pppfilter.h
	sha1.h
	iconv.h
	cod.h
	bmp.h
	s11n-boost.h
	dp_codinfo.h
	j_manager.h
	j_server.h
	vformat.h
	vbase.h
	vcard.h
	vevent.h
	vjournal.h
	vtodo.h
	mimeio.h
	scoped_lock.h
	semaphore.h
	backup.h
	restore.h
	pipe.h
	connector.h
	trim.h
	tzwrapper.h
	usbwrap.h
	tr1_support.h
	fifoargs.h
) DO (
	echo Installing %%H header
	copy ..\src\%%H dist\include\barry
)

REM install the binaries
IF NOT EXIST "dist\%PLATFORMNAME%" md "dist\%PLATFORMNAME%"
IF NOT EXIST "dist\%PLATFORMNAME%\%CONFIGURATIONNAME%" md "dist\%PLATFORMNAME%\%CONFIGURATIONNAME%"
echo Copying libraries
copy "%CONFIGURATIONNAME%\%PLATFORMNAME%\libbarry.lib" "dist\%PLATFORMNAME%\%CONFIGURATIONNAME%"
copy "%CONFIGURATIONNAME%\%PLATFORMNAME%\libbarry.dll" "dist\%PLATFORMNAME%\%CONFIGURATIONNAME%"