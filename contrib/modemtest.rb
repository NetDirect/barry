#!/usr/bin/env ruby

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
