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

#include <Pegasus/Common/Config.h>
#include <cstdlib>
#include <cstdio>
#include <Pegasus/Common/SCMOStreamer.h>
#include <Pegasus/Common/ArrayIterator.h>
#include <Pegasus/Common/Tracer.h>

PEGASUS_USING_STD;

PEGASUS_NAMESPACE_BEGIN


#define PEGASUS_ARRAY_T SCMOResolutionTable
# include <Pegasus/Common/ArrayImpl.h>
#undef PEGASUS_ARRAY_T

SCMOStreamer::SCMOStreamer(CIMBuffer& out, Array<SCMOInstance>& x) :
    _buf(out),
    _scmoInstances(x),
    _ttlNumInstances(0),
    _ttlNumClasses(0)
{
};

// Writes a single SCMOClass to the given CIMBuffer
void SCMOStreamer::serializeClass(CIMBuffer& out, const SCMOClass& scmoClass)
{
    PEG_METHOD_ENTER(TRC_DISPATCHER,"SCMOStreamer::serializeClass");

    Array<SCMBClass_Main*> classTable;
    classTable.append(scmoClass.cls.hdr);

    _putClasses(out, classTable);

    PEG_METHOD_EXIT();
};

// Reads a single SCMOClass from the given CIMBuffer
bool SCMOStreamer::deserializeClass(CIMBuffer& in, SCMOClass& scmoClass)
{
    PEG_METHOD_ENTER(TRC_DISPATCHER,"SCMOStreamer::deserializeClass");

    Array<SCMBClass_Main*> classTable;
    if(!_getClasses(in, classTable))
    {
        PEG_TRACE_CSTRING(TRC_DISCARDED_DATA, Tracer::LEVEL1,
            "Failed to get Class!");
        PEG_METHOD_EXIT();
        return false;
    }

    if (classTable.size() > 0)
    {
        scmoClass = SCMOClass(classTable[0]);
    }

    PEG_METHOD_EXIT();
    return true;
};

// Writes the list of SCMOInstances stored in this instance of SCMOStreamer
// to the output buffer, including their referenced Classes and Instances
void SCMOStreamer::serialize()
{
    PEG_METHOD_ENTER(TRC_DISPATCHER,"SCMOStreamer::serialize");

    PEG_TRACE((TRC_DISPATCHER, Tracer::LEVEL3,
        "Serializing %d instances\n",
        _scmoInstances.size()));

    ConstArrayIterator<SCMOInstance> iterator(_scmoInstances);

    // First build the tables for references classes and instances
    for (Uint32 i = 0; i < iterator.size(); i++)
    {
        const SCMOInstance& inst = iterator[i];

        _appendToResolverTables(inst);
    }

    _putClasses(_buf,_classTable);

    _putInstances();

    //dumpTables();

    PEG_METHOD_EXIT();
}

// Reads a list of SCMOInstances from the input buffer and stores them in this
// instance of SCMOStreamer, including their referenced Classes and Instances
bool SCMOStreamer::deserialize()
{
    PEG_METHOD_ENTER(TRC_DISPATCHER,"SCMOStreamer::deserialize");

    if(!_getClasses(_buf,_classTable))
    {
        PEG_TRACE_CSTRING(TRC_DISCARDED_DATA, Tracer::LEVEL1,
            "Failed to get Classes!");
        PEG_METHOD_EXIT();
        return false;
    }

    if(!_getInstances())
    {
        PEG_TRACE_CSTRING(TRC_DISCARDED_DATA, Tracer::LEVEL1,
            "Failed to get Instances!");
        PEG_METHOD_EXIT();
        return false;
    }

    //dumpTables();

    PEG_METHOD_EXIT();
    return true;
}

// This function takes and instance and adds all of its external
// references (SCMOClass and SCMOInstances) to the index tables
// For referenced SCMOInstances, the function recusively calls itself.
//
// Returns the index position at which the class for this instance was inserted
// in the class resolver table.
Uint32 SCMOStreamer::_appendToResolverTables(const SCMOInstance& inst)
{
    // First handle the external references to other SCMOInstances
    Uint32 numExtRefs = inst.numberExtRef();
    for (Uint32 x=0; x < numExtRefs; x++)
    {
        SCMOInstance* extRef = inst.getExtRef(x);

        Uint32 idx = _appendToResolverTables(*extRef);
        _appendToInstResolverTable(*extRef,idx);
    }

    // Add the instance to the class resolution table
    // (Also adds the class to the class table when neccessary)
    return _appendToClassResolverTable(inst);
}


// This function adds the given instance to the instance resolver table,
// which stores the connections between instances and their external
// references.
//
// Returns the index position at which the instance was inserted in the
// instance resolver table.
Uint32 SCMOStreamer::_appendToInstResolverTable(
    const SCMOInstance& inst,
    Uint32 idx)
{
    SCMOResolutionTable tableEntry = { (Uint64)(void*)&inst, idx};
    _instResolverTable.append(tableEntry);

    // The number of elements in the array minus 1 is the index position
    // at which the instance was added.
    return _instResolverTable.size() - 1;
}


// Adds an instance to the class resolution table.
// Also adds the class to the class table when neccessary
//
// Returns the index position at which the instance was inserted in the
// instance resolver table.
Uint32 SCMOStreamer::_appendToClassResolverTable(const SCMOInstance& inst)
{
    // Add the SCMOClass for this instance to the class table,
    // or give us the index at which the class resides in the table
    Uint32 clsIdx = _appendToClassTable(inst);


    // Now build a new entry for the class resolution table
    SCMOResolutionTable tableEntry = {(Uint64)(void*)inst.inst.hdr, clsIdx};
    _clsResolverTable.append(tableEntry);

    // The number of elements in the array minus 1 is the index position
    // at which the instance was added.
    return _clsResolverTable.size() - 1;
}


// Add the SCMOClass for the given instance to the class table.
// If the class already exists in the table, it returns the index position
// for this class, otherwise appends the class at the end of the table
// and returns the new index position of the class.
Uint32 SCMOStreamer::_appendToClassTable(const SCMOInstance& inst)
{
    Uint32 clsTableSize = _classTable.size();
    SCMBClass_Main* clsPtr = inst.inst.hdr->theClass.ptr->cls.hdr;

    const SCMBClass_Main* const* clsArray = _classTable.getData();

    // Search through the table for the index of the class
    for (Uint32 x=0; x < clsTableSize; x++)
    {
        if (clsArray[x] == clsPtr)
        {
            return x;
        }
    }

    // Class is not yet listed in the table, therefore append it at the end ...
    _classTable.append(clsPtr);

    // ... and return the new size off the array minus one as the index
    return _classTable.size()-1;
}

#ifdef PEGASUS_DEBUG
void SCMOStreamer::_dumpTables()
{
    fprintf(stderr,"=====================================================\n");
    fprintf(stderr,"Dump of SCMOStreamer Tables:\n");
    fprintf(stderr,"CLASSES:\n");
    for (Uint32 x=0; x < _classTable.size(); x++)
    {
        fprintf(stderr,"\t[%2d] %p %s\n",x, _classTable[x],
                _getCharString(_classTable[x]->className,
                               (const char*)_classTable[x]));
    }

    fprintf(stderr,"INSTANCES:\n");
    for (Uint32 x=0; x < _clsResolverTable.size(); x++)
    {
        fprintf(stderr,"\t[%2d] I = %llx - cls = %2lld\n",
                x,
                _clsResolverTable[x].scmbptr,
                _clsResolverTable[x].index);
    }

    fprintf(stderr,"INSTANCE REFERENCES:\n");
    for (Uint32 x=0; x < _instResolverTable.size(); x++)
    {
        fprintf(stderr,"\t[%2d] R = %llx - I = %2lld\n",
                x,
                _instResolverTable[x].scmbptr,
                _instResolverTable[x].index);
    }
    fprintf(stderr,"=====================================================\n");
}
#endif


// Adds the list of SCMOClasses from the ClassTable to the output buffer
void SCMOStreamer::_putClasses(
    CIMBuffer& out,
    Array<SCMBClass_Main*>& classTable)
{
    Uint32 numClasses = classTable.size();
    const SCMBClass_Main* const* clsArray = classTable.getData();

    // Number of classes
    out.putUint32(numClasses);

    // SCMOClasses, one by one
    for (Uint32 x=0; x < numClasses; x++)
    {
        // Calculate the in-use size of the SCMOClass data
        Uint64 size =
            clsArray[x]->header.totalSize - clsArray[x]->header.freeBytes;
        out.putUint64(size);

        // Write class data
        out.putBytes(clsArray[x],(size_t)size);
    }

}

// Reads a list of SCMOClasses from the input buffer
bool SCMOStreamer::_getClasses(
    CIMBuffer& in,
    Array<SCMBClass_Main*>& classTable)
{
    // Number of classes
    Uint32 numClasses;
    if(! in.getUint32(numClasses) )
    {
        return false;
    }

    // SCMOClasses, one by one
    for (Uint32 x=0; x < numClasses; x++)
    {
        Uint64 size;
        if (!in.getUint64(size))
        {
            return false;
        }

        // Read class data
        SCMBClass_Main* scmbClassPtr = (SCMBClass_Main*)malloc((size_t)size);
        if (0 == scmbClassPtr)
        {
            // Not enough memory!
            throw PEGASUS_STD(bad_alloc)();
        }

        if (!in.getBytes(scmbClassPtr,(size_t)size))
        {
            return false;
        }


        // Resolve the class
        scmbClassPtr->header.totalSize = size;
        scmbClassPtr->header.freeBytes = 0;
        scmbClassPtr->refCount.set(0);

        classTable.append(scmbClassPtr);
    }

    return true;
}

// Adds the list of SCMO Instances from the _clsResolverTable to the
// output buffer
void SCMOStreamer::_putInstances()
{
    Uint32 numInst = _clsResolverTable.size();
    const SCMOResolutionTable* instArray = _clsResolverTable.getData();

    // Number of instances
    _buf.putUint32(numInst);

    // Instance to class resolution table
    _buf.putBytes(instArray, numInst*sizeof(SCMOResolutionTable));


    Uint32 numExtRefs = _instResolverTable.size();
    const SCMOResolutionTable* extRefArray = _instResolverTable.getData();

    // Number of references
    _buf.putUint32(numExtRefs);

    // Instance references resolution table
    _buf.putBytes(extRefArray, numExtRefs*sizeof(SCMOResolutionTable));


    // SCMOInstances, one by one
    for (Uint32 x=0; x < numInst; x++)
    {
        // Calculate the in-use size of the SCMOInstance data
        SCMBInstance_Main* instPtr = (SCMBInstance_Main*)instArray[x].scmbptr;
        Uint64 size = instPtr->header.totalSize - instPtr->header.freeBytes;
        _buf.putUint64(size);

        // Write class data
        _buf.putBytes(instPtr,(size_t)size);
    }
}


// Reads a list of SCMO Instances from the input buffer.
// Resolves the pointers to the SCMOClass and external references via the
// Class and Instance resolver tables.
bool SCMOStreamer::_getInstances()
{
    // Number of instances
    Uint32 numInst;
    if(!_buf.getUint32(numInst))
    {
        return false;
    }

    // Instance to class resolution table
    SCMOResolutionTable *instArray = new SCMOResolutionTable[numInst];
    if(!_buf.getBytes(instArray, numInst*sizeof(SCMOResolutionTable)))
    {
        return false;
    }

    // Number of references
    Uint32 numExtRefs;
    if(!_buf.getUint32(numExtRefs))
    {
        return false;
    }

    // Instance references resolution table
    SCMOResolutionTable *extRefArray = new SCMOResolutionTable[numExtRefs];
    Uint32 extRefIndex=0;
    if (numExtRefs > 0)
    {
        if(!_buf.getBytes(extRefArray, numExtRefs*sizeof(SCMOResolutionTable)))
        {
            return false;
        }
    }

    // Use simple array for access to class pointers
    const SCMBClass_Main* const* clsArray = _classTable.getData();

    // SCMOInstances, one by one
    for (Uint32 x=0; x < numInst; x++)
    {
        Uint64 size;
        if(!_buf.getUint64(size))
        {
            return false;
        }

        // Reserve 64 bytes more of storage to allow for hostname and namespace
        // updates without reallocation

        // Read instance data
        SCMBInstance_Main* scmbInstPtr =
            (SCMBInstance_Main*)malloc((size_t)size+64);
        if (0 == scmbInstPtr)
        {
            // Not enough memory!
            throw PEGASUS_STD(bad_alloc)();
        }

        if(!_buf.getBytes(scmbInstPtr,(size_t)size))
        {
            return false;
        }

        // Resolve the instance
        scmbInstPtr->header.totalSize = size+64;
        scmbInstPtr->header.freeBytes = 64;
        scmbInstPtr->refCount.set(0);
        scmbInstPtr->theClass.ptr =
             new SCMOClass((SCMBClass_Main*)clsArray[instArray[x].index]);

        SCMOInstance* scmoInstPtr = new SCMOInstance(scmbInstPtr);

        if (numExtRefs > 0)
        {
            // Handle the external references to other SCMOInstances
            Uint32 numExtRefs = scmoInstPtr->numberExtRef();
            for (Uint32 i=0; i < numExtRefs; i++)
            {
                Uint32 extRefPos = extRefArray[extRefIndex].index;
                SCMOInstance* extRefPtr =
                    (SCMOInstance*)instArray[extRefPos].scmbptr;
                scmoInstPtr->putExtRef(i,extRefPtr);

                // Mark instance as already consumed
                instArray[extRefPos].scmbptr = 0;

                extRefIndex++;
            }
        }

        instArray[x].scmbptr = (Uint64)(void*)scmoInstPtr;

#ifdef PEGASUS_DEBUG
        _clsResolverTable.append(instArray[x]);
#endif
    }

    // Append all non-referenced instances to output array
    for (Uint32 x=0; x < numInst; x++)
    {
        if (instArray[x].scmbptr)
        {
            _scmoInstances.append(*((SCMOInstance*)instArray[x].scmbptr));
            delete (SCMOInstance*)instArray[x].scmbptr;
        }
    }
    delete [] instArray;
    delete [] extRefArray;

    return true;
}

PEGASUS_NAMESPACE_END

