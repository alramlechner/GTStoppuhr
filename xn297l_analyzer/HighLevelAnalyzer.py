# High Level Analyzer
# For more information and documentation, please go to https://support.saleae.com/extensions/high-level-analyzer-extensions

from saleae.analyzers import HighLevelAnalyzer, AnalyzerFrame, StringSetting, NumberSetting, ChoicesSetting


# High level analyzers must subclass the HighLevelAnalyzer class.
class Hla(HighLevelAnalyzer):
    # List of settings that a user can set for this High Level Analyzer.
    #my_string_setting = StringSetting()
    #my_number_setting = NumberSetting(min_value=0, max_value=100)
    ignoreEmptyStatusRead = ChoicesSetting(choices=('Y', 'N'))

    startTime = None
    command = None
    data = []

    # An optional list of types this analyzer produces, providing a way to customize the way frames are displayed in Logic 2.
    result_types = {
        'readReg': {
            'format': 'Read reg: {{data.registerName}} ({{data.registerNum}}): {{data.flags}}'
        },
        'writeReg': {
            'format': 'Write reg: {{data.registerName}} ({{data.registerNum}}): {{data.flags}}'
        },
        'command': {
            'format': 'Cmd: {{data.command}}'
        },
        'mytype': {
            'format': 'Output type: {{type}}, Input type: {{data.input_type}}'
        }
    }

    def __init__(self):
        '''
        Initialize HLA.

        Settings can be accessed using the same name used above.
        '''
        command = None
        data = []

        #print("Settings:", self.my_string_setting,
        #      self.my_number_setting, self.my_choices_setting)

    def getRegisterName(self, regNum):
        if regNum == 0x00:
          return 'Config'
        if regNum == 0x01:
          return 'Auto ACK enable'
        if regNum == 0x02:
          return 'RX pipe enable'
        if regNum == 0x03:
          return 'Adress length'
        if regNum == 0x04:
          return 'Auto retrans'
        if regNum == 0x05:
          return 'RF Channel'
        if regNum == 0x06:
          return 'RF Speed'
        if regNum == 0x07:
          return 'Status'
        if regNum == 0x08:
          return 'TX observe'
        if regNum == 0x09:
          return 'Received power detector'
        if regNum == 0x0A:
          return 'RX address P0'
        if regNum == 0x0B:
          return 'RX address P1'
        if regNum == 0x0C:
          return 'RX address P2'
        if regNum == 0x0D:
          return 'RX address P3'
        if regNum == 0x0E:
          return 'RX address P4'
        if regNum == 0x0F:
          return 'RX address P5'
        if regNum == 0x10:
          return 'TX address'
        if regNum == 0x11:
          return 'RX bytes in P0'
        if regNum == 0x12:
          return 'RX bytes in P1'
        if regNum == 0x13:
          return 'RX bytes in P2'
        if regNum == 0x14:
          return 'RX bytes in P3'
        if regNum == 0x15:
          return 'RX bytes in P4'
        if regNum == 0x16:
          return 'RX bytes in P5'
        if regNum == 0x17:
          return 'FIFO status'
        if regNum == 0x19:
          return 'DEMOD_CAL'
        if regNum == 0x1A:
          return 'RF_CAL2'
        if regNum == 0x1B:
          return 'DEM_CAL2'
        if regNum == 0x1C:
          return 'Dynamic payload enable'
        if regNum == 0x1D:
          return 'Feature'
        if regNum == 0x1E:
          return 'RF_CAL'
        if regNum == 0x1F:
          return 'BB_CAL'
        return '?'

    def decode(self, frame: AnalyzerFrame):
        if frame.type == 'enable':
          self.startTime = frame.start_time
          self.command = None
          self.data = []
          return
        if frame.type == 'result':
            # first byte is the command
            if self.command is None:
                self.command = frame.data['mosi'][0]
                return
            # it must be data byte
            self.data.append(frame.data['mosi'][0])
            return
        if frame.type == 'error':
            return
        if frame.type != 'disable':
            return
        # diable has been sent - decode now
        if (self.command is None):
            return
        # read register
        if (self.command >> 5) == 0:
            regNum = self.command & 0x1F
            flags = ""
            if (regNum == 0x07 and self.data[0]&0x01 == 0x01):
                flags += "TX_FULL "
            if (regNum == 0x07 and self.data[0]&0x0E != 0x0E):
                flags += "RX_DATA_AVAILABLE "
            if (self.data[0] == 0x0E):
                if (self.ignoreEmptyStatusRead == 'Y'):
                    return
                flags += "normal"
                # return; # we ignore empty status reads?
            return AnalyzerFrame('readReg', self.startTime, frame.end_time, {
                'registerNum': regNum,
                'registerName': self.getRegisterName(regNum),
                'flags': flags,
                'input_type': frame.type
            })
        # writer register
        if (self.command >> 5) == 1:
            regNum = self.command & 0x1F
            flags = ""
            if regNum == 0x00 and self.data[0]&0b00000001 == 0:
                flags = "PTX "
            if regNum == 0x00 and self.data[0]&0b00000001 == 1:
                flags = "RTX "
            if regNum == 0x00 and self.data[0]&0b00000010 > 0:
                flags = "PwrUp "
            if regNum == 0x00 and self.data[0]&0b00000100 == 0b00000100:
                flags = "CRC-2bytes "
            if regNum == 0x00 and self.data[0]&0b00000100 == 0b00000000:
                flags = "CRC-1bytes "
            if regNum == 0x00 and self.data[0]&0b00001000 == 0b00001000:
                flags = "EN_CRC "
            if regNum == 0x00 and self.data[0]&0b00010000 == 0b00000000:
                flags = "MaxRT-IE "
            if regNum == 0x00 and self.data[0]&0b00100000 == 0b00000000:
                flags = "MaxTX-IE "
            if regNum == 0x00 and self.data[0]&0b01000000 == 0b00000000:
                flags = "MaxRX-IE "
            if regNum == 0x00 and self.data[0]&0b10000000 == 0b00000000:
                flags = "STB1 "
            if regNum == 0x00 and self.data[0]&0b10000000 == 0b10000000:
                flags = "STB3 "
            if regNum == 0x05:
                flags = "Ch " + str(self.data[0])
            if (regNum == 0x07) and (self.data[0] == 0x70):
                flags = "Clr-int"
            if (regNum == 0x03 and self.data[0] == 0x01):
                flags = "3 bytes"
            if (regNum == 0x03 and self.data[0] == 0x02):
                flags = "4 bytes"
            if (regNum == 0x03 and self.data[0] == 0x03):
                flags = "5 bytes"
            if (regNum == 0x011):
                flags = str(self.data[0]) +" bytes"
            if (regNum == 0x01C and self.data[0] == 0x00):
                flags = "disable"
            if (regNum == 0x06 and self.data[0]&0x1F == 0b00100111):
                flags = "11dBm"
            if (regNum == 0x06 and self.data[0]&0x1F == 0b00010101):
                flags = "9dBm"
            if (regNum == 0x06 and self.data[0]&0x1F == 0b00101100):
                flags = "5dBm"
            if (regNum == 0x06 and self.data[0]&0x1F == 0b00010100):
                flags = "4dBm"
            if (regNum == 0x06 and self.data[0]&0x1F == 0b00101010):
                flags = "-1dBm"
            if (regNum == 0x06 and self.data[0]&0x1F == 0b00011001):
                flags = "-10dBm"
            if (regNum == 0x06 and self.data[0]&0x1F == 0b00110000):
                flags = "-23dBm"
            if (regNum == 0x06 and self.data[0]&0xC0 == 0b01000000):
                flags = " 2MBit/s"
            if (regNum == 0x06 and self.data[0]&0xC0 == 0b00000000):
                flags = " 1MBit/s"
            if (regNum == 0x06 and self.data[0]&0xC0 == 0b11000000):
                flags = " 250KBit/s"
            return AnalyzerFrame('writeReg', self.startTime, frame.end_time, {
                'registerNum': regNum,
                'registerName': self.getRegisterName(regNum),
                'flags': flags
            })

        if (self.command == 0xFD):
            # CE on is not of interest
            return AnalyzerFrame('command', self.startTime, frame.end_time, {
                'command': 'CE_FSPI_ON'
            })
            #return
        
        if (self.command == 0xFC):
            # CE off is not of interest
            return AnalyzerFrame('command', self.startTime, frame.end_time, {
                'command': 'CE_FSPI_OFF'
            })
            #return

        if (self.command == 0x61):
            return AnalyzerFrame('command', self.startTime, frame.end_time, {
                'command': 'Read RX payload'
            })
        if (self.command == 0xE1):
            return AnalyzerFrame('command', self.startTime, frame.end_time, {
                'command': 'Flush TX fifo'
            })
        if (self.command == 0xE2):
            return AnalyzerFrame('command', self.startTime, frame.end_time, {
                'command': 'Flush RX fifo'
            })
        if (self.command == 0x53 and self.data[0] == 0x5A):
            return AnalyzerFrame('command', self.startTime, frame.end_time, {
                'command': 'goto reset'
            })
        if (self.command == 0x50 and self.data[0] == 0x73):
            return AnalyzerFrame('command', self.startTime, frame.end_time, {
                'command': 'activate'
            })
        if (self.command == 0x50 and self.data[0] == 0x8C):
            return AnalyzerFrame('command', self.startTime, frame.end_time, {
                'command': 'deactivate'
            })
        if (self.command == 0x53 and self.data[0] == 0xA5):
            return AnalyzerFrame('command', self.startTime, frame.end_time, {
                'command': 'release reset'
            })

        return AnalyzerFrame('command', self.startTime, frame.end_time, {
            'command': "Unknown command",
            'input_type': frame.type
        })
          



