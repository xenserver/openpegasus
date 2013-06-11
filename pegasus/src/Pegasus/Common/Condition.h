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

#ifndef Pegasus_Condition_h
#define Pegasus_Condition_h

#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/Linkage.h>
#include <Pegasus/Common/Mutex.h>
#include <Pegasus/Common/AtomicInt.h>

#if defined(PEGASUS_HAVE_PTHREADS)
# include <pthread.h>
#elif defined(PEGASUS_HAVE_WINDOWS_THREADS)
# include <windows.h>
# include "List.h"
#else
# error "<Pegasus/Common/Condition.h>: not implemented"
#endif

PEGASUS_NAMESPACE_BEGIN

//==============================================================================
//
// ConditionRep
//
//==============================================================================

#if defined(PEGASUS_HAVE_PTHREADS)

struct ConditionRep
{
    pthread_cond_t cond;
};

#endif /* PEGASUS_HAVE_PTHREADS */

#if defined(PEGASUS_HAVE_WINDOWS_THREADS)

struct ConditionWaiter : public Linkable
{
    HANDLE event;
};

struct ConditionRep
{
    List<ConditionWaiter,Mutex> waiters;
};

#endif /* PEGASUS_HAVE_WINDOWS_THREADS */

//==============================================================================
//
// Condition
//
//==============================================================================

class PEGASUS_COMMON_LINKAGE Condition
{
public:

    Condition();

    ~Condition();

    void signal();

    void wait(Mutex& mutex);

private:
    Condition(const Condition&);
    Condition& operator=(const Condition&);
    ConditionRep _rep;
};

PEGASUS_NAMESPACE_END

#endif /* Pegasus_Condition_h */
