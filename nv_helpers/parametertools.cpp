/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "parametertools.hpp"
#include "nvprint.hpp"

#include <algorithm>

namespace nv_helpers {

  ParameterList::ParameterList()
  {
    ParameterHelpCallback* helpCallback = new ParameterHelpCallback(this);
    setHelp(add("help", helpCallback), "Print help");
    setHelp(add("h", helpCallback), "Print help");
  }

  void ParameterList::tokenizeString(std::string& content, std::vector<const char*> &args)
  {
    bool wasSpace = true;
    bool inQuotes = false;
    bool inComment = false;
    bool wasQuote = false;
    bool isEscape = false;
    bool wasEscape = false;

    for (size_t i = 0; i < content.size(); i++) {
      char* token = &content[i];
      char current = content[i];
      bool isEndline = current == '\n';
      bool isSpace = (current == ' ' || current == '\t' || current == '\n');
      bool isQuote = current == '"';
      bool isComment = current == '#';
      bool isEscape = current == '\\';

      if (isEndline && inComment) {
        inComment = false;
      }
      if (isComment && !inQuotes) {
        content[i] = 0;
        inComment = true;
      }

      if (inComment) continue;

      if (inQuotes) {
        if (wasEscape && (current == 'n' || current == 't')) {
          content[i] = current == 'n' ? '\n' : '\t';
          content[i - 1] = ' ';
        }
      }

      if (isQuote) {
        inQuotes = !inQuotes;
        // treat as space
        content[i] = 0;
        isSpace = true;
      }
      else if (isSpace) {
        // turn space to a terminator
        if (!inQuotes) {
          content[i] = 0;
        }
      }
      else if (wasSpace && (!inQuotes || wasQuote)) {
        // start a new arg unless comment
        args.push_back(token);
      }

      wasSpace = isSpace;
      wasQuote = isQuote;
      wasEscape = isEscape;
    }
  }

  ParameterList::Parameter::Parameter(Type atype, const char* aname, Callback* acallback, void* adestination, uint32_t areadLength, uint32_t awriteLength)
  {
    type = atype;
    callback = acallback;
    readLength = areadLength;
    writeLength = awriteLength;
    destination.ptr = adestination;

    // Set name and if specified, helptext
    // Split at delimiter '|'
    std::string sname = std::string(aname);
    size_t delimiterPos = sname.find_first_of('|');
    if (delimiterPos != std::string::npos)
    {
      name = sname.substr(0, delimiterPos);
      helptext = sname.substr(delimiterPos+1);
    }
    else 
    {
      name = sname;
      helptext = "";
    }
  }

  uint32_t ParameterList::append(const ParameterList& list)
  {
    uint32_t index = uint32_t(m_parameters.size());
    m_parameters.insert(m_parameters.end(), list.m_parameters.begin(), list.m_parameters.end());
    return index;
  }

  uint32_t ParameterList::add(const char* name, float* destination, Callback* callback, uint32_t length /*= 1*/, float min /*= -FLT_MAX*/, float max /*= FLT_MAX*/)
  {
    uint32_t index = uint32_t(m_parameters.size());
    Parameter param(TYPE_FLOAT, name, callback, destination, length, length);
    param.minmax[0].f32 = min;
    param.minmax[1].f32 = max;
    m_parameters.push_back(param);
    return index;
  }

  uint32_t ParameterList::add(const char* name, int32_t* destination, Callback* callback, uint32_t length /*= 1*/, int32_t min /*= -INT_MAX*/, int32_t max /*= +INT_MAX*/)
  {
    uint32_t index = uint32_t(m_parameters.size());
    Parameter param(TYPE_INT, name, callback, destination, length, length);
    param.minmax[0].s32 = min;
    param.minmax[1].s32 = max;
    m_parameters.push_back(param);
    return index;
  }

  uint32_t ParameterList::add(const char* name, uint32_t* destination, Callback* callback, uint32_t length /*= 1*/, uint32_t min /*= 0*/, uint32_t max /*= 0xFFFFFFFF*/)
  {
    uint32_t index = uint32_t(m_parameters.size());
    Parameter param(TYPE_UINT, name, callback, destination, length, length);
    param.minmax[0].u32 = min;
    param.minmax[1].u32 = max;
    m_parameters.push_back(param);
    return index;
  }

  uint32_t ParameterList::add(const char* name, bool* destination, Callback* callback, uint32_t length /*= 1*/)
  {
    uint32_t index = uint32_t(m_parameters.size());
    Parameter param(TYPE_BOOL, name, callback, destination, length, length);
    m_parameters.push_back(param);
    return index;
  }

  uint32_t ParameterList::add(const char* name, bool* destination, bool value, Callback* callback /*= nullptr*/, uint32_t length /*= 1*/)
  {
    uint32_t index = uint32_t(m_parameters.size());
    Parameter param(TYPE_BOOL_VALUE, name, callback, destination, 0, length);
    param.minmax[0].b = value;
    m_parameters.push_back(param);
    return index;
  }

  uint32_t ParameterList::add(const char* name, std::string* destination, Callback* callback, uint32_t length)
  {
    uint32_t index = uint32_t(m_parameters.size());
    Parameter param(TYPE_STRING, name, callback, destination, length, length);
    m_parameters.push_back(param);
    return index;
  }

  uint32_t ParameterList::add(const char* name, Callback* callback, uint32_t length)
  {
    uint32_t index = uint32_t(m_parameters.size());
    Parameter param(TYPE_TRIGGER, name, callback, nullptr, length, length);
    m_parameters.push_back(param);
    return index;
  }

  uint32_t ParameterList::addFilename(const char* name, std::string* destination, Callback* callback)
  {
    uint32_t index = uint32_t(m_parameters.size());
    Parameter param(TYPE_FILENAME, name, callback, destination, 1, 1);
    m_parameters.push_back(param);
    return index;
  }

  uint32_t ParameterList::setHelp(uint32_t parameterIndex, const char* helptext) 
  {
    this->m_parameters[parameterIndex].helptext = helptext;
    return parameterIndex;
  }

  bool ParameterList::applyParameters(uint32_t argc, const char** argv, uint32_t &a, const char* paramPrefix /*= nullptr*/, const char* defaultFilePath /*= nullptr*/) const
  {
    std::string prefixStr(paramPrefix ? paramPrefix : "");
    std::string defaultPathStr(defaultFilePath ? defaultFilePath : "");

    for (uint32_t p = 0; p < uint32_t(m_parameters.size()); p++) {
      const Parameter& param = m_parameters[p];
      std::string combined = prefixStr + param.name;

      if (strcmp(argv[a], combined.c_str()) == 0 && a + param.readLength < argc) {
        switch (param.type) {
        case TYPE_FLOAT:
        {
          for (uint32_t i = 0; i < param.writeLength; i++) {
            param.destination.f32[i] = std::min(std::max(float(atof(argv[a + i + 1])), param.minmax[0].f32), param.minmax[1].f32);
          }
        }
        break;
        case TYPE_UINT:
        {
          for (uint32_t i = 0; i < param.writeLength; i++) {
            param.destination.u32[i] = std::min(std::max(uint32_t(atoi(argv[a + i + 1])), param.minmax[0].u32), param.minmax[1].u32);
          }
        }
        break;
        case TYPE_INT:
        {
          for (uint32_t i = 0; i < param.writeLength; i++) {
            param.destination.s32[i] = std::min(std::max(int32_t(atoi(argv[a + i + 1])), param.minmax[0].s32), param.minmax[1].s32);
          }
        }
        break;
        case TYPE_BOOL:
        {
          for (uint32_t i = 0; i < param.writeLength; i++) {
            param.destination.b[i] = atoi(argv[a + i + 1]) != 0;
          }
        }
        break;
        case TYPE_BOOL_VALUE:
        {
          for (uint32_t i = 0; i < param.writeLength; i++) {
            param.destination.b[i] = param.minmax[0].b;
          }
        }
        break;
        case TYPE_STRING:
        {
          for (uint32_t i = 0; i < param.writeLength; i++) {
            param.destination.str[i] = std::string(argv[a + i + 1]);
          }
        }
        break;
        case TYPE_FILENAME:
        {
          std::string filename(argv[a + 1]);

          if (
#ifdef _WIN32
            filename.find(':') != std::string::npos
#else
            !filename.empty() && filename[0] == '/'
#endif
            )
          {
            param.destination.str[0] = filename;
          }
          else {
            param.destination.str[0] = defaultPathStr + "/" + filename;
          }
        }
        break;
        case TYPE_TRIGGER:
        {

        }
        }

        if (param.callback) {
          param.callback->parameterCallback(p);
        }

        a += param.readLength;
        return true;
      }
    }
    return false;
  }

  const char* ParameterList::toString(Type typ)
  {
    switch (typ) {
    case TYPE_FLOAT:
      return "float   ";
    case TYPE_INT:
      return "int     ";
    case TYPE_UINT:
      return "uint    ";
    case TYPE_BOOL:
      return "bool    ";
    case TYPE_BOOL_VALUE:
      return "value   ";
    case TYPE_STRING:
      return "string  ";
    case TYPE_FILENAME:
      return "filename";
    case TYPE_TRIGGER:
      return "trigger ";
    }

    return "unknown";
  }

  void ParameterList::print() const
  {
    // Get maximum parameter name length
    size_t maxParamNameLength = 0;
    for(const auto& it : m_parameters)
    {
      maxParamNameLength = std::max(it.name.size(), maxParamNameLength);
    }

    // Print header
    LOGI("parameterlist:\n");
    LOGI(" type [args] %-*s   helptext\n", maxParamNameLength, "helptext");  // Pad helptext column with blanks
    
    // Print underline. Format: -----
    LOGI(" ");
    for (int i = 0; i < maxParamNameLength + 23; i++)
    {
      LOGI("-");
    }
    LOGI("\n");

    // Print command line arguments
    for (const auto& it : m_parameters) {

      // Log param type, [args], name
      LOGI(" %s[%d] %-*s", toString(it.type), it.readLength, maxParamNameLength, it.name.c_str());
      if(it.helptext != "")
      {
        // Log helptext
        LOGI(" - %s", it.helptext.c_str());
      }
      // Newline
      LOGI("\n");
    }
    LOGI("\n");
  }

  uint32_t ParameterList::applyTokens(uint32_t argc, const char** argv, const char* prefix, const char* defaultPath) const
  {
    uint32_t found = 0;

    for (uint32_t a = 0; a < argc; a++) {
      if (applyParameters(argc, argv, a, prefix, defaultPath)) {
        found++;
      }
    }

    return found;
  }

  bool ParameterSequence::advanceIteration(const char* separator, uint32_t separatorArgLength, uint32_t &argBegin, uint32_t &argCount)
  {
    if (!m_list || m_index >= m_tokens.size()) return true;

    size_t begin = m_index;
    size_t end = begin;

    m_separator = ~0;

    for (size_t i = m_index; i < m_tokens.size(); i++)
    {
      if (strcmp(m_tokens[i], separator) == 0 && i + separatorArgLength < m_tokens.size()) {
        end = i - 1;
        m_separator = i;
        m_index = i + separatorArgLength + 1;
        break;
      }
      end = i;
    }

    uint32_t count = uint32_t(1 + end - begin);
    if (count)
    {
      argCount = count;
      argBegin = uint32_t(begin);

      m_iteration++;
      return false;
    }
    else {
      return true;
    }
  }

  bool ParameterSequence::applyIteration(const char* separator, uint32_t separatorLength, const char* paramPrefix /*= nullptr*/, const char* defaultFilePath /*= nullptr*/)
  {
    uint32_t argBegin;
    uint32_t argCount;
    if (!advanceIteration(separator, separatorLength, argBegin, argCount)) {
      m_list->applyTokens(argCount, (const char**)&m_tokens[argBegin], paramPrefix, defaultFilePath);
      return false;
    }
    else {
      return true;
    }
  }

  void ParameterSequence::resetIteration()
  {
    m_index = 0;
    m_iteration = 0;
  }

  ParameterHelpCallback::ParameterHelpCallback(ParameterList* parameterList)
      : m_parameterList(parameterList)
  {}
  
  void ParameterHelpCallback::parameterCallback(uint32_t param)
  {
    m_parameterList->print();
  }
}
