/*
Copyright (c) 2013, Silas Parker
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    The name of Silas Parker may not be used to endorse or promote products
    derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "serial.hpp"

Serial::Serial() : com_port(INVALID_HANDLE_VALUE)
{}

Serial::~Serial()
{
  close();
}

bool Serial::is_valid() const
{
  return com_port != INVALID_HANDLE_VALUE;
}

bool Serial::open(const std::string& name, std::string& errmsg)
{
  bool retval = false;
  DCB config;
  
  errmsg.clear();
  
  // If port already open, ignore call
  if (com_port != INVALID_HANDLE_VALUE)
  {
    errmsg = "COM port already open";
    goto exit; 
  }
  
  // Open COM port
  com_port = CreateFile(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL,
    OPEN_EXISTING, 0, NULL);
  if (com_port == INVALID_HANDLE_VALUE)
  {
    errmsg = "Can not open COM port";
    goto exit;
  }
  
  // Get current config
  config.DCBlength = sizeof(config);
  if (!GetCommState(com_port, &config))
  {
    errmsg = "Can not get COM port config";
    goto exit;
  }
  
  // Set defaults
  config.BaudRate = CBR_115200;
  config.StopBits = ONESTOPBIT;
  config.Parity =   NOPARITY;
  config.ByteSize = 8;
  
  // Set config
  if (!SetCommState(com_port, &config))
  {
    errmsg = "Can not set COM port config";
    goto exit;
  }
  
  retval = true;

exit:
  if (retval == false)
  {
    close();
  }
  return retval;
}

void Serial::close()
{
  if (com_port != INVALID_HANDLE_VALUE)  
  {
    CloseHandle(com_port);
    com_port = INVALID_HANDLE_VALUE;
  }
}

void Serial::write(const unsigned char* data, unsigned length)
{
  if (data == 0 || length == 0)
    return;

  if (com_port != INVALID_HANDLE_VALUE)  
  {
    unsigned long wrote;
    WriteFile(com_port, data, length, &wrote, NULL);
    FlushFileBuffers(com_port);
  }
}




