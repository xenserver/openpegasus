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
%{
  /* Flex grammar created from CIM Specification Version 2.2 Appendix A */

  /*
     Note the following implementation details:

       1. The MOF specification has a production of type assocDeclaration,
       but an association is just a type of classDeclaration with a few
       special rules.  At least for the first pass, I'm treating an
       associationDeclaration as a classDeclaration and applying its
       syntactical rules outside of the grammar definition.

       2. Same with the indicationDeclaration.  It appears to be a normal
       classDeclaration with the INDICATION qualifier and no special
       syntactical rules.

       3. The Parser uses String objects throughout to represent
       character data.  However, the tokenizer underneath is probably
       working with 8-bit chars.  If we later use an extended character
       compatible tokenizer, I anticipate NO CHANGE to this parser.

       4. Besides the tokenizer, this parser uses 2 sets of outside
       services:
          1)Class valueFactory.  This has a couple of static methods
      that assist in creating CIMValue objects from Strings.
      2)Class cimmofParser.  This has a wide variety of methods
      that fall into these catagories:
            a) Interfaces to the Repository.  You call cimmofParser::
            methods to query and store compiled CIM elements.
        b) Error handling.
            c) Data format conversions.
            d) Tokenizer manipulation
            e) Pragma handling
            f) Alias Handling
  */


#define YYSTACKSIZE 2000

#include <cstdlib>
#if !defined(PEGASUS_OS_ZOS) && !defined(PEGASUS_OS_VMS)
#if defined(PEGASUS_OS_DARWIN)
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#endif
#include <cstdio>
#include <cstring>
#include <Pegasus/Common/String.h>
#include <Pegasus/Common/CIMName.h>
#include <Pegasus/Common/StringConversion.h>
#include "cimmofParser.h"
#include "valueFactory.h"
#include "memobjs.h"
#include <Pegasus/Common/CIMQualifierList.h>
#include <Pegasus/Common/AutoPtr.h>

//include any useful debugging stuff here

/* Debugging the parser.  Debugging is provided through
   1. debug functions in Bison that are controlled by a compile time
      flag (YYDEBUG) and a runtime flag (yydebug) which is redefined
      to cimmof_debug.
   2. Debug functions defined through YACCTRACE, a macro defined
      in cimmofparser.h and turned on and off manually.
   All debugging must be turned on manually at this point by
   setting the YYDEBUG compile flag and also setting YACCTRACE.
   ATTN: TODO: automate the debug information flags.
*/
// Enable this define to compie Bison/Yacc tracing
// ATTN: p3 03092003 ks Enabling this flag currently causes a compile error

#define YYDEBUG 1
//static int cimmof_debug;

//extern cimmofParser g_cimmofParser;

extern int   cimmof_lex(void);
extern int   cimmof_error(...);
extern char *cimmof_text;
extern void  cimmof_yy_less(int n);
extern int   cimmof_leng;


/* ------------------------------------------------------------------- */
/* These globals provide continuity between various pieces of a        */
/* declaration.  They are usually interpreted as "these modifiers were */
/* encountered and need to be applied to the finished object".  For    */
/* example, qualifiers are accumulated in g_qualifierList[] as they    */
/* encountered, then applied to the production they qualify when it    */
/* is completed.                                                       */
/* ------------------------------------------------------------------- */
  CIMFlavor g_flavor = CIMFlavor (CIMFlavor::NONE);
  CIMScope g_scope = CIMScope ();
  //ATTN: BB 2001 BB P1 - Fixed size qualifier list max 20. Make larger or var
  CIMQualifierList g_qualifierList;
  CIMMethod *g_currentMethod = 0;
  CIMClass *g_currentClass = 0;
  CIMInstance *g_currentInstance = 0;
  String g_currentAliasRef; // Alias reference
  String g_currentAliasDecl; // Alias declaration
  CIMName g_referenceClassName = CIMName();
  Array<CIMKeyBinding> g_KeyBindingArray; // it gets created empty
  TYPED_INITIALIZER_VALUE g_typedInitializerValue;

/* ------------------------------------------------------------------- */
/* Pragmas, except for the Include pragma, are not handled yet    */
/* I don't understand them, so it may be a while before they are       */
/* ------------------------------------------------------------------- */
  struct pragma {
    String name;
    String value;
  };

/* ---------------------------------------------------------------- */
/* Use our general wrap manager to handle end-of-file               */
/* ---------------------------------------------------------------- */
extern "C" {
int
cimmof_wrap() {
  return cimmofParser::Instance()->wrapCurrentBuffer();
}
}

/* ---------------------------------------------------------------- */
/* Pass errors to our general log manager.                          */
/* ---------------------------------------------------------------- */
void
cimmof_error(const char *msg) {
  cimmofParser::Instance()->log_parse_error(cimmof_text, msg);
  // printf("Error: %s\n", msg);
}

static void MOF_error(const char * str, const char * S);
static void MOF_trace(const char* str);
static void MOF_trace2(const char * str, const char * S);

%}

%union {
  //char                     *strval;
  CIMClass                 *mofclass;
  CIMFlavor                *flavor;
  CIMInstance              *instance;
  CIMKeyBinding            *keybinding;
  CIMMethod                *method;
  CIMName                  *cimnameval;
  CIMObjectPath            *reference;
  CIMProperty              *property;
  CIMQualifier             *qualifier;
  CIMQualifierDecl         *mofqualifier;
  CIMScope                 *scope;
  CIMType                   datatype;
  CIMValue                 *value;
  int                       ival;
  CIMObjectPath            *modelpath;
  String                   *strptr;
  String                   *strval;
  struct pragma            *pragma;
  TYPED_INITIALIZER_VALUE  *typedinitializer;
}



%token TOK_ALIAS_IDENTIFIER
%token TOK_ANY
%token TOK_AS
%token TOK_ASSOCIATION
%token TOK_BINARY_VALUE
%token TOK_CHAR_VALUE
%token TOK_CLASS
%token TOK_COLON
%token TOK_COMMA
%token TOK_DISABLEOVERRIDE
%token TOK_DQUOTE
%token TOK_DT_BOOL
%token TOK_DT_CHAR16
%token TOK_DT_CHAR8
%token TOK_DT_DATETIME
%token TOK_DT_REAL32
%token TOK_DT_REAL64
%token TOK_DT_SINT16
%token TOK_DT_SINT32
%token TOK_DT_SINT64
%token TOK_DT_SINT8
%token TOK_DT_STR
%token TOK_DT_UINT16
%token TOK_DT_UINT32
%token TOK_DT_UINT64
%token TOK_DT_UINT8
%token TOK_ENABLEOVERRIDE
%token TOK_END_OF_FILE
%token TOK_EQUAL
%token TOK_FALSE
%token TOK_FLAVOR
%token TOK_HEX_VALUE
%token TOK_INCLUDE
%token TOK_INDICATION
%token TOK_INSTANCE
%token TOK_LEFTCURLYBRACE
%token TOK_LEFTPAREN
%token TOK_LEFTSQUAREBRACKET
%token TOK_METHOD
%token TOK_NULL_VALUE
%token TOK_OCTAL_VALUE
%token TOK_OF
%token TOK_PARAMETER
%token TOK_PERIOD
%token TOK_POSITIVE_DECIMAL_VALUE
%token TOK_PRAGMA
%token TOK_PROPERTY
%token TOK_QUALIFIER
%token TOK_REAL_VALUE
%token TOK_REF
%token TOK_REFERENCE
%token TOK_RESTRICTED
%token TOK_RIGHTCURLYBRACE
%token TOK_RIGHTPAREN
%token TOK_RIGHTSQUAREBRACKET
%token TOK_SCHEMA
%token TOK_SCOPE
%token TOK_SEMICOLON
%token TOK_SIGNED_DECIMAL_VALUE
%token TOK_SIMPLE_IDENTIFIER
%token TOK_STRING_VALUE
%token TOK_TOSUBCLASS
%token TOK_TRANSLATABLE
%token TOK_TRUE
%token TOK_UNEXPECTED_CHAR


%type <cimnameval>     propertyName parameterName methodName className
%type <cimnameval>     superClass
%type <datatype>       dataType intDataType realDataType parameterType objectRef
%type <flavor>         flavor defaultFlavor
%type <instance>       instanceHead instanceDeclaration
%type <ival>           array
%type <ival>           booleanValue keyValuePairList
%type <keybinding>     keyValuePair
%type <method>         methodHead methodDeclaration
%type <modelpath>      modelPath
%type <mofclass>       classHead classDeclaration
%type <mofqualifier>   qualifierDeclaration
%type <pragma>         compilerDirectivePragma
%type <property>       propertyBody propertyDeclaration referenceDeclaration
%type <qualifier>      qualifier
%type <scope>          scope metaElements metaElement
%type <strval>         arrayInitializer constantValues
%type <strval>         fileName referencedObject referenceName referencePath
%type <strval>         integerValue TOK_REAL_VALUE TOK_CHAR_VALUE
%type <strval>         namespaceHandle namespaceHandleRef
%type <strval>         nonNullConstantValue
%type <strval>         pragmaName pragmaVal keyValuePairName qualifierName
%type <strval>         referenceInitializer objectHandle
%type <strval>         aliasInitializer
%type <strval>         aliasIdentifier
%type <strval>         stringValue stringValues initializer constantValue
%type <strval>         TOK_ALIAS_IDENTIFIER  alias
%type <strval>         TOK_POSITIVE_DECIMAL_VALUE TOK_OCTAL_VALUE TOK_HEX_VALUE
%type <strval>         TOK_BINARY_VALUE TOK_SIGNED_DECIMAL_VALUE
%type <strval>         TOK_SIMPLE_IDENTIFIER TOK_STRING_VALUE
%type <strval>         TOK_UNEXPECTED_CHAR
%type <typedinitializer> typedInitializer typedDefaultValue
%type <typedinitializer> typedQualifierParameter
%type <value>          qualifierValue

%%

/*
**------------------------------------------------------------------------------
**
**   Production rules section
**
**------------------------------------------------------------------------------
*/
mofSpec: mofProductions ;


mofProductions: mofProduction mofProductions
              | /* empty */ ;


// ATTN: P1 KS Apr 2002 Limit in (none) Directive handling. See FIXME below.
mofProduction: compilerDirective { /* FIXME: Where do we put directives? */ }
    | qualifierDeclaration
        {
            cimmofParser::Instance()->addQualifier($1);
            delete $1;
        }
    | classDeclaration
        {
            cimmofParser::Instance()->addClass($1);
        }
    | instanceDeclaration
        {
        cimmofParser::Instance()->addInstance($1);
        } ;

/*
**------------------------------------------------------------------------------
**
**   class Declaration productions and processing
**
**------------------------------------------------------------------------------
*/
classDeclaration: classHead  classBody
{
    YACCTRACE("classDeclaration");
    if (g_currentAliasDecl != String::EMPTY)
        cimmofParser::Instance()->addClassAlias(g_currentAliasDecl, $$, false);
} ;


classHead: qualifierList TOK_CLASS className alias superClass
{
    // create new instance of class with className and superclassName
    // put returned class object on stack
    YACCTRACE("classHead:");
    $$ = cimmofParser::Instance()->newClassDecl(*$3, *$5);

    // put list of qualifiers into class
    applyQualifierList(g_qualifierList, *$$);

    g_currentAliasRef = *$4;
    if (g_currentClass)
        delete g_currentClass;
    g_currentClass = $$;
    delete $3;
    delete $4;
    delete $5;
} ;


className: TOK_SIMPLE_IDENTIFIER {} ;


superClass: TOK_COLON className
{
    $$ = new CIMName(*$2);
    delete $2;
}
| /* empty */
{
    $$ = new CIMName();
}


classBody: TOK_LEFTCURLYBRACE classFeatures TOK_RIGHTCURLYBRACE TOK_SEMICOLON
    | TOK_LEFTCURLYBRACE TOK_RIGHTCURLYBRACE TOK_SEMICOLON ;


classFeatures: classFeature
    | classFeatures classFeature ;


classFeature: propertyDeclaration
    {
        YACCTRACE("classFeature:applyProperty");
        cimmofParser::Instance()->applyProperty(*g_currentClass, *$1);
        delete $1;
    }
    | methodDeclaration
    {
        YACCTRACE("classFeature:applyMethod");
        cimmofParser::Instance()->applyMethod(*g_currentClass, *$1);
    }
    | referenceDeclaration
    {
        YACCTRACE("classFeature:applyProperty");
        cimmofParser::Instance()->applyProperty(*g_currentClass, *$1);
        delete $1;
    };






/*
**------------------------------------------------------------------------------
**
** method Declaration productions and processing.
**
**------------------------------------------------------------------------------
*/

methodDeclaration: qualifierList methodHead methodBody methodEnd
{
    YACCTRACE("methodDeclaration");
    $$ = $2;
} ;



// methodHead processes the datatype and methodName and puts qualifierList.
// Note that the qualifierList is parsed in methodDeclaration and applied here
// so that it is not overwritten by parameter qualifier lists.
methodHead: dataType methodName
{
    YACCTRACE("methodHead");
    if (g_currentMethod)
        delete g_currentMethod;

    // create new method instance with pointer to method name and datatype
    g_currentMethod = cimmofParser::Instance()->newMethod(*$2, $1) ;

    // put new method on stack
    $$ = g_currentMethod;

    // apply the method qualifier list.
    applyQualifierList(g_qualifierList, *$$);

    delete $2;
} ;


methodBody: TOK_LEFTPAREN parameters TOK_RIGHTPAREN ;


methodEnd: TOK_SEMICOLON ;


methodName: TOK_SIMPLE_IDENTIFIER
{
    $$ = new CIMName(*$1);
    delete $1;
}


//
//  Productions for method parameters
//
parameters : parameter
    | parameters TOK_COMMA parameter
    | /* empty */ ;


parameter: qualifierList parameterType parameterName array
{
    // ATTN: P2 2002 Question Need to create default value including type?

    YACCTRACE("parameter:");
    CIMParameter *p = 0;
    cimmofParser *cp = cimmofParser::Instance();

    // Create new parameter with name, type, isArray, array, referenceClassName
    if ($4 == -1) {
        p = cp->newParameter(*$3, $2, false, 0, g_referenceClassName);
    }
    else
    {
        p = cp->newParameter(*$3, $2, true, $4, g_referenceClassName);
    }

    g_referenceClassName = CIMName();

    YACCTRACE("parameter:applyQualifierList");
    applyQualifierList(g_qualifierList, *p);

    cp->applyParameter(*g_currentMethod, *p);
    delete p;
    delete $3;
} ;


parameterType: dataType { $$ = $1; }
             | objectRef { $$ = CIMTYPE_REFERENCE; } ;


/*
**------------------------------------------------------------------------------
**
**   property Declaration productions and processing
**
**------------------------------------------------------------------------------
*/

propertyDeclaration: qualifierList propertyBody propertyEnd
{
    // set body to stack and apply qualifier list
    // ATTN: the apply qualifer only works here because
    // there are not lower level qualifiers.  We do productions
    // that might have lower level qualifiers differently by
    // setting up a xxxHead production where qualifiers are
    // applied.
    YACCTRACE("propertyDeclaration:");
    $$ = $2;
    applyQualifierList(g_qualifierList, *$$);
} ;


propertyBody: dataType propertyName array typedDefaultValue
{
    CIMValue *v = valueFactory::createValue($1, $3,
            ($4->type == CIMMOF_NULL_VALUE), $4->value);
    if ($3 == -1)
    {
        $$ = cimmofParser::Instance()->newProperty(*$2, *v, false, 0);
    }
    else
    {
        $$ = cimmofParser::Instance()->newProperty(*$2, *v, true, $3);
    }

    delete $2;
    delete $4->value;
    delete v;
} ;


propertyEnd: TOK_SEMICOLON ;


/*
**------------------------------------------------------------------------------
**
**    reference Declaration productions and processing
**
**------------------------------------------------------------------------------
*/

referenceDeclaration: qualifierList referencedObject TOK_REF referenceName
                      referencePath TOK_SEMICOLON
{
    String s(*$2);
    if (!String::equal(*$5, String::EMPTY))
        s.append("." + *$5);
    CIMValue *v = valueFactory::createValue(CIMTYPE_REFERENCE, -1, true, &s);
    //KS add the isArray and arraysize parameters. 8 mar 2002
    $$ = cimmofParser::Instance()->newProperty(*$4, *v, false,0, *$2);
    applyQualifierList(g_qualifierList, *$$);
    delete $2;
    delete $4;
    delete $5;
    delete v;
} ;


referencedObject: TOK_SIMPLE_IDENTIFIER { $$ = $1; } ;


referenceName: TOK_SIMPLE_IDENTIFIER { $$ = $1; };


referencePath: TOK_EQUAL stringValue { $$ = $2; }
    | /* empty */ { $$ = new String(String::EMPTY); } ;


objectRef: className TOK_REF
{
    g_referenceClassName = *$1;
    delete $1;
}


parameterName: TOK_SIMPLE_IDENTIFIER
{
    $$ = new CIMName(*$1);
    delete $1;
}


propertyName: TOK_SIMPLE_IDENTIFIER
{
    $$ = new CIMName(*$1);
    delete $1;
}


array: TOK_LEFTSQUAREBRACKET TOK_POSITIVE_DECIMAL_VALUE
     TOK_RIGHTSQUAREBRACKET
        {
            $$ = (Uint32) valueFactory::stringToUint(*$2, CIMTYPE_UINT32);
            delete $2;
        }
    | TOK_LEFTSQUAREBRACKET TOK_RIGHTSQUAREBRACKET
        { $$ = 0; }
    | /* empty */
        { $$ = -1; } ;


typedDefaultValue: TOK_EQUAL typedInitializer { $$ = $2; }
|
    {   /* empty */
        g_typedInitializerValue.type = CIMMOF_NULL_VALUE;
        g_typedInitializerValue.value = new String(String::EMPTY);
        $$ = &g_typedInitializerValue;
    };


initializer: constantValue
        { $$ = $1; }
    | arrayInitializer
        { $$ = $1; }
    | referenceInitializer
        { $$ = $1; } ;


// The typedInitializer element is syntactially identical to
// the initializer element. However, the typedInitializer element
// returns, in addition to the value, the type of the value.
typedInitializer: nonNullConstantValue
        {
            g_typedInitializerValue.type = CIMMOF_CONSTANT_VALUE;
            g_typedInitializerValue.value =  $1;
            $$ = &g_typedInitializerValue;
        }
    | TOK_NULL_VALUE
        {
            g_typedInitializerValue.type = CIMMOF_NULL_VALUE;
            g_typedInitializerValue.value = new String(String::EMPTY);
            $$ = &g_typedInitializerValue;
        }
    | arrayInitializer
        {
            g_typedInitializerValue.type = CIMMOF_ARRAY_VALUE;
            g_typedInitializerValue.value =  $1;
            $$ = &g_typedInitializerValue;
        }
    | referenceInitializer
        {
            g_typedInitializerValue.type = CIMMOF_REFERENCE_VALUE;
            g_typedInitializerValue.value =  $1;
            $$ = &g_typedInitializerValue;
        };


// BUG 497 - Commmas embedded in strings in arrays change the
// strings.  Aded function stringWComma to escape commas.
constantValues: constantValue
        {
        *$$ = valueFactory::stringWComma(String(*$1)); }
    | constantValues TOK_COMMA constantValue
        {
            YACCTRACE("constantValues:1, Value= " << *$3);
            (*$$).append(",");
            //(*$$).append(*$3);
            (*$$).append(valueFactory::stringWComma(String(*$3)));
            delete $3;
        } ;


// The nonNullConstantValue has been added to allow NULL
// to be distinguished from the EMPTY STRING.

constantValue: nonNullConstantValue
        {$$ = $1;}
    | TOK_NULL_VALUE
        { $$ = new String(String::EMPTY); } ;


nonNullConstantValue: integerValue { $$ = $1; }
    | TOK_REAL_VALUE
        { $$ = $1; }
    | TOK_CHAR_VALUE
        { $$ =  $1; }
    | stringValues
        { }
    | booleanValue
        {
            $$ = new String($1 ? "T" : "F");
    };


integerValue: TOK_POSITIVE_DECIMAL_VALUE
    | TOK_OCTAL_VALUE
    | TOK_HEX_VALUE
    | TOK_BINARY_VALUE
    | TOK_SIGNED_DECIMAL_VALUE;


booleanValue: TOK_FALSE
        { $$ = 0; }
    | TOK_TRUE
        { $$ = 1; } ;


stringValues: stringValue { $$ = $1; }
    | stringValues stringValue
    {
        (*$$).append(*$2);  delete $2;
    } ;


stringValue: TOK_STRING_VALUE
{
   //String oldrep = *$1;
   //String s(oldrep), s1(String::EMPTY);
   // Handle quoted quote
   //int len = s.size();
   //if (s[len] == '\n') {
       //error: new line inside a string constant unless it is quoted
       //if (s[len - 2] == '\\') {
           //if (len > 3)
            //s1 = s.subString(1, len-3);
       //} else {
           //cimmof_error("New line in string constant");
           //}
       //cimmofParser::Instance()->increment_lineno();
   //} else { // Can only be a quotation mark
       //if (s[len - 2] == '\\') {  // if it is quoted
           //if (len > 3) s1 = s.subString(1, len-3);
           //s1.append('\"');
           //cimmof_yy_less(len-1);
       //} else { // This is the normal case:  real quotes on both end
           //s1 = s.subString(1, len - 2) ;
           //}
       //}
   //delete $1;
   $$ = //new String(s1);
        new String(*$1);
   delete $1;
} ;


arrayInitializer:
        TOK_LEFTCURLYBRACE constantValues TOK_RIGHTCURLYBRACE
            { $$ = $2; }
    | TOK_LEFTCURLYBRACE  TOK_RIGHTCURLYBRACE
       { $$ = new String(String::EMPTY); };


referenceInitializer: objectHandle {}
    | aliasInitializer {  } ;


objectHandle: TOK_DQUOTE namespaceHandleRef modelPath TOK_DQUOTE
{
    // The objectName string is decomposed for syntactical validation purposes
    // and reassembled here for later parsing in creation of an CIMObjectPath
    // instance
    String *s = new String(*$2);
    if (!String::equal(*s, String::EMPTY) && $3)
        (*s).append(":");
    if ($3) {
        (*s).append($3->toString());
    }
    $$ = s;
    delete $2;
    delete $3;
    MOF_trace2 ("objectHandle done $$ = ", $$->getCString());
} ;


aliasInitializer : aliasIdentifier
{

    CIMObjectPath AOP;

    MOF_trace2("aliasInitializer $$ = ", $$->getCString());
    MOF_trace2("aliasInitializer $1 = ", $1->getCString());

    g_currentAliasRef = *$$;

    MOF_trace2("aliasInitializer g_currentAliasRef = ",
        g_currentAliasRef.getCString());
    if (cimmofParser::Instance()->getInstanceAlias(g_currentAliasRef, AOP) == 0)
    {
        yyerror("aliasInitializer - 'aliasIdentifier' NOT FOUND");
        YYABORT;
    }

    String *s = new String(AOP.toString());

    $$ = s;

    delete $1;

    MOF_trace2 ("aliasInitializer done $$ = ", $$->getCString());

};


namespaceHandleRef: namespaceHandle TOK_COLON
        { }
    | /* empty */ { $$ = new String(String::EMPTY); };


namespaceHandle: stringValue {};


modelPath: className TOK_PERIOD keyValuePairList {
    CIMObjectPath *m = new CIMObjectPath(
        String::EMPTY,
        CIMNamespaceName(),
        (*$1).getString(),
        g_KeyBindingArray);
    g_KeyBindingArray.clear();
    delete $1;} ;


keyValuePairList: keyValuePair
        { $$ = 0; }
    | keyValuePairList TOK_COMMA keyValuePair
        { $$ = 0; } ;


keyValuePair: keyValuePairName TOK_EQUAL initializer
    {
        CIMKeyBinding::Type keyBindingType;
        Char16 firstChar = (*$3)[0];
        if (firstChar = '\"')
            keyBindingType = CIMKeyBinding::STRING;
        else if ((firstChar == 'T') || (firstChar == 't') ||
                 (firstChar == 'F') || (firstChar == 'f'))
            keyBindingType = CIMKeyBinding::BOOLEAN;
        else
            keyBindingType = CIMKeyBinding::NUMERIC;
        CIMKeyBinding *kb = new CIMKeyBinding(*$1, *$3, keyBindingType);
        g_KeyBindingArray.append(*kb);
        delete kb;
        delete $1;
        delete $3;
    } ;


keyValuePairName: TOK_SIMPLE_IDENTIFIER ;


alias: TOK_AS aliasIdentifier
    {
        $$ = $2;
        g_currentAliasDecl = *$2;
        MOF_trace2("aliasIdentifier $$ = ", $$->getCString());
        MOF_trace2("aliasIdentifier g_currentAliasDecl = ",
            g_currentAliasDecl.getCString());

    }
    | /* empty */ {
        $$ = new String(String::EMPTY);
        g_currentAliasDecl = String::EMPTY} ;


aliasIdentifier: TOK_ALIAS_IDENTIFIER ;


/*
**------------------------------------------------------------------------------
**
**   Instance Declaration productions and processing
**
**-----------------------------------------------------------------------------
*/

instanceDeclaration: instanceHead instanceBody
{
    $$ = g_currentInstance;
    if (g_currentAliasDecl != String::EMPTY)
    {
        MOF_trace2("instanceDeclaration g_currentAliasDecl = ",
                  g_currentAliasDecl.getCString());
        // MOF_trace2 ("instanceDeclaration instance =
        //  ", ((CIMObject *)$$)->toString().getCString());
        if (cimmofParser::Instance()->addInstanceAlias(
            g_currentAliasDecl, $$, true) == 0)
        {
            // Error alias already exist
            MOF_error("ERROR: alias ALREADY EXISTS: aliasIdentifier = ",
                g_currentAliasDecl.getCString());
            yyerror("instanceDeclaration - 'aliasIdentifier' ALREADY EXISTS");
            YYABORT;
        }
    }
};



instanceHead: qualifierList TOK_INSTANCE TOK_OF className alias
{
    if (g_currentInstance)
        delete g_currentInstance;
    g_currentAliasDecl = *$5;
    g_currentInstance = cimmofParser::Instance()->newInstance(*$4);
    // apply the qualifierlist to the current instance
    $$ = g_currentInstance;
    applyQualifierList(g_qualifierList, *$$);
    delete $4;
    delete $5;
    if (g_currentAliasDecl != String::EMPTY)
        MOF_trace2("instanceHead g_currentAliasDecl = ",
            g_currentAliasDecl.getCString());
} ;



instanceBody: TOK_LEFTCURLYBRACE valueInitializers TOK_RIGHTCURLYBRACE
    TOK_SEMICOLON ;



valueInitializers: valueInitializer
    | valueInitializers valueInitializer ;



// ATTN-DE-P1-20020427: Processing NULL Initializer values is incomplete.
// Currently only the arrayInitializer element has been modified to
// return CIMMOF_NULL_VALUE
valueInitializer: qualifierList TOK_SIMPLE_IDENTIFIER TOK_EQUAL
                  typedInitializer TOK_SEMICOLON
{
    AutoPtr<String> identifier($2);
    cimmofParser *cp = cimmofParser::Instance();
    // ATTN: P1 InstanceUpdate function 2001 BB  Instance update needs
    // work here and CIMOM
    // a property.  It must be fixed in the Common code first.
    // What we have to do here is create a CIMProperty  and initialize it with
    // the value provided.  The name of the property is $2 and it belongs
    // to the class whose name is in g_currentInstance->getClassName().
    // The steps are
    //   2. Get  property declaration's value object
    CIMProperty *oldprop =
        cp->PropertyFromInstance(*g_currentInstance, *identifier);
    CIMValue *oldv = cp->ValueFromProperty(*oldprop);

    //   3. create the new Value object of the same type

    // We want createValue to interpret a value as an array if is enclosed
    // in {}s (e.g., { 2 } or {2, 3, 5}) or it is NULL and the property is
    // defined as an array. createValue is responsible for the actual
    // validation.

    CIMValue *v = valueFactory::createValue(oldv->getType(),
            (($4->type == CIMMOF_ARRAY_VALUE) ||
             (($4->type == CIMMOF_NULL_VALUE) && oldprop->isArray()))?0:-1,
            ($4->type == CIMMOF_NULL_VALUE),
            $4->value);


    //   4. create a clone property with the new value
    CIMProperty *newprop = cp->copyPropertyWithNewValue(*oldprop, *v);

    //   5. apply the qualifiers;
    applyQualifierList(g_qualifierList, *newprop);

    //   6. and apply the CIMProperty to g_currentInstance.
    cp->applyProperty(*g_currentInstance, *newprop);
    delete $4->value;
    delete oldprop;
    delete oldv;
    delete v;
    delete newprop;
};





/*
**------------------------------------------------------------------------------
**
** Compiler directive productions and processing
**
**------------------------------------------------------------------------------
*/

compilerDirective: compilerDirectiveInclude
    {
        //printf("compilerDirectiveInclude ");
    }
    | compilerDirectivePragma
    {
        //printf("compilerDirectivePragma ");
    } ;


compilerDirectiveInclude: TOK_PRAGMA TOK_INCLUDE TOK_LEFTPAREN fileName
                          TOK_RIGHTPAREN
    {
      cimmofParser::Instance()->enterInlineInclude(*$4); delete $4;
    };


fileName: stringValue { $$ = $1; } ;


compilerDirectivePragma: TOK_PRAGMA pragmaName
       TOK_LEFTPAREN pragmaVal TOK_RIGHTPAREN
    {
        cimmofParser::Instance()->processPragma(*$2, *$4);
        delete $2;
        delete $4;
    };




/*
**------------------------------------------------------------------------------
**
**  qualifier Declaration productions and processing
**
**------------------------------------------------------------------------------
*/

qualifierDeclaration: TOK_QUALIFIER qualifierName qualifierValue scope
                       defaultFlavor TOK_SEMICOLON
{
    $$ = cimmofParser::Instance()->newQualifierDecl(*$2, $3, *$4, *$5);
    delete $2;
    delete $3;  // CIMValue object created in qualifierValue production
    delete $4;  // CIMScope object created in scope/metaElements production
} ;


qualifierValue: TOK_COLON dataType array typedDefaultValue
{
    $$ = valueFactory::createValue($2, $3,
                                   ($4->type == CIMMOF_NULL_VALUE),
                                   $4->value);
    delete $4->value;
} ;


scope: scope_begin metaElements TOK_RIGHTPAREN { $$ = $2; } ;


scope_begin: TOK_COMMA TOK_SCOPE TOK_LEFTPAREN
    {
    g_scope = CIMScope (CIMScope::NONE);
    } ;


metaElements: metaElement { $$ = $1; }
    | metaElements TOK_COMMA metaElement
        {
            $$->addScope(*$3);
            delete $3;
        } ;
// ATTN:  2001 P3 defer There is not CIMScope::SCHEMA. Spec Limit KS


metaElement: TOK_CLASS       { $$ = new CIMScope(CIMScope::CLASS);        }
//           | TOK_SCHEMA      { $$ = new CIMScope(CIMScope::SCHEMA);       }
           | TOK_SCHEMA        { $$ = new CIMScope(CIMScope::CLASS); }
           | TOK_ASSOCIATION { $$ = new CIMScope(CIMScope::ASSOCIATION);  }
           | TOK_INDICATION  { $$ = new CIMScope(CIMScope::INDICATION);   }
//           | TOK_QUALIFIER   { $$ = new CIMScope(CIMScope::QUALIFIER); }
           | TOK_PROPERTY    { $$ = new CIMScope(CIMScope::PROPERTY);     }
           | TOK_REFERENCE   { $$ = new CIMScope(CIMScope::REFERENCE);    }
           | TOK_METHOD      { $$ = new CIMScope(CIMScope::METHOD);       }
           | TOK_PARAMETER   { $$ = new CIMScope(CIMScope::PARAMETER);    }
           | TOK_ANY         { $$ = new CIMScope(CIMScope::ANY);          } ;


// Correction KS 4 march 2002 - Set the default if empty
defaultFlavor: TOK_COMMA flavorHead explicitFlavors TOK_RIGHTPAREN
    { $$ = &g_flavor; }
    | /* empty */
    {
        g_flavor = CIMFlavor (CIMFlavor::NONE);
        $$ = &g_flavor;
    } ;


// Correction KS 4 March 2002 - set the defaults (was zero)
// Set the flavors for the defaults required: via DEFAULTS

flavorHead: TOK_FLAVOR TOK_LEFTPAREN
    {g_flavor = CIMFlavor (CIMFlavor::NONE);};


explicitFlavors: explicitFlavor
    | explicitFlavors TOK_COMMA explicitFlavor ;


// ATTN:KS-26/03/02 P2 This accumulates the flavor definitions.
// However, it allows multiple instances
// of any keyword.  Note also that each entity simply sets a bit so that you may
// set disable and enable and we will not know which overrides the other.
// We need to create the function to insure that you cannot enable then
//disable or accept the latter and override the former.

// The compiler simply provides the flavors defined in the MOF and does not
// make any assumptions about defaults, etc.  That is a problem for
// resolution of the flavors.

explicitFlavor: TOK_ENABLEOVERRIDE
        { g_flavor.addFlavor (CIMFlavor::ENABLEOVERRIDE); }
    | TOK_DISABLEOVERRIDE { g_flavor.addFlavor (CIMFlavor::DISABLEOVERRIDE); }
    | TOK_RESTRICTED      { g_flavor.addFlavor (CIMFlavor::RESTRICTED); }
    | TOK_TOSUBCLASS      { g_flavor.addFlavor (CIMFlavor::TOSUBCLASS); }
    | TOK_TRANSLATABLE    { g_flavor.addFlavor (CIMFlavor::TRANSLATABLE); };


flavor: overrideFlavors { $$ = &g_flavor; }
    | /* empty */
    {
        g_flavor = CIMFlavor (CIMFlavor::NONE);
        $$ = &g_flavor;
    } ;


overrideFlavors: explicitFlavor
    | overrideFlavors explicitFlavor ;



dataType: intDataType     { $$ = $1; }
    | realDataType    { $$ = $1; }
    | TOK_DT_STR      { $$ = CIMTYPE_STRING;   }
    | TOK_DT_BOOL     { $$ = CIMTYPE_BOOLEAN;  }
    | TOK_DT_DATETIME { $$ = CIMTYPE_DATETIME; } ;


intDataType: TOK_DT_UINT8  { $$ = CIMTYPE_UINT8;  }
    | TOK_DT_SINT8  { $$ = CIMTYPE_SINT8;  }
    | TOK_DT_UINT16 { $$ = CIMTYPE_UINT16; }
    | TOK_DT_SINT16 { $$ = CIMTYPE_SINT16; }
    | TOK_DT_UINT32 { $$ = CIMTYPE_UINT32; }
    | TOK_DT_SINT32 { $$ = CIMTYPE_SINT32; }
    | TOK_DT_UINT64 { $$ = CIMTYPE_UINT64; }
    | TOK_DT_SINT64 { $$ = CIMTYPE_SINT64; }
    | TOK_DT_CHAR16 { $$ = CIMTYPE_CHAR16; } ;


realDataType: TOK_DT_REAL32
        { $$ =CIMTYPE_REAL32; }
    | TOK_DT_REAL64
        { $$ =CIMTYPE_REAL64; };

/*
**------------------------------------------------------------------------------
**
**   Qualifier list and qualifier processing
**
**------------------------------------------------------------------------------
*/
qualifierList: qualifierListBegin qualifiers TOK_RIGHTSQUAREBRACKET
    | /* empty */
        {
            //yydebug = 1; stderr = stdout;
        };

qualifierListBegin: TOK_LEFTSQUAREBRACKET {
    //yydebug = 1; stderr = stdout;
    YACCTRACE("qualifierListbegin");
    g_qualifierList.clear(); } ;

qualifiers: qualifier
        { }
    | qualifiers TOK_COMMA qualifier
        { } ;

qualifier: qualifierName typedQualifierParameter flavor
{
    cimmofParser *p = cimmofParser::Instance();
    // The qualifier value can't be set until we know the contents of the
    // QualifierDeclaration.  That's what QualifierValue() does.
    CIMValue *v = p->QualifierValue(*$1,
                  ($2->type == CIMMOF_NULL_VALUE), *$2->value);
    $$ = p->newQualifier(*$1, *v, g_flavor);
    g_qualifierList.add(*$$);
    delete $$;
    delete $1;
    delete $2->value;
    delete v;
} ;

qualifierName: TOK_SIMPLE_IDENTIFIER {
        g_flavor = CIMFlavor (CIMFlavor::NONE); }
    | metaElement {
        $$ = new String((*$1).toString ());
        g_flavor = CIMFlavor (CIMFlavor::NONE);
        delete $1; } ;



typedQualifierParameter: TOK_LEFTPAREN nonNullConstantValue TOK_RIGHTPAREN
        {
            g_typedInitializerValue.type = CIMMOF_CONSTANT_VALUE;
            g_typedInitializerValue.value =  $2;
            $$ = &g_typedInitializerValue;
        }
    | TOK_LEFTPAREN TOK_NULL_VALUE TOK_RIGHTPAREN
        {
            g_typedInitializerValue.type = CIMMOF_NULL_VALUE;
            g_typedInitializerValue.value = new String(String::EMPTY);
            $$ = &g_typedInitializerValue;
        }
    | arrayInitializer
        {
            g_typedInitializerValue.type = CIMMOF_ARRAY_VALUE;
            g_typedInitializerValue.value =  $1;
            $$ = &g_typedInitializerValue;
        }
    |   {   /* empty */
            g_typedInitializerValue.type = CIMMOF_NULL_VALUE;
            g_typedInitializerValue.value = new String(String::EMPTY);
            $$ = &g_typedInitializerValue;
        };


pragmaName: TOK_SIMPLE_IDENTIFIER { $$ = $1; } ;


pragmaVal:  TOK_STRING_VALUE { $$ = $1; } ;

%%

/*
**==============================================================================
**
** MOF_error():
**
**==============================================================================
*/
static void MOF_error(const char * str, const char * S)
{
    printf("%s %s\n", str, S);
}

/*
**==============================================================================
**
** MOF_trace():
**
**==============================================================================
*/
// #define DEBUG_cimmof 1

static void MOF_trace(const char* str)
{
#ifdef DEBUG_cimmof
    printf("MOF_trace(): %s \n", str);
#endif // DEBUG_cimmof
}

static void MOF_trace2(const char * str, const char * S)
{
#ifdef DEBUG_cimmof
    printf("MOF_trace2(): %s %s\n", str, S);
#endif // DEBUG_cimmof
}
