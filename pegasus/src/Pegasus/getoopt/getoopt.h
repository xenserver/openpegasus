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
// Author: Bob Blair (bblair@bmc.com)
//
// Modified By: Carol Ann Krug Graves, Hewlett-Packard Company
//                  (carolann_graves@hp.com)
//              David Dillard, VERITAS Software Corp.
//                  (david.dillard@veritas.com)
//
//%/////////////////////////////////////////////////////////////////////////////


//
// Yet another attempt to create a general-purpose, object-oriented,
// portable C++ command line parser.
//
// There are two classes involved:
//    getoopt which encapsulates three functions:
//        1. Registration of program-defined flags and their characteristics
//        2. Parsing of the command line IAW the flag definitions
//        3. Orderly retrieval of the command line components
//
//    Optarg which abstracts the idea of a command line argument.
//
// The details of these classes are discussed in the comments above each
// class.
//

#ifndef _GETOOPT_H_
#define _GETOOPT_H_

#include <stdio.h>
#include <iostream>
#include <Pegasus/Common/ArrayInternal.h>
#include <Pegasus/Common/String.h>
#include <Pegasus/Common/Config.h>
#include <Pegasus/Common/Exception.h>
#include <Pegasus/getoopt/Linkage.h>

PEGASUS_USING_STD;
PEGASUS_USING_PEGASUS;

//
// This structure describes a program-defined command line option.
// The syntax of these options are the same as those understood by
// the standard C language routines getopt() and getopt_long()
//
// Command line options are named, and the name is prefaced by
// either a hyphen or by two hyphens.  Names prefixed by one
// hyphen are restricted to a length of one character.  There is
// no limit to the size of names prefixed by two hyphens.  The
// two-hyphen-named options are called "long" options.
//
// The argtype indicates whether the name should be bound to a value.
// If it never has a value, the type is 0.  If it always has a value,
// the type is 1.  If it can optionally have a value, the type is 2.
// Type 2 is valid only with long-named options
//
// The islong flag tells whether the option is long-named.
//
// The isactive flag tells whether the option should be considered
// during parsing.  It is on unless explicitly turned off by the program.
struct flagspec {
    String name;
    int    argtype;
    Boolean   islong;
    Boolean   active;
};

//
// Class Optarg encapsulates a command line argument as it was parsed.
// If it has a name, it means that it is bound to that command line option.
// For example, if the command line were
//      myprog --database=xyz mytable
// then the name "database" would be associated with the first argument.
// There would be no name associated with the second argument ("mytable").
//
// In the example above, the value property of the arguments would
// be "xyz" bound to the name "database" and "mytable" bound to a
// blank name.
//
// The option type further describes the binding:
//   A FLAG means that the value is bound to a short-named option name (flag)
//   A LONGFLAG means that the value is bound to a long-named option name
//   REGULAR means that the argument value is not preceded by a flag
//
class PEGASUS_GETOOPT_LINKAGE Optarg
{
public:
    enum opttype {FLAG, LONGFLAG, REGULAR};

private:
    String _name;
    opttype _opttype;
    String _value;

public:
    // Constructors and Destructor.  Default copying is OK for this class.
    Optarg();
    Optarg(
        const String& name,
        opttype type,
        const String& value);
    ~Optarg();

    // Methods to set or reset the properties
    void setName(const String& name);
    void setType(opttype type);
    void setValue(const String& value);

    // Methods to get information about the object
    const String& getName() const;
    const String& getopt() const;
    opttype getType() const;
    Boolean  isFlag() const;  // Is the opttype == "FLAG" or "LONGFLAG"?
    Boolean isLongFlag() const;  // IS the Opttype == LONGFLAG?
    const String& Value() const;  // return the value as a String
    const String& optarg() const; // ditto, in getopt() terminology
    void Value(String& v) const ; // Fill in a String with the Value
    // @exception TypeMismatchException
    void Value(int& v) const;  // Fill in an int with
                               // the value
    // @exception TypeMismatchException
    void Value(unsigned int& v) const;  // ditto an
                                        // unsigned int
    void Value(long& v) const ;   // ditto a long
    void Value(unsigned long& v) const;  // ditto an unsigned long
    void Value(double& d) const;  // ditto a double
    ostream& print(ostream& os) const;  // print the members (for debug)
};


//
//  class getoopt (a portamentau of "getopt" and "oo") is a container
//  for Optarg objects parsed from the command line, and it provides
//  methods for specifying command line options and initiating the
//  parse.
//
//  The idea is to be able to do getopt()-like things with it:
//      getoopt cmdline(optstring);
//      for (getoopt::const_iterator it = cmdline.begin();
//              it != cmdline.end();
//              it++)
//      {
//        . . . (process an Optarg represented by *it.)
//
//  There are three steps in using this class:
//    1. Initialization -- specifying the command line options
//       You can pass a String identical in format the an optstring
//       to the object either in the constructor or by an explicit method.
//       If you have long-named options to describe, use as manu
//       addLongFilespec() calls as you need.
//    2. Parse the command line.
//       This will almost always be cmdline.parse(argc, argv);
//       You can check for errors (violations of the command line
//       options specified) by calling hasErrors().  If you need
//       a description of the errors, they are stored in a
//       Array<String> which can be retrieved with getErrorStrings();
//       You can also print the error strings with printErrors();
//    3. Analyze the parsed data.  You can either iterate through the
//       the command line, or use indexes like this:
//       for (unsigned int i = cmdline.first(); i < cmdline.last(); i++)
//         . . . (process an Optarg represented by cmdline[i])
//
//       You can also look at the parsed data for named arguments
//       in an adhoc fashion by calling
//           isSet(flagName);
//       and
//           value(flagName);
//
class PEGASUS_GETOOPT_LINKAGE getoopt
{
public:
    typedef Array<flagspec> Flagspec_List;
    typedef Array<String>   Error_List;
    typedef Array<Optarg>   Arg_List;

    /**
        In the valid option definition string, following an option,
        indicates that the preceding option takes a required argument.
     */
    static const char GETOPT_ARGUMENT_DESIGNATOR;

private:
    Flagspec_List  _flagspecs;
    Error_List     _errorStrings;
    Arg_List       _args;
    flagspec* getFlagspecForUpdate(char c);
    flagspec* getFlagspecForUpdate(const String& s);
    String emptystring;
    Optarg _emptyopt;

public:
    enum argtype {NOARG, MUSTHAVEARG, OPTIONALARG};
    // Constructor and destructor.  You can initialize an instance with
    // an optstring to specify command line flags.
    getoopt(const char* optstring = 0);
    ~getoopt();

    // Routines for specifying the command line options
    //   add short-named flags, either en masse as an optstring
    Boolean addFlagspec(const String& opt);
    //   or individually
    Boolean addFlagspec(char opt, Boolean hasarg = false);
    //   (You can also remove a short flag specification if you need to)
    Boolean removeFlagspec(char opt);
    //   You can add long-named flags only individually
    Boolean addLongFlagspec(const String& name,  argtype type);
    //   and remove them in the same way.
    Boolean removeLongFlagspec(const String& name);
    // You can also get a pointer to the flagspec structure for
    // a particular flag, specifying a char for short or String for long name
    const flagspec* getFlagspec(char c);
    const flagspec* getFlagspec(const String& s);

    // Routines for initiating the parse and checking its success.
    Boolean parse(int argc, char** argv);
    Boolean hasErrors() const;
    const Error_List& getErrorStrings() const;
    ostream& printErrors(ostream& os) const;
    void printErrors(String& s) const;

    // Routines for processing the parsed command line
    //   Using indexes
    unsigned int size() const;  // The number of arguments found
    const Optarg& operator[](unsigned int n);  // The nth element
    unsigned int first() const;  // always 0 (duh)
    unsigned int last() const;   // always == size();
    //   Ad Hoc
    //        isSet returns the number of times a particular option appeared
    //        in the argument set.
    unsigned int  isSet(char opt) const;
    unsigned int  isSet(const String& opt) const;
    //        value returns the String value bount to the nth instance of
    //        the flag on the command line
    const String& value(char opt, unsigned int idx = 0) const;
    const String& value(const String& opt, unsigned int idx = 0) const;
    //   Still not flexible enough?  Here's an array of the results for
    //   your perusal.
    const Arg_List& getArgs() const;

    // Miscellanous methods
    //   You can add your own error to the error list if you want
    void  addError(const String& errstr);
    //   This method gives the number of named arguments (flags)
    //   size() - flagent() == number of nonflag arguments.
    unsigned int flagcnt() const;
};

inline int operator==(const Optarg& x, const Optarg& y)
{
    return 0;
}

inline int operator==(const flagspec& x, const flagspec& y)
{
    return 0;
}

#endif
