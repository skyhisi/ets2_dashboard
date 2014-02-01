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

#include "options.hpp"

#include <fstream>
#include <sstream>

Options::Options() :
  mOptionsFloat(),
  mOptionsInt(),
  mOptionsString()
{
}

bool Options::read_file(const std::string& path)
{
  std::ifstream file(path);
  if (!file.is_open())
    return false;
  
  while (file.good())
  {
    std::string line;
    std::getline(file, line);
    parse_option_line(line);
  }
  return true;
}

float Options::get_option_float(const std::string& key, float def) const
{
  const float_map::const_iterator it = mOptionsFloat.find(key);
  return (it == mOptionsFloat.end()) ? def : it->second;
}

int Options::get_option_int(const std::string& key, int def) const
{
  const int_map::const_iterator it = mOptionsInt.find(key);
  return (it == mOptionsInt.end()) ? def : it->second;
}

const std::string& Options::get_option_string(const std::string& key, const std::string& def) const
{
  const string_map::const_iterator it = mOptionsString.find(key);
  return (it == mOptionsString.end()) ? def : it->second;
}


void Options::parse_option_line(const std::string& line)
{
  std::string key;
  std::string type;
  std::stringstream ss(line);
  
  ss >> key >> type;
  if (type == "flt")
  {
    float val;
    ss >> val;
    mOptionsFloat[key] = val;
  }
  else if (type == "int")
  {
    int val;
    ss >> val;
    mOptionsInt[key] = val;
  }
  else if (type == "str")
  {
    std::string val;
    ss >> val;
    mOptionsString[key] = val;
  }
}

