#!/usr/bin/env ruby

#
#    Copyright (C) 2008, Andy Herkey <a.herkey@comcast.net>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
#    See the GNU General Public License in the COPYING file at the
#    root directory of this project for more details.
#
#

#
# The purpose of this script is to try a large batch of known AT
# commands, and report which ones work with your Blackberry
# modem / provider.  There will be output written to stdout
# as well as a file created in /tmp/modemtest.log which holds
# the raw pppob traffic log.
#

#Change the $PPP variable to point to where pppob is installed.
$PPP="/usr/local/sbin/pppob -vP xxxxx -l /tmp/modemtest.log"

class IPmodem

   def initialize
      @connecting = true
      @connectionHandle = IO.popen( $PPP,"w+" )
      sleep 2
      authAtempts = 0
      timeout = 0
   end

   def write( data )
      @connectionHandle.write( data ) 
   end

   def read()
      timeout = 0
      return nil if @connectionHandle.closed?
         @ioBuffer = ""
         while input = IO.select([@connectionHandle],nil,nil,1)
            c = @connectionHandle.getc
            @ioBuffer <<  c if c != nil
            return if ! @connecting
            if c == nil
               timeout+=1
               break if timeout >= 9000
            else
               timeout = 0
            end
         end
      return @ioBuffer
   end
end

commands = [
	"+++AT",
	"AT",
	"AT&F",
	"ATZ",
	"ATS0=0",
	"ATE0",
	"ATE0V1",
	"ATE0V1Q0X4",
	"AT+CRC=1",
	"AT+SPSERVICE",
	"AT+SPSERVICE",
	"AT$QCMIPP?",
	"AT$QCMIPP=?",
	"AT+CSQ",
	"AT+CSQ?",
	"AT+CSQ=?",
	"AT+CSS",
	"AT+CSS?",
	"AT+CSS=?",
	"ATI1",
	"ATI2",
	"ATI3",
	"AT+CAD?",
	"AT+CIMI",
	"AT+CGMI",
	"AT+CGMR",
	"AT+CGDCONT?",
	"AT+GMI",
	"AT+GMM",
	"AT+GMR",
	"AT+GSN",
	"AT+CBC",
	"AT+CBIP",
	"AT+CCED?",
	"AT+ESR",
	"AT+CIND=?",
	"AT+FCLASS=?",
	"AT+GCAP=?",
	"AT$SPMDN?",
	"AT$QCMIPGETP=1",
	"AT&V",
	"ATH"
]

modem = IPmodem.new
puts("Testing the Blackberry Modem by sending AT commands through pppob.") 
printf("Starting %s: %s",$PPP,modem.read())
puts("--------------------------------------------")

commands.each {|c|
         modem.write(c+"\r")
         printf("Command: %s \nResult: %s",c,modem.read())
         puts("--------------------------------------------")

         }
