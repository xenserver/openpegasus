//%LICENSE////////////////////////////////////////////////////////////////
//
// Licensed to The Open Group (TOG) under one or more contributor license
// agreements.  Refer to the OpenPegasusNOTICE.txt file distributed with
// this work for additional information regarding copyright ownership.
// Each contributor licenses this file to you under the OpenPegasus Open
// Source License; you may not use this file except in compliance with the
// License.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////////
//
//%/////////////////////////////////////////////////////////////////////////////

#include <Pegasus/Common/FileSystem.h>
#include <Pegasus/Common/Tracer.h>
#include <Pegasus/Common/TraceFileHandler.h>
#include <Pegasus/Common/Logger.h>

#if defined(PEGASUS_OS_TYPE_WINDOWS)
# include <Pegasus/Common/TraceFileHandlerWindows.cpp>
#elif defined(PEGASUS_OS_TYPE_UNIX) || defined(PEGASUS_OS_VMS)
# include <Pegasus/Common/TraceFileHandlerPOSIX.cpp>
#else
# error "Unsupported platform"
#endif


PEGASUS_USING_STD;

PEGASUS_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
//  Constructs TraceFileHandler
////////////////////////////////////////////////////////////////////////////////

TraceFileHandler::TraceFileHandler()
{
    _fileName = 0;
    _fileHandle = 0;
    _logErrorBitField = 0;
    _configHasChanged = true;
#ifdef PEGASUS_PLATFORM_LINUX_GENERIC_GNU
    _baseFileName = 0;
    _fileCount = 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////
//  Destructs TraceFileHandler
////////////////////////////////////////////////////////////////////////////////

TraceFileHandler::~TraceFileHandler()
{
    // Close the File
    if (_fileHandle)
    {
        fclose(_fileHandle);
    }
    free(_fileName);
#ifdef PEGASUS_PLATFORM_LINUX_GENERIC_GNU
    free(_baseFileName);
#endif
}

////////////////////////////////////////////////////////////////////////////////
// The configuration of the trace has been updated.
// At the next trace write, change to the new configuration.
////////////////////////////////////////////////////////////////////////////////
void TraceFileHandler::configurationUpdated()
{
    _configHasChanged = true;
}

////////////////////////////////////////////////////////////////////////////////
// If the trace configuration has been updated,
// close the old file and open the new file.
////////////////////////////////////////////////////////////////////////////////
void TraceFileHandler::_reConfigure()
{
    AutoMutex writeLock(writeMutex);

    if(!_configHasChanged)
    {
        // An other thread does already the re-configuration.
        // do nothing.
        return;
    }

    free(_fileName);
    _fileName = 0;
#ifdef PEGASUS_PLATFORM_LINUX_GENERIC_GNU
    free(_baseFileName);
    _baseFileName = 0;
#endif

    if (Tracer::_getInstance() ->_traceFile.size() == 0)
    {
        // if the file name is empty/NULL pointer do nothing
        // wait for a new trace file.
        _configHasChanged=false;
        return;
    }

    _fileName = strdup((const char*)Tracer::_getInstance()
                            ->_traceFile.getCString());

    // If a file is already open, close it.
    if (_fileHandle)
    {
        fclose(_fileHandle);
        _fileHandle = 0;
    }

    _fileHandle = _openFile(_fileName);
    if (!_fileHandle)
    {
        // return with no message. _openFile() already wrote one.
        free(_fileName);
        _fileName = 0;
        // wait for a new trace file
        _configHasChanged=false;
        return;
    }

    #ifdef PEGASUS_PLATFORM_LINUX_GENERIC_GNU
    _baseFileName = strdup(_fileName);
    #endif

    _configHasChanged=false;

    return;
}

FILE* TraceFileHandler::_openFile(const char* fileName)
{
#ifdef PEGASUS_OS_VMS
//    FILE* fileHandle = fopen(fileName,"a+", "shr=get,put,upd");
    FILE* fileHandle = fopen(fileName,"w", "shr=get,put,upd");
#else
    FILE* fileHandle = fopen(fileName,"a+");
#endif
    if (!fileHandle)
    {
        // Unable to open file, log a message
        MessageLoaderParms parm(
            "Common.TraceFileHandler.FAILED_TO_OPEN_FILE_SYSMSG",
            "Failed to open file $0: $1",
            fileName,PEGASUS_SYSTEM_ERRORMSG_NLS);
        _logError(TRCFH_FAILED_TO_OPEN_FILE_SYSMSG,parm);
        return 0;
    }

    //
    // Verify that the file has the correct owner
    //
    if (!System::verifyFileOwnership(fileName))
    {
        MessageLoaderParms parm(
            "Common.TraceFileHandler.UNEXPECTED_FILE_OWNER",
            "File $0 is not owned by user $1.",
            fileName,
            System::getEffectiveUserName());
        _logError(TRCFH_UNEXPECTED_FILE_OWNER,parm);
        fclose(fileHandle);
        return 0;
    }

    //
    // Set the file permissions to 0600
    //
#if !defined(PEGASUS_OS_TYPE_WINDOWS)
    if (!FileSystem::changeFilePermissions(
            String(fileName), (S_IRUSR|S_IWUSR)) )
#else
    if (!FileSystem::changeFilePermissions(
            String(fileName), (_S_IREAD|_S_IWRITE)) )
#endif
    {
        MessageLoaderParms parm(
            "Common.TraceFileHandler.FAILED_TO_SET_FILE_PERMISSIONS",
            "Failed to set permissions on file $0",
            fileName);
        _logError(TRCFH_FAILED_TO_SET_FILE_PERMISSIONS,parm);
        fclose(fileHandle);
        return 0;
    }

    return fileHandle;
}

void TraceFileHandler::_logError(
    ErrLogMessageIds msgID,
    const MessageLoaderParms & parms)
{
    // msgID has to be within range, else we have a severe coding error
    PEGASUS_ASSERT((msgID >= TRCFH_FAILED_TO_OPEN_FILE_SYSMSG) &&
        (msgID <= TRCFH_INVALID_FILE_HANDLE));

    if ((_logErrorBitField & (1 << msgID)) == 0)
    {
        // log message not yet written, write log message
        Logger::put_l(
            Logger::ERROR_LOG,
            System::CIMSERVER,
            Logger::WARNING,
            parms);        
        // mark bit in log error field to flag that specific log message
        // has been written
        _logErrorBitField |= (1 << msgID);
    }
}


PEGASUS_NAMESPACE_END
