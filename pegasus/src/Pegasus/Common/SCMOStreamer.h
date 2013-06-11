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

#ifndef Pegasus_SCMOStreamer_h
#define Pegasus_SCMOStreamer_h

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/ArrayInternal.h>
#include <Pegasus/Common/SCMO.h>
#include <Pegasus/Common/SCMOInstance.h>
#include <Pegasus/Common/CIMBuffer.h>
#include <Pegasus/Common/Array.h>

PEGASUS_NAMESPACE_BEGIN


struct SCMOResolutionTable
{
    // Though we really store a pointer here, it is stored as Uint64 to
    // become independent from 64bit versus 32bit incarnations of the struct.
    Uint64 scmbptr;
    Uint64 index;
};

#define PEGASUS_ARRAY_T SCMOResolutionTable
# include <Pegasus/Common/ArrayInter.h>
#undef PEGASUS_ARRAY_T


class PEGASUS_COMMON_LINKAGE SCMOStreamer
{
public:

    SCMOStreamer(CIMBuffer& out, Array<SCMOInstance>& x);

    // Writes the list of SCMOInstances stored in this instance of SCMOStreamer
    // to the output buffer, including their referenced Classes and Instances
    void serialize();

    // Reads a list of SCMOInstances from the input buffer and stores them in
    // this instance of SCMOStreamer, including their referenced Classes and
    // Instances
    bool deserialize();

    // Writes a single SCMOClass to the given CIMBuffer
    static void serializeClass(CIMBuffer& out, const SCMOClass& scmoClass);

    // Reads a single SCMOClass from the given CIMBuffer
    static bool deserializeClass(CIMBuffer& in, SCMOClass& scmoClass);

private:



    // This function takes and instance and adds all of its external
    // references (SCMOClass and SCMOInstances) to the index tables
    // For referenced SCMOInstances, the function recusively calls itself.
    //
    // Returns the index position at which the class for this instance was
    // inserted in the class resolver table.
    Uint32 _appendToResolverTables(const SCMOInstance& inst);


    // This function adds the given instance to the instance resolver table,
    // which stores the connections between instances and their external
    // references.
    //
    // Returns the index position at which the instance was inserted in the
    // instance resolver table.
    Uint32 _appendToInstResolverTable(const SCMOInstance& inst, Uint32 idx);


    // Adds an instance to the class resolution table.
    // Also adds the class to the class table when neccessary
    //
    // Returns the index position at which the instance was inserted in the
    // instance resolver table.
    Uint32 _appendToClassResolverTable(const SCMOInstance& inst);


    // Add the SCMOClass for the given instance to the class table.
    // If the class already exists in the table, it returns the index position
    // for this class, otherwise appends the class at the end of the table
    // and returns the new index position of the class.
    Uint32 _appendToClassTable(const SCMOInstance& inst);

    static void _putClasses(CIMBuffer& out,Array<SCMBClass_Main*>& classTable);
    static bool _getClasses(CIMBuffer& in,Array<SCMBClass_Main*>& classTable);
    void _putInstances();
    bool _getInstances();


#ifdef PEGASUS_DEBUG
    void _dumpTables();
#endif

    // The Buffer for output streaming
    CIMBuffer& _buf;

    // The array of SCMOInstances to be streamed
    Array<SCMOInstance>& _scmoInstances;

    // Counters for the total number of classes and scmo instances
    // to be streamed. These counters increase during streaming process.
    Uint32 _ttlNumInstances;
    Uint32 _ttlNumClasses;

    // Index table used to resolve the absolute pointers to SCMOClasses
    // to a relative sequence number (index) in the stream
    Array<SCMOResolutionTable> _clsResolverTable;

    // Index table used to resolve the absolute pointers to SCMOInstances
    // to a relative sequence number (index) in the stream
    Array<SCMOResolutionTable> _instResolverTable;

    // Table of pointers to SCMOClasses to be streamed
    Array<SCMBClass_Main*> _classTable;
};

PEGASUS_NAMESPACE_END

#endif /* Pegasus_SCMOStreamer_h */
