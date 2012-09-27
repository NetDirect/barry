<? include ("barry.inc"); ?>

<? createHeader("Building Barry for Windows CE"); ?>

<? createSubHeader("Introduction"); ?>

<p>Barry and command line tools can be built for the Windows CE 5.0 Standard SDK.</p>

<? createSubHeader("Prerequisites"); ?>

<p>To build Barry for WinCE you will need the following files and tools.</p>
<ul>
<li>Microsoft Visual Studio 2005.</li>
<li><a href="http://www.microsoft.com/en-gb/download/details.aspx?id=17310">Windows CE 5.0: Standard SDK</a>.</li>
<li>A download of <a href="http://downloads.sourceforge.net/project/boost/boost/1.49.0/boost_1_49_0.tar.bz2">boost 1.49.0</a>.</li>
<li>A download of <a href="http://downloads.sourceforge.net/project/stlport/STLport/STLport-5.2.1/STLport-5.2.1.tar.bz2">STLPort 5.2.1</a>.</li>
<li>A cvs checkout of the source for pthreads-win32 on 23rd April 2012. This can be retrieved by running the following command.
<pre>cvs -d :pserver:anonymous@sourceware.org:/cvs/pthreads-win32 checkout -D "20120423" pthreads</pre></li>
<li>A pre-built WinCE build of libusb. This should contain an <tt>include</tt> directory and one or more of <tt>MS32</tt>, <tt>MS64</tt> and <tt>WinCE_STD500_ARMVI</tt>.</li>
</ul>

<? createSubHeader("Building"); ?>

<p>Before building Barry and the dependencies it is necessary to define the following environment variables.</p>
<ul>
<li><tt>LIBUSB_PREBUILT_DIR</tt> - To a pre-built Win32/WinCE build of libusb. This should contain an <tt>include</tt> directory and one or more of <tt>MS32</tt>, <tt>MS64</tt> and <tt>WinCE_STD500_ARMV4I</tt>.</li>
<li><tt>PTHREADS-WIN32_PREBUILT_DIR</tt> - This will be a directory that contains a pre-built Win32/WinCE build of pthreads-win32.</li>
<li><tt>BOOST_SRC_PATH</tt> - This will be a directory that contains extracted boost source.</li>
<li><tt>STLPORT_PREBUILT_DIR</tt> - This will be a directory that contains a pre-built Win32/WinCE build of STLPort.</li>
</ul>

<p>To setup the <tt>BOOST_SRC_PATH</tt> directory just extract the boost 1.49.0 source into that directory.</p>

<p>To setup the <tt>PTHREADS-WIN32_PREBUILT_DIR</tt> directory do the following.</p>
<ol>
<li>Apply the patch contained in <tt>wince/pthreads_patch_20120423.diff</tt> to the pthreads-win32 source. It might be necessary to adjust the path used for the directory containing the source.</li>
<li>Add the following environment variables, pointing to the CE build tool paths.
<pre>set PATH=%PROGRAMFILES%\Microsoft Visual Studio 8\vc\ce\bin\x86_arm;%PROGRAMFILES%\Microsoft Visual Studio 8\VC\bin;%PROGRAMFILES%\Microsoft Visual Studio 8\Common7\IDE;%PATH%
set INCLUDE=%PROGRAMFILES%\Windows CE Tools\wce500\STANDARDSDK_500\Include\Armv4i\;C:\src\pthreads
set LIB=%PROGRAMFILES%\Windows CE Tools\wce500\STANDARDSDK_500\Lib\ARMV4I;%PROGRAMFILES%\Microsoft Visual Studio 8\vc\ce\lib\armv4;%PROGRAMFILES%\Microsoft Visual Studio 8\VC\ATLMFC\LIB;%PROGRAMFILES%\Microsoft Visual Studio 8\VC\LIB;%PROGRAMFILES%\Microsoft Visual Studio 8\VC\PlatformSDK\lib;%PROGRAMFILES%\Microsoft Visual Studio 8\SDK\v2.0\lib;</pre></li>
<li>Compile the code by running the following command from the pthreads-win32 source directory.
<pre>nmake clean VC</pre></li>
<li>Move the built files to the correct location with the following commands, again from the pthreads-win32 source directory.
<pre>set ARCH=WinCE_STD500_ARMV4I
md %PTHREADS-WIN32_PREBUILT_DIR%\include
copy pthread.h %PTHREADS-WIN32_PREBUILT_DIR%\include
copy sched.h %PTHREADS-WIN32_PREBUILT_DIR%\include
copy semaphore.h %PTHREADS-WIN32_PREBUILT_DIR%\include
copy need_errno.h %PTHREADS-WIN32_PREBUILT_DIR%\include
md %PTHREADS-WIN32_PREBUILT_DIR%\%ARCH%
md %PTHREADS-WIN32_PREBUILT_DIR%\%ARCH%\dll
copy pthread*.dll %PTHREADS-WIN32_PREBUILT_DIR%\%ARCH%\dll
copy pthread*.lib %PTHREADS-WIN32_PREBUILT_DIR%\%ARCH%\dll</pre></li>
<li>Edit <tt>%PTHREADS-WIN32_PREBUILT_DIR%\include\need_errno.h</tt> so that it contains <tt>"#include &lt;winsock2.h&gt;"</tt> instead of <tt>"#include &lt;winsock.h&gt;"</tt></li>
</ol>

<p>To setup the <tt>STLPORT_PREBUILT_DIR</tt> directory run the following commands from the STLPort source directory.</p>
<pre>set PATH=%PROGRAMFILES%\Microsoft Visual Studio 8\vc\ce\bin\x86_arm;%PROGRAMFILES%\Microsoft Visual Studio 8\VC\bin;%PROGRAMFILES%\Microsoft Visual Studio 8\Common7\IDE;%PATH%
set INCLUDE=%PROGRAMFILES%\Windows CE Tools\wce500\STANDARDSDK_500\Include\Armv4i\;%PROGRAMFILES%\Windows CE Tools\wce500\STANDARDSDK_500\Include\MFC\include;%PROGRAMFILES%\Windows CE Tools\wce500\STANDARDSDK_500\Include\ATL\include
set LIB=%PROGRAMFILES%\Windows CE Tools\wce500\STANDARDSDK_500\Lib\ARMV4I;%PROGRAMFILES%\Windows CE Tools\wce500\STANDARDSDK_500\MFC\Lib\ARMV4I;%PROGRAMFILES%\Windows CE Tools\wce500\STANDARDSDK_500\ATL\Lib\ARMV4I;%PROGRAMFILES%\Microsoft Visual Studio 8\vc\ce\lib\armv4
set TARGETCPU=ARMV4I
configure evc8
cd build\lib
nmake clean install

REM Move the built files to the correct location:
cd ..\..
set ARCH=WinCE_STD500_ARMV4I
md %STLPORT_PREBUILT_DIR%\include
xcopy /S /Y stlport\* %STLPORT_PREBUILT_DIR%\include
del %STLPORT_PREBUILT_DIR%\include\pthread.h
del %STLPORT_PREBUILT_DIR%\include\signal.h
del %STLPORT_PREBUILT_DIR%\include\cstddef
md %STLPORT_PREBUILT_DIR%\%ARCH%
md %STLPORT_PREBUILT_DIR%\%ARCH%\dll
copy bin\evc8-arm\*.dll %STLPORT_PREBUILT_DIR%\%ARCH%\dll
copy lib\evc8-arm\*.lib %STLPORT_PREBUILT_DIR%\%ARCH%\dll</pre>

<p>To build Barry just open <tt>wince/barry.sln</tt> and select build.</p>

<? createSubHeader("Installing"); ?>

<p>To install Barry just copy across the binaries from the <tt>barry\wince\dist</tt> directory.</p>

<? createSubHeader("Running"); ?>

<p>When running barry you'll also need the following libraries in your PATH.</p>
<ul>
<li><tt>libusb-1.0.so</tt> - from LIBUSB_PREBUILT_DIR
<li><tt>pthreadVC2.dll</tt>, <tt>pthreadVC2d.dll</tt> - from the pthreads-win32 build.</li>
<li><tt>stlport.5.2.dll</tt>, <tt>stlportd.5.2.dll</tt> - from the STLPort build.</li>
<li><tt>msvcr80.dll</tt>, <tt>msvcr80d.dll</tt> - from your Visual Studio install,
usually found in <tt>%PROGRAMFILES%\Microsoft Visual Studio 8\vc\ce\dll\%ARCH%</tt></li>
</li>
