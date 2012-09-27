<? include ("barry.inc"); ?>

<? createHeader("Building Barry for Android"); ?>

<? createSubHeader("Introduction"); ?>

<p>Barry can be built as a C++ library for the <a
href="http://www.android.com/">Android operating system</a>. Several
of the command line tools provided with Barry can also be built for
Android.</p>

<p>There are currently no Java APIs and no graphical
applications for Android which make use of Barry.</p>

<? createSubHeader("Prerequisites"); ?>

<p>To build Barry for Android you will require the following to be
installed on the system performing the build:</p>
<ul>
<li>wget - this can be found as a package in most Linux distributions</li>
<li>tar - this is installed by default on most Linux distributions</li>
<li>GNU make - this can be found as a package in most Linux distributions</li>
<li>Android NDK - this can be downloaded from <a href="http://developer.android.com/tools/sdk/ndk/index.html">the Android developer site</a>.
</ul>

<p>The Android device that will run Barry will also need a USB port
capable of being a USB host. It is also necessary for the Android
device to be setup in a way to allow permissions to the USB device
nodes.</p>

<? createSubHeader("Building"); ?>

<p>Building of Barry for Android is best performed on a Linux machine.</p>
<ol>
<li>Open a shell and ensure that the tools describe above, including 'ndk-build' are on the PATH.</li>
<li>Extract the source for Barry and enter the change to the 'android' directory.
<pre>user@machine:~$ cd src/barry
user@machine:~/src/barry$ cd android
user@machine:~/src/barry/android$ </pre></li>
<li>Run the 'make' command.
<pre>user@machine:~/src/barry/android$ make</pre></li>
<li>Wait for the compilation to complete successfully. If the build fails then check that all the required tools are on the PATH.</li>
</ol>

<? createSubHeader("Installing"); ?>

<p>The easiest way to install Barry on an Android device is to place it into
the system image. This can only be done on a 'rooted' device.</p>
<ol>
<li>Make sure that the device you wish to install Barry on is connected to the machien you built Barry on and that USB debugging is enabled on the device.</li>
<li>Remount the system partition as read/write.
<pre>user@machine:~/src/barry/android$ adb remount</pre></li>
<li>Push the library dependencies to the device.
<pre>user@machine:~/src/barry/android$ adb push libs/armeabi/libiconv.so /system/lib
user@machine:~/src/barry/android$ adb push libs/armeabi/libusb1.0.so /system/lib</pre></li>
<li>Push the Barry tools to the device. This example installs the 'btool' executable.
<pre>user@machine:~/src/barry/android$ adb push libs/armeabi/btool /system/bin</pre></li>
</ol>

<? createSubHeader("Running"); ?>

<p>To run the Barry tools on Android just execute the command at a shell. For example, 'btool' can be run via adb with the following command:</p>
<pre>user@machine:~$ adb shell btool
Blackberry devices found:
No device selected</pre>

<p>If no devices are found then it is recommended to check the permissions of the USB device files. These files can be found in the Android file system in <tt>/dev/bus/usb</tt>. Running the following command will give all users access to all USB devices, which is not recommended, but can be useful in discovering if there are issues with the permissions.</p>
<pre>user@machine:~$ adb shell chmod -R 0666 /dev/bus/usb</pre>
