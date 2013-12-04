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

#ifdef PEGASUS_HAS_SSL
# include <Pegasus/Common/Config.h>
# include <Pegasus/Common/Executor.h>
# include <Pegasus/Common/Network.h>
# define OPENSSL_NO_KRB5 1
# include <openssl/err.h>
# include <openssl/ssl.h>
# include <openssl/rand.h>
#else
# define SSL_CTX void
#endif // end of PEGASUS_HAS_SSL

#include <time.h>
#include <Pegasus/Common/Socket.h>
#include <Pegasus/Common/Tracer.h>
#include <Pegasus/Common/FileSystem.h>
#include <Pegasus/Common/MessageLoader.h>

#include "SSLContext.h"
#include "SSLContextRep.h"

#ifdef PEGASUS_OS_PASE
# include <ILEWrapper/ILEUtilities.h>
#endif

typedef struct x509_store_ctx_st X509_STORE_CTX;

typedef struct Timestamp
{
    char year[4];
    char month[2];
    char day[2];
    char hour[2];
    char minutes[2];
    char seconds[2];
    char dot;
    char microSeconds[6];
    char plusOrMinus;
    char utcOffset[3];
    char padding[3];
} Timestamp_t;

PEGASUS_USING_STD;

PEGASUS_NAMESPACE_BEGIN

const int SSLCallbackInfo::SSL_CALLBACK_INDEX = 0;

//
// use the following definitions only if SSL is available
//
#ifdef PEGASUS_HAS_SSL

AutoArrayPtr<Mutex> SSLEnvironmentInitializer::_sslLocks;
int SSLEnvironmentInitializer::_instanceCount = 0;
Mutex SSLEnvironmentInitializer::_instanceCountMutex;


//
// Convert ASN1_UTCTIME to CIMDateTime
//
CIMDateTime getDateTime(const ASN1_UTCTIME *utcTime)
{
    struct tm time;
    int offset;
    Timestamp_t timeStamp;
    char tempString[80];
    char plusOrMinus = '+';
    unsigned char* utcTimeData = utcTime->data;

    memset(&time, '\0', sizeof(time));

#define g2(p) ( ( (p)[0] - '0' ) * 10 + (p)[1] - '0' )

    if (utcTime->type == V_ASN1_GENERALIZEDTIME)
    {
        time.tm_year = g2(utcTimeData) * 100;
        utcTimeData += 2;  // Remaining data is equivalent to ASN1_UTCTIME type
        time.tm_year += g2(utcTimeData);
    }
    else
    {
        time.tm_year = g2(utcTimeData);
        if (time.tm_year < 50)
        {
            time.tm_year += 2000;
        }
        else
        {
            time.tm_year += 1900;
        }
    }

    time.tm_mon = g2(utcTimeData + 2) - 1;
    time.tm_mday = g2(utcTimeData + 4);
    time.tm_hour = g2(utcTimeData + 6);
    time.tm_min = g2(utcTimeData + 8);
    time.tm_sec = g2(utcTimeData + 10);

    if (utcTimeData[12] == 'Z')
    {
        offset = 0;
    }
    else
    {
        offset = g2(utcTimeData + 13) * 60 + g2(utcTimeData + 15);
        if (utcTimeData[12] == '-')
        {
            plusOrMinus = '-';
        }
    }
#undef g2

    memset((void *)&timeStamp, 0, sizeof(Timestamp_t));

    // Format the date.
    sprintf((char *) &timeStamp,"%04d%02d%02d%02d%02d%02d.%06d%04d",
            time.tm_year,
            time.tm_mon + 1,
            time.tm_mday,
            time.tm_hour,
            time.tm_min,
            time.tm_sec,
            0,
            offset);

    timeStamp.plusOrMinus = plusOrMinus;

    CIMDateTime dateTime;

    dateTime.clear();
    strcpy(tempString, (char *)&timeStamp);
    dateTime.set(tempString);

    return dateTime;
}

//
// Static class used to define C++ callback functions for OpenSSL.
//
class SSLCallback
{

public:
    static int verificationCallback(
        int preVerifyOk,
        X509_STORE_CTX* ctx);
    static int verificationCRLCallback(
        int ok,
        X509_STORE_CTX* ctx,
        X509_STORE* sslCRLStore);
};

//
// Callback function that is called by the OpenSSL library. This function
// checks whether the certificate is listed in any of the CRL's
//
// return 1 if revoked, 0 otherwise
//
int SSLCallback::verificationCRLCallback(
    int ok,
    X509_STORE_CTX* ctx,
    X509_STORE* sslCRLStore)
{
    PEG_METHOD_ENTER(TRC_SSL, "SSLCallback::verificationCRLCallback");

    char buf[1024];

    //check whether a CRL store was specified
    if (sslCRLStore == NULL)
    {
        PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL3,
            "---> SSL: CRL store is NULL");
        PEG_METHOD_EXIT();
        return 0;
    }

    //get the current certificate info
    X509* currentCert;
    X509_NAME* issuerName;
    X509_NAME* subjectName;
    ASN1_INTEGER* serialNumber;

    currentCert = X509_STORE_CTX_get_current_cert(ctx);
    subjectName = X509_get_subject_name(currentCert);
    issuerName = X509_get_issuer_name(currentCert);
    serialNumber = X509_get_serialNumber(currentCert);

    //log certificate information
    //this is information in the "public" key, so it does no harm to log it
    X509_NAME_oneline(issuerName, buf, sizeof(buf));
    PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL4,
        "---> SSL: Certificate Data: Issuer/Subject");
    PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL4, buf);
    X509_NAME_oneline(subjectName, buf, sizeof(buf));
    PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL4, buf);

    //initialize the CRL store
    X509_STORE_CTX crlStoreCtx;
    X509_STORE_CTX_init(&crlStoreCtx, sslCRLStore, NULL, NULL);

    PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL4,
        "---> SSL: Initialized CRL store");

    //attempt to get a CRL issued by the certificate's issuer
    X509_OBJECT obj;
    if (X509_STORE_get_by_subject(
            &crlStoreCtx, X509_LU_CRL, issuerName, &obj) <= 0)
    {
        X509_STORE_CTX_cleanup(&crlStoreCtx);
        PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL3,
            "---> SSL: No CRL by that issuer");
        PEG_METHOD_EXIT();
        return 0;
    }
    X509_STORE_CTX_cleanup(&crlStoreCtx);

    //get CRL
    X509_CRL* crl = obj.data.crl;
    if (crl == NULL)
    {
        PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL4, "---> SSL: CRL is null");
        PEG_METHOD_EXIT();
        return 0;
    }
    else
    {
        PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL4,
            "---> SSL: Found CRL by that issuer");
    }

    //get revoked certificates
    STACK_OF(X509_REVOKED)* revokedCerts = NULL;
    revokedCerts = X509_CRL_get_REVOKED(crl);
    int numRevoked = sk_X509_REVOKED_num(revokedCerts);
    PEG_TRACE((TRC_SSL, Tracer::LEVEL4,
        "---> SSL: Number of certificates revoked by the issuer %d\n",
        numRevoked));

    //check whether the subject's certificate is revoked
    X509_REVOKED* revokedCert = NULL;
    for (int i = 0; i < sk_X509_REVOKED_num(revokedCerts); i++)
    {
#if OPENSSL_VERSION_NUMBER >= 0x10000000
        revokedCert = (X509_REVOKED *)sk_value(&X509_CRL_get_REVOKED(crl)->stack, i);
#else
        revokedCert = (X509_REVOKED *)sk_value(X509_CRL_get_REVOKED(crl), i);
#endif

        //a matching serial number indicates revocation
        if (ASN1_INTEGER_cmp(revokedCert->serialNumber, serialNumber) == 0)
        {
            PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL2,
                "---> SSL: Certificate is revoked");
            X509_STORE_CTX_set_error(ctx, X509_V_ERR_CERT_REVOKED);
            X509_CRL_free(crl);
            PEG_METHOD_EXIT();
            return 1;
        }
    }

    X509_CRL_free(crl);

    PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL4,
        "---> SSL: Certificate is not revoked at this level");

    PEG_METHOD_EXIT();
    return 0;
}

//
// Callback function that is called by the OpenSSL library. This function
// extracts X509 certficate information and pass that on to client application
// callback function.
// We HAVE to build the certificate in all cases since it's needed to get
// the associated username out of the repository later in the transaction.
//
int SSLCallback::verificationCallback(int preVerifyOk, X509_STORE_CTX* ctx)
{
    PEG_METHOD_ENTER(TRC_SSL, "SSLCallback::callback()");
    char   buf[256];
    X509   *currentCert;
    SSL    *ssl;
    int    revoked = -1;

    PEG_TRACE((TRC_SSL, Tracer::LEVEL4,
        "--->SSL: Preverify result %d", preVerifyOk));

    //
    // get the verification callback info specific to each SSL connection
    //
    ssl = (SSL*) X509_STORE_CTX_get_ex_data(
        ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    SSLCallbackInfo* exData = (SSLCallbackInfo*) SSL_get_ex_data(
        ssl, SSLCallbackInfo::SSL_CALLBACK_INDEX);

#ifdef PEGASUS_ENABLE_SSL_CRL_VERIFICATION
    //
    // Check to see if a CRL path is defined
    //
    if (exData->_rep->crlStore != NULL)
    {
        revoked = verificationCRLCallback(
            preVerifyOk,ctx,exData->_rep->crlStore);
        PEG_TRACE((TRC_SSL, Tracer::LEVEL4,
            "---> SSL: CRL callback returned %d", revoked));

        if (revoked) //with the SSL callbacks '0' indicates failure
        {
            PEG_METHOD_EXIT();
            return 0;
        }
    }

    PEG_TRACE((TRC_SSL, Tracer::LEVEL4,
        "---> SSL: CRL callback returned %d", revoked));
#endif

    //
    // get the current certificate
    //
    currentCert = X509_STORE_CTX_get_current_cert(ctx);

    //
    // get the default verification error code
    //
    int errorCode = X509_STORE_CTX_get_error(ctx);

    //
    // get the depth of certificate chain
    //
    int depth = X509_STORE_CTX_get_error_depth(ctx);

    //
    // get the version on the certificate
    //
    long version = X509_get_version(currentCert);

    //
    // get the serial number of the certificate
    //
    long serialNumber = ASN1_INTEGER_get(X509_get_serialNumber(currentCert));

    //
    // get the validity of the certificate
    //
    CIMDateTime notBefore = getDateTime(X509_get_notBefore(currentCert));

    CIMDateTime notAfter = getDateTime(X509_get_notAfter(currentCert));

    //
    // get the subject name on the certificate
    //
    X509_NAME_oneline(X509_get_subject_name(currentCert), buf, 256);
    String subjectName = String(buf);

    //
    // get the default verification error string
    //
    String errorStr = String(X509_verify_cert_error_string(errorCode));

    //
    // log the error string if the default verification was failed
    //
    if (!preVerifyOk)
    {
        PEG_TRACE((TRC_SSL, Tracer::LEVEL2,
            "---> SSL: certificate default verification error: %s",
            (const char*)errorStr.getCString()));
    }

    //
    // get the issuer name on the certificate
    //
    X509_NAME_oneline(X509_get_issuer_name(currentCert), buf, 256);
    String issuerName = String(buf);

    //
    // Create the certificate object
    //

    // insert at the beginning of the array so that the peer certificate is
    // first and the root CA is last
    exData->_rep->peerCertificate.insert(0, new SSLCertificateInfo(
        subjectName, issuerName, version, serialNumber,
        notBefore, notAfter, depth, errorCode, errorStr, preVerifyOk));

    PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL3, "Created SSLCertificateInfo");

    // NOT_YET_VALID checks do not work correctly on subsequent tries --
    // Bugzilla#4283
    // call this prior to calling the user-specified callback in case they
    // want to override it
    if (errorCode == X509_V_OK &&
        (CIMDateTime::getDifference(
             CIMDateTime::getCurrentDateTime(), notBefore) > 0))
    {
        PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL2,
            "Certificate was not yet valid.");

        X509_STORE_CTX_set_error(ctx, X509_V_ERR_CERT_NOT_YET_VALID);
    }

    //
    // Call the user-specified application callback if it is specified.
    // If it is null, return OpenSSL's verification code.
    // Note that the verification result does not automatically get set
    // to X509_V_OK if the callback is successful.
    // This is because OpenSSL retains the original default error in case
    // we want to use it later.
    // To set the error, we could use X509_STORE_CTX_set_error(ctx,
    // X509_V_OK); but there is no real benefit to doing that here.
    //
    if (exData->_rep->verifyCertificateCallback == NULL)
    {
        PEG_METHOD_EXIT();
        return preVerifyOk;
    }
    else
    {
        if (exData->_rep->verifyCertificateCallback(
                *exData->_rep->peerCertificate[0]))
        {
            PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL4,
                "--> SSL: _rep->verifyCertificateCallback() returned "
                    "X509_V_OK");
            PEG_METHOD_EXIT();
            return 1;
        }
        else // verification failed, handshake will be immediately terminated
        {
            PEG_TRACE((TRC_SSL, Tracer::LEVEL1,
                "--> SSL: _rep->verifyCertificateCallback() returned error %d",
                exData->_rep->peerCertificate[0]->getErrorCode()));

            PEG_METHOD_EXIT();
            return 0;
        }
    }
}

//
// Callback function called by OpenSSL.  This request is merely forwarded
// to the static function SSLCallback::callback().  The SSLCallback class
// is a friend class of the Pegasus SSL related classes needed to complete
// the callback.
//
extern "C" int prepareForCallback(int preVerifyOk, X509_STORE_CTX *ctx)
{
    return SSLCallback::verificationCallback(preVerifyOk, ctx);
}

//
// SSL context area
//
// For the OSs that don't have /dev/random device file,
// must enable PEGASUS_SSL_RANDOMFILE flag.
//
SSLContextRep::SSLContextRep(
    const String& trustStore,
    const String& certPath,
    const String& keyPath,
    const String& crlPath,
    SSLCertificateVerifyFunction* verifyCert,
    const String& randomFile)
{
    PEG_METHOD_ENTER(TRC_SSL, "SSLContextRep::SSLContextRep()");

    _trustStore = trustStore;
    _certPath = certPath;
    _keyPath = keyPath;
    _crlPath = crlPath;
    _certificateVerifyFunction = verifyCert;

    //
    // If a truststore and/or peer verification function is specified,
    // enable peer verification
    //
    _verifyPeer = (trustStore != String::EMPTY || verifyCert != NULL);

    _randomInit(randomFile);

    _sslContext = _makeSSLContext();

    PEG_METHOD_EXIT();
}

SSLContextRep::SSLContextRep(const SSLContextRep& sslContextRep)
{
    PEG_METHOD_ENTER(TRC_SSL, "SSLContextRep::SSLContextRep()");

    _trustStore = sslContextRep._trustStore;
    _certPath = sslContextRep._certPath;
    _keyPath = sslContextRep._keyPath;
    _crlPath = sslContextRep._crlPath;
    _verifyPeer = sslContextRep._verifyPeer;
    _certificateVerifyFunction = sslContextRep._certificateVerifyFunction;
    _randomFile = sslContextRep._randomFile;

    _sslContext = _makeSSLContext();

    PEG_METHOD_EXIT();
}

//
// Destructor
//

SSLContextRep::~SSLContextRep()
{
    PEG_METHOD_ENTER(TRC_SSL, "SSLContextRep::~SSLContextRep()");

    SSL_CTX_free(_sslContext);

    PEG_METHOD_EXIT();
}

//
// initialize OpenSSL's PRNG
//
void SSLContextRep::_randomInit(const String& randomFile)
{
    PEG_METHOD_ENTER(TRC_SSL, "SSLContextRep::_randomInit()");

    Boolean ret;
    int retVal = 0;

#if defined(PEGASUS_SSL_RANDOMFILE) && !defined(PEGASUS_OS_PASE)
    if ( RAND_status() == 0 )
    {
        //
        // Initialise OpenSSL random number generator.
        //
        if ( randomFile == String::EMPTY )
        {
            PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL1,
                "Random seed file is required.");
            PEG_METHOD_EXIT();
            MessageLoaderParms parms(
                "Common.SSLContext.RANDOM_SEED_FILE_REQUIRED",
                "Random seed file required");
            throw SSLException(parms);
        }

        //
        // Try the given random seed file
        //
        ret = FileSystem::exists(randomFile);
        if (ret)
        {
            retVal = RAND_load_file(randomFile.getCString(), -1);
            if ( retVal < 0 )
            {
                PEG_TRACE((TRC_SSL, Tracer::LEVEL1,
                    "Not enough seed data in seed file: %s",
                    (const char*)randomFile.getCString()));
                PEG_METHOD_EXIT();
                // do not put in $0 in default message, but pass in filename
                // for bundle message
                MessageLoaderParms parms(
                    "Common.SSLContext.NOT_ENOUGH_SEED_DATA_IN_FILE",
                    "Not enough seed data in random seed file.",
                    randomFile);
                throw SSLException(parms);
            }
        }
        else
        {
            PEG_TRACE((TRC_SSL, Tracer::LEVEL1,
                "Seed file \'%s\' does not exist.",
                (const char*)randomFile.getCString()));
            PEG_METHOD_EXIT();
            MessageLoaderParms parms(
                "Common.SSLContext.SEED_FILE_DOES_NOT_EXIST",
                "Seed file '$0' does not exist.",
                randomFile);
            throw SSLException(parms);
        }

        if ( RAND_status() == 0 )
        {
            //
            // Try to do more seeding
            //
            long seedNumber;
# if defined(PEGASUS_COMPILER_MSVC)
            srand((unsigned int)time(NULL)); // Initialize
            seedNumber = rand();
# else
            srandom((unsigned int)time(NULL)); // Initialize
            seedNumber = random();
# endif
            RAND_seed((unsigned char *) &seedNumber, sizeof(seedNumber));

            int  seedRet = RAND_status();
            if ( seedRet == 0 )
            {
                PEG_TRACE((TRC_SSL, Tracer::LEVEL1,
                    "Not enough seed data in random seed file, "
                        "RAND_status = %d",
                    seedRet));
                PEG_METHOD_EXIT();
                // do not put in $0 in default message, but pass in filename
                // for bundle message
                MessageLoaderParms parms(
                    "Common.SSLContext.NOT_ENOUGH_SEED_DATA_IN_FILE",
                    "Not enough seed data in random seed file.",
                    randomFile);
                throw SSLException(parms);
            }
        }
    }
#endif  /* PEGASUS_SSL_RANDOMFILE */

#ifdef PEGASUS_OS_PASE
    if (RAND_status() == 0)
    {
        // generate random number for pase must use specify function
        unsigned char prn[1024];
        umeGenerateRandomNumber(prn, sizeof(prn));
        RAND_seed(prn, 1024);
    }
#endif

    int seedRet = RAND_status();
    if (seedRet == 0)
    {
        PEG_TRACE((TRC_SSL, Tracer::LEVEL1,
            "Not enough seed data, RAND_status = %d",
            seedRet));
        PEG_METHOD_EXIT();
        MessageLoaderParms parms(
            "Common.SSLContext.NOT_ENOUGH_SEED_DATA",
            "Not enough seed data.");
        throw SSLException(parms);
    }

    PEG_METHOD_EXIT();
}

SSL_CTX* SSLContextRep::_makeSSLContext()
{
    PEG_METHOD_ENTER(TRC_SSL, "SSLContextRep::_makeSSLContext()");

    SSL_CTX * sslContext = 0;

    //
    // create SSL Context Area
    //

    if (!(sslContext = SSL_CTX_new(SSLv23_method())))
    {
        PEG_METHOD_EXIT();
        MessageLoaderParms parms(
            "Common.SSLContext.COULD_NOT_GET",
            "Could not get SSL CTX");
        throw SSLException(parms);
    }

#ifdef PEGASUS_SSL_WEAKENCRYPTION
    if (!(SSL_CTX_set_cipher_list(sslContext, SSL_TXT_EXP40)))
    {
        MessageLoaderParms parms(
            "Common.SSLContext.COULD_NOT_SET_CIPHER_LIST",
            "Could not set the cipher list");
        throw SSLException(parms);
    }
#endif

    //
    // set overall SSL Context flags
    //

    SSL_CTX_set_quiet_shutdown(sslContext, 1);
    SSL_CTX_set_mode(sslContext, SSL_MODE_AUTO_RETRY);
    SSL_CTX_set_mode(sslContext, SSL_MODE_ENABLE_PARTIAL_WRITE);
    SSL_CTX_set_session_cache_mode(sslContext, SSL_SESS_CACHE_OFF);

    int options = SSL_OP_ALL;
#ifndef PEGASUS_ENABLE_SSLV2 //SSLv2 is disabled by default
    options |= SSL_OP_NO_SSLv2;
#endif
    SSL_CTX_set_options(sslContext, options);

    if (_verifyPeer)
    {
        // ATTN: We might still need a flag to specify
        // SSL_VERIFY_FAIL_IF_NO_PEER_CERT
        // If SSL_VERIFY_FAIL_IF_NO_PEER_CERT is ON, SSL will immediately be
        // terminated if the client sends no certificate or sends an
        // untrusted certificate.  The callback function is not called in
        // this case; the handshake is simply terminated.
        // This value has NO effect in from a client perspective

        if (_certificateVerifyFunction != NULL)
        {
            PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL3,
                "---> SSL: certificate verification callback specified");
            SSL_CTX_set_verify(sslContext,
                SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, prepareForCallback);
        }
        else
        {
            PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL3,
                "---> SSL: Trust Store specified");
            SSL_CTX_set_verify(sslContext,
                SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE |
                    SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                prepareForCallback);
        }
    }
    else
    {
        PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL3,
            "---> SSL: Trust Store and certificate verification callback "
                "are NOT specified");
        SSL_CTX_set_verify(sslContext, SSL_VERIFY_NONE, NULL);
    }

    //
    // Check if there is CA certificate file or directory specified. If
    // specified, and is not empty, load the certificates from the Trust store.
    //
    if (_trustStore != String::EMPTY)
    {
        //
        // The truststore may be a single file of CA certificates OR
        // a directory containing multiple CA certificates.
        // Check which one it is, and call the load_verify_locations function
        // with the appropriate parameter.  Note: It is possible to have both
        // options, in which case the CA file takes precedence over the CA path.
        // However, since there is currently only one trust parameter to the
        // SSL functions, only allow one choice here.
        //
        if (FileSystem::isDirectory(_trustStore))
        {
            PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL3,
                            "---> SSL: Truststore is a directory");
            //
            // load certificates from the trust store
            //
            PEG_TRACE((TRC_SSL, Tracer::LEVEL3,
                "---> SSL: Loading certificates from the trust store: %s",
                (const char*)_trustStore.getCString()));

            if ((!SSL_CTX_load_verify_locations(
                     sslContext, NULL, _trustStore.getCString())) ||
                (!SSL_CTX_set_default_verify_paths(sslContext)))
            {
                PEG_TRACE((TRC_SSL, Tracer::LEVEL1,
                    "---> SSL: Could not load certificates from the "
                    "trust store: %s ",(const char*)_trustStore.getCString()));
                MessageLoaderParms parms(
                    "Common.SSLContext.COULD_NOT_LOAD_CERTIFICATES",
                    "Could not load certificates in to trust store.");
                PEG_METHOD_EXIT();
                throw SSLException(parms);
            }

        }
        else if (FileSystem::exists(_trustStore))
        {
            PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL3,
                "---> SSL: Truststore is a file");
            //
            // Get size of the trust store file:
            //
            Uint32 fileSize = 0;

            FileSystem::getFileSize(_trustStore, fileSize);

            if (fileSize > 0)
            {
                //
                // load certificates from the trust store
                //
                PEG_TRACE((TRC_SSL, Tracer::LEVEL3,
                    "---> SSL: Loading certificates from the trust store: %s",
                    (const char*)_trustStore.getCString()));

                if ((!SSL_CTX_load_verify_locations(
                         sslContext, _trustStore.getCString(), NULL)) ||
                    (!SSL_CTX_set_default_verify_paths(sslContext)))
                {
                    PEG_TRACE((TRC_SSL, Tracer::LEVEL1,
                        "---> SSL: Could not load certificates from the "
                        "trust store: %s",
                        (const char*)_trustStore.getCString()));
                    MessageLoaderParms parms(
                        "Common.SSLContext.COULD_NOT_LOAD_CERTIFICATES",
                        "Could not load certificates in to trust store.");
                    PEG_METHOD_EXIT();
                    throw SSLException(parms);
                }
            }
            else
            {
                //
                // no certificates found in the trust store
                //
                PEG_TRACE((TRC_SSL, Tracer::LEVEL2,
                    "---> SSL: No certificates to load from the "
                    "trust store: %s",(const char*)_trustStore.getCString()));
            }
        }
    }

    if (_crlPath != String::EMPTY)
    {
        // need to save this -- can we make it static since there's only
        // one CRL for cimserver?
        X509_LOOKUP* pLookup;

        _crlStore.reset(X509_STORE_new());
        if (_crlStore.get() == NULL)
        {
            PEG_METHOD_EXIT();
            throw PEGASUS_STD(bad_alloc)();
        }

        // the validity of the crlstore was checked in ConfigManager
        // during server startup
        if (FileSystem::isDirectory(_crlPath))
        {
            PEG_TRACE((TRC_SSL, Tracer::LEVEL3,
                "---> SSL: CRL store is a directory in %s",
                (const char*)_crlPath.getCString()));

            if ((pLookup = X509_STORE_add_lookup(
                     _crlStore.get(), X509_LOOKUP_hash_dir())) == NULL)
            {
                MessageLoaderParms parms(
                    "Common.SSLContext.COULD_NOT_LOAD_CRLS",
                    "Could not load certificate revocation list.");
                _crlStore.reset();
                PEG_METHOD_EXIT();
                throw SSLException(parms);
            }

            X509_LOOKUP_add_dir(
                pLookup, (const char*)_crlPath.getCString(), X509_FILETYPE_PEM);

            PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL3,
                "---> SSL: Successfully configured CRL directory");
        }
        else
        {
            PEG_TRACE((TRC_SSL, Tracer::LEVEL3,
                "---> SSL: CRL store is the file %s",
                (const char*)_crlPath.getCString()));

            if ((pLookup = X509_STORE_add_lookup(
                   _crlStore.get(), X509_LOOKUP_file())) == NULL)
            {
                MessageLoaderParms parms(
                    "Common.SSLContext.COULD_NOT_LOAD_CRLS",
                    "Could not load certificate revocation list.");
                _crlStore.reset();
                PEG_METHOD_EXIT();
                throw SSLException(parms);
            }

            X509_LOOKUP_load_file(
                pLookup, (const char*)_crlPath.getCString(), X509_FILETYPE_PEM);

            PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL3,
                "---> SSL: Successfully configured CRL file");
        }
    }

    Boolean keyLoaded = false;

    //
    // Check if there is a certificate file (file containing server
    // certificate) specified. If specified, validate and load the
    // certificate.
    //
    if (_certPath != String::EMPTY)
    {
        //
        // load the specified server certificates
        //
        PEG_TRACE((TRC_SSL, Tracer::LEVEL3,
            "---> SSL: Loading server certificate from: %s",
            (const char*)_certPath.getCString()));

        if (SSL_CTX_use_certificate_file(sslContext,
            _certPath.getCString(), SSL_FILETYPE_PEM) <=0)
        {
            PEG_TRACE((TRC_SSL, Tracer::LEVEL1,
                "---> SSL: No server certificate found in %s",
                (const char*)_certPath.getCString()));
            MessageLoaderParms parms(
                "Common.SSLContext.COULD_NOT_ACCESS_SERVER_CERTIFICATE",
                "Could not access server certificate in $0.",
                (const char*)_certPath.getCString());
            PEG_METHOD_EXIT();
            throw SSLException(parms);
        }

        //
        // If there is no key file (file containing server
        // private key) specified, then try loading the key from the
        // certificate file.
        // As of 2.4, if a keyfile is specified, its location is verified
        // during server startup and will throw an error if the path is invalid.
        //
        if (_keyPath == String::EMPTY)
        {
            PEG_TRACE((TRC_SSL, Tracer::LEVEL3,
                "---> SSL: Key file empty, loading private key from "
                "certificate file: %s",(const char*)_certPath.getCString()));
            //
            // load the private key and check for validity
            //
            if (!_verifyPrivateKey(sslContext, _certPath))
            {
                MessageLoaderParms parms(
                    "Common.SSLContext.COULD_NOT_GET_PRIVATE_KEY",
                    "Could not get private key.");
                PEG_METHOD_EXIT();
                throw SSLException(parms);
            }
            keyLoaded = true;
        }
    }

    //
    // Check if there is a key file (file containing server
    // private key) specified and the key was not already loaded.
    // If specified, validate and load the key.
    //
    if (_keyPath != String::EMPTY && !keyLoaded)
    {
        PEG_TRACE((TRC_SSL, Tracer::LEVEL3,
            "---> SSL: loading private key from: %s",
            (const char*)_keyPath.getCString()));
        //
        // load given private key and check for validity
        //
        if (!_verifyPrivateKey(sslContext, _keyPath))
        {
            MessageLoaderParms parms(
                "Common.SSLContext.COULD_NOT_GET_PRIVATE_KEY",
                "Could not get private key.");
            PEG_METHOD_EXIT();
            throw SSLException(parms);
        }
        keyLoaded = true;
    }

    PEG_METHOD_EXIT();
    return sslContext;
}

Boolean SSLContextRep::_verifyPrivateKey(SSL_CTX *ctx, const String& keyPath)
{
    PEG_METHOD_ENTER(TRC_SSL, "_verifyPrivateKey()");

    // Open the private key file.

    FILE* is = Executor::openFile(keyPath.getCString(), 'r');

    if (!is)
    {
        PEG_TRACE((TRC_SSL, Tracer::LEVEL1,
            "---> SSL: failed to open private key file: %s",
            (const char*)keyPath.getCString()));
        return false;
    }

    // Read the private key from the input stream.

    EVP_PKEY* pkey;
    pkey = PEM_read_PrivateKey(is, NULL, NULL, NULL);

    if (!pkey)
    {
        PEG_TRACE_CSTRING(
            TRC_SSL, Tracer::LEVEL1, "---> SSL: failed to create private key");
        return false;
    }

    // Close the input stream.

    fclose(is);

    // Associate the new private key with the SSL context object.

    if (SSL_CTX_use_PrivateKey(ctx, pkey) <= 0)
    {
        EVP_PKEY_free(pkey);
        PEG_TRACE((TRC_SSL, Tracer::LEVEL3,
            "---> SSL: no private key found in %s",
            (const char*)keyPath.getCString()));
        PEG_METHOD_EXIT();
        return false;
    }

    EVP_PKEY_free(pkey);

    // Check private key for validity.

    if (!SSL_CTX_check_private_key(ctx))
    {
        PEG_TRACE_CSTRING(TRC_SSL, Tracer::LEVEL1,
            "---> SSL: Private and public key do not match");
        PEG_METHOD_EXIT();
        return false;
    }

    PEG_METHOD_EXIT();
    return true;
}

SSL_CTX* SSLContextRep::getContext() const
{
    return _sslContext;
}

String SSLContextRep::getTrustStore() const
{
    return _trustStore;
}

String SSLContextRep::getCertPath() const
{
    return _certPath;
}

String SSLContextRep::getKeyPath() const
{
    return _keyPath;
}

#ifdef PEGASUS_USE_DEPRECATED_INTERFACES
String SSLContextRep::getTrustStoreUserName() const
{
    return String::EMPTY;
}
#endif

String SSLContextRep::getCRLPath() const
{
    return _crlPath;
}

SharedPtr<X509_STORE, FreeX509STOREPtr> SSLContextRep::getCRLStore() const
{
    return _crlStore;
}

void SSLContextRep::setCRLStore(X509_STORE* store)
{
    _crlStore.reset(store);
}

Boolean SSLContextRep::isPeerVerificationEnabled() const
{
    return _verifyPeer;
}

SSLCertificateVerifyFunction*
    SSLContextRep::getSSLCertificateVerifyFunction() const
{
    return _certificateVerifyFunction;
}

void SSLContextRep::validateCertificate()
{
    BIO* in = BIO_new_file(_certPath.getCString(), "r");
    PEGASUS_ASSERT(in != NULL);
    X509* cert = PEM_read_bio_X509(in, NULL, 0, NULL);
    BIO_free(in);
    PEGASUS_ASSERT(cert != NULL);

    if (X509_cmp_current_time(X509_get_notBefore(cert)) > 0)
    {
        X509_free(cert);
        MessageLoaderParms parms(
           "Common.SSLContext.CERTIFICATE_NOT_YET_VALID",
           "Certificate $0 is not yet valid.",
           _certPath);
        throw SSLException(parms);
    }

    if (X509_cmp_current_time(X509_get_notAfter(cert)) < 0)
    {
        X509_free(cert);
        MessageLoaderParms parms(
           "Common.SSLContext.CERTIFICATE_EXPIRED",
           "Certificate $0 has expired.",
           _certPath);
        throw SSLException(parms);
    }

    X509_free(cert);
}

#else

//
// these definitions are used if ssl is not available
//

SSLContextRep::SSLContextRep(
    const String& trustStore,
    const String& certPath,
    const String& keyPath,
    const String& crlPath,
    SSLCertificateVerifyFunction* verifyCert,
    const String& randomFile)
{
}

SSLContextRep::SSLContextRep(const SSLContextRep& sslContextRep) {}

SSLContextRep::~SSLContextRep() {}

SSL_CTX* SSLContextRep::_makeSSLContext() { return 0; }

Boolean SSLContextRep::_verifyPrivateKey(
    SSL_CTX *ctx,
    const String& keyPath)
{
    return false;
}

SSL_CTX* SSLContextRep::getContext() const { return 0; }

String SSLContextRep::getTrustStore() const { return String::EMPTY; }

String SSLContextRep::getCertPath() const { return String::EMPTY; }

String SSLContextRep::getKeyPath() const { return String::EMPTY; }

#ifdef PEGASUS_USE_DEPRECATED_INTERFACES
String SSLContextRep::getTrustStoreUserName() const { return String::EMPTY; }
#endif

String SSLContextRep::getCRLPath() const { return String::EMPTY; }

SharedPtr<X509_STORE, FreeX509STOREPtr> SSLContextRep::getCRLStore() const
{
    return SharedPtr<X509_STORE, FreeX509STOREPtr>();
}

void SSLContextRep::setCRLStore(X509_STORE* store) { }

Boolean SSLContextRep::isPeerVerificationEnabled() const { return false; }

SSLCertificateVerifyFunction*
    SSLContextRep::getSSLCertificateVerifyFunction() const
{
    return NULL;
}

void SSLContextRep::validateCertificate() { }

#endif // end of PEGASUS_HAS_SSL

///////////////////////////////////////////////////////////////////////////////
//
// SSLContext
//
///////////////////////////////////////////////////////////////////////////////


SSLContext::SSLContext(
    const String& trustStore,
    SSLCertificateVerifyFunction* verifyCert,
    const String& randomFile)
{
    _rep = new SSLContextRep(
        trustStore,
        String::EMPTY,
        String::EMPTY,
        String::EMPTY,
        verifyCert,
        randomFile);
}

SSLContext::SSLContext(
    const String& trustStore,
    const String& certPath,
    const String& keyPath,
    SSLCertificateVerifyFunction* verifyCert,
    const String& randomFile)
{
    _rep = new SSLContextRep(
        trustStore, certPath, keyPath, String::EMPTY, verifyCert, randomFile);
}

//PEP187
SSLContext::SSLContext(
        const String& trustStore,
        const String& certPath,
        const String& keyPath,
        const String& crlPath,
        SSLCertificateVerifyFunction* verifyCert,
        const String& randomFile)
{
#ifndef PEGASUS_ENABLE_SSL_CRL_VERIFICATION
    if (crlPath.size() > 0)
    {
        MessageLoaderParms parms(
            "Common.Exception.SSL_CRL_NOT_ENABLED_EXCEPTION",
            "SSL CRL verification is not enabled.");
        throw Exception(parms);
    }
#endif
    _rep = new SSLContextRep(
        trustStore, certPath, keyPath, crlPath, verifyCert, randomFile);
}

#ifdef PEGASUS_USE_DEPRECATED_INTERFACES
SSLContext::SSLContext(
    const String& trustStore,
    const String& certPath,
    const String& keyPath,
    SSLCertificateVerifyFunction* verifyCert,
    String trustStoreUserName,
    const String& randomFile)
{
    _rep = new SSLContextRep(
        trustStore, certPath, keyPath, String::EMPTY, verifyCert, randomFile);
}
#endif

SSLContext::SSLContext(const SSLContext& sslContext)
{
    _rep = new SSLContextRep(*sslContext._rep);
}

// Dummy constructor made private to disallow default construction
SSLContext::SSLContext()
{
}

SSLContext::~SSLContext()
{
    delete _rep;
}

String SSLContext::getTrustStore() const
{
    return _rep->getTrustStore();
}

String SSLContext::getCertPath() const
{
    return _rep->getCertPath();
}

String SSLContext::getKeyPath() const
{
    return _rep->getKeyPath();
}

String SSLContext::getCRLPath() const
{
#ifdef PEGASUS_ENABLE_SSL_CRL_VERIFICATION
    return _rep->getCRLPath();
#else
    MessageLoaderParms parms(
        "Common.Exception.SSL_CRL_NOT_ENABLED_EXCEPTION",
        "SSL CRL verification is not enabled.");
    throw Exception(parms);
    PEGASUS_UNREACHABLE(return String::EMPTY;)
#endif
}

X509_STORE* SSLContext::getCRLStore() const
{
#ifdef PEGASUS_ENABLE_SSL_CRL_VERIFICATION
    return _rep->getCRLStore().get();
#else
    MessageLoaderParms parms(
        "Common.Exception.SSL_CRL_NOT_ENABLED_EXCEPTION",
        "SSL CRL verification is not enabled.");
    throw Exception(parms);
    PEGASUS_UNREACHABLE(return 0;)
#endif
}

Boolean SSLContext::isPeerVerificationEnabled() const
{
    return _rep->isPeerVerificationEnabled();
}

#ifdef PEGASUS_USE_DEPRECATED_INTERFACES
String SSLContext::getTrustStoreUserName() const
{
    return _rep->getTrustStoreUserName();
}
#endif

SSLCertificateVerifyFunction*
    SSLContext::getSSLCertificateVerifyFunction() const
{
    return _rep->getSSLCertificateVerifyFunction();
}

void SSLContext::_validateCertificate()
{
    _rep->validateCertificate();
}

///////////////////////////////////////////////////////////////////////////////
//
// SSLCertificateInfo
//
///////////////////////////////////////////////////////////////////////////////
//
// Certificate validation result codes.
//
const int    SSLCertificateInfo::V_OK                                      = 0;

const int    SSLCertificateInfo::V_ERR_UNABLE_TO_GET_ISSUER_CERT           = 2;
const int    SSLCertificateInfo::V_ERR_UNABLE_TO_GET_CRL                   = 3;
const int    SSLCertificateInfo::V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE    = 4;
const int    SSLCertificateInfo::V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE     = 5;
const int    SSLCertificateInfo::V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY  = 6;
const int    SSLCertificateInfo::V_ERR_CERT_SIGNATURE_FAILURE              = 7;
const int    SSLCertificateInfo::V_ERR_CRL_SIGNATURE_FAILURE               = 8;
const int    SSLCertificateInfo::V_ERR_CERT_NOT_YET_VALID                  = 9;
const int    SSLCertificateInfo::V_ERR_CERT_HAS_EXPIRED                    = 10;
const int    SSLCertificateInfo::V_ERR_CRL_NOT_YET_VALID                   = 11;
const int    SSLCertificateInfo::V_ERR_CRL_HAS_EXPIRED                     = 12;
const int    SSLCertificateInfo::V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD      = 13;
const int    SSLCertificateInfo::V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD       = 14;
const int    SSLCertificateInfo::V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD      = 15;
const int    SSLCertificateInfo::V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD      = 16;
const int    SSLCertificateInfo::V_ERR_OUT_OF_MEM                          = 17;
const int    SSLCertificateInfo::V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT         = 18;
const int    SSLCertificateInfo::V_ERR_SELF_SIGNED_CERT_IN_CHAIN           = 19;
const int    SSLCertificateInfo::V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY   = 20;
const int    SSLCertificateInfo::V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE     = 21;
const int    SSLCertificateInfo::V_ERR_CERT_CHAIN_TOO_LONG                 = 22;
const int    SSLCertificateInfo::V_ERR_CERT_REVOKED                        = 23;
const int    SSLCertificateInfo::V_ERR_INVALID_CA                          = 24;
const int    SSLCertificateInfo::V_ERR_PATH_LENGTH_EXCEEDED                = 25;
const int    SSLCertificateInfo::V_ERR_INVALID_PURPOSE                     = 26;
const int    SSLCertificateInfo::V_ERR_CERT_UNTRUSTED                      = 27;
const int    SSLCertificateInfo::V_ERR_CERT_REJECTED                       = 28;
const int    SSLCertificateInfo::V_ERR_SUBJECT_ISSUER_MISMATCH             = 29;
const int    SSLCertificateInfo::V_ERR_AKID_SKID_MISMATCH                  = 30;
const int    SSLCertificateInfo::V_ERR_AKID_ISSUER_SERIAL_MISMATCH         = 31;
const int    SSLCertificateInfo::V_ERR_KEYUSAGE_NO_CERTSIGN                = 32;

const int    SSLCertificateInfo::V_ERR_APPLICATION_VERIFICATION            = 50;

class SSLCertificateInfoRep
{
public:
    String    subjectName;
    String    issuerName;
    Uint32    depth;
    Uint32    errorCode;
    Uint32    respCode;
    String    errorString;
    Uint32    versionNumber;
    long      serialNumber;
    CIMDateTime    notBefore;
    CIMDateTime    notAfter;
};


SSLCertificateInfo::SSLCertificateInfo(
    const String subjectName,
    const String issuerName,
    const int errorDepth,
    const int errorCode,
    const int respCode)
{
    _rep = new SSLCertificateInfoRep();
    _rep->subjectName = subjectName;
    _rep->issuerName = issuerName;
    _rep->versionNumber = 0;
    _rep->serialNumber = 0;
    _rep->notBefore = CIMDateTime();
    _rep->notAfter = CIMDateTime();
    _rep->depth = errorDepth;
    _rep->errorCode = errorCode;
    _rep->errorString = String::EMPTY;
    _rep->respCode = respCode;
}

SSLCertificateInfo::SSLCertificateInfo(
    const String subjectName,
    const String issuerName,
    const Uint32 versionNumber,
    const long serialNumber,
    const CIMDateTime notBefore,
    const CIMDateTime notAfter,
    const Uint32 depth,
    const Uint32 errorCode,
    const String errorString,
    const Uint32 respCode)
{
    _rep = new SSLCertificateInfoRep();
    _rep->subjectName = subjectName;
    _rep->issuerName = issuerName;
    _rep->versionNumber = versionNumber;
    _rep->serialNumber = serialNumber;
    _rep->notBefore = notBefore;
    _rep->notAfter = notAfter;
    _rep->depth = depth;
    _rep->errorCode = errorCode;
    _rep->errorString = errorString;
    _rep->respCode = respCode;
}

SSLCertificateInfo::SSLCertificateInfo(
    const SSLCertificateInfo& certificateInfo)
{
    _rep = new SSLCertificateInfoRep();
    _rep->subjectName = certificateInfo._rep->subjectName;
    _rep->issuerName = certificateInfo._rep->issuerName;
    _rep->versionNumber = certificateInfo._rep->versionNumber;
    _rep->serialNumber = certificateInfo._rep->serialNumber;
    _rep->notBefore = certificateInfo._rep->notBefore;
    _rep->notAfter = certificateInfo._rep->notAfter;
    _rep->depth = certificateInfo._rep->depth;
    _rep->errorCode = certificateInfo._rep->errorCode;
    _rep->errorString = certificateInfo._rep->errorString;
    _rep->respCode = certificateInfo._rep->respCode;
}

// Dummy constructor made private to disallow default construction
SSLCertificateInfo::SSLCertificateInfo()
{
}

SSLCertificateInfo::~SSLCertificateInfo()
{
    delete _rep;
}

String SSLCertificateInfo::getSubjectName() const
{
    return _rep->subjectName;
}

String SSLCertificateInfo::getIssuerName() const
{
    return _rep->issuerName;
}

Uint32 SSLCertificateInfo::getVersionNumber() const
{
    return _rep->versionNumber;
}

long SSLCertificateInfo::getSerialNumber() const
{
    return _rep->serialNumber;
}

CIMDateTime SSLCertificateInfo::getNotBefore() const
{
    return _rep->notBefore;
}

CIMDateTime SSLCertificateInfo::getNotAfter() const
{
    return _rep->notAfter;
}

Uint32 SSLCertificateInfo::getErrorDepth() const
{
    return _rep->depth;
}

Uint32 SSLCertificateInfo::getErrorCode()  const
{
    return _rep->errorCode;
}

void SSLCertificateInfo::setErrorCode(const int errorCode)
{
    _rep->errorCode = errorCode;
}

String SSLCertificateInfo::getErrorString() const
{
    return _rep->errorString;
}

Uint32 SSLCertificateInfo::getResponseCode()  const
{
    return _rep->respCode;
}

void SSLCertificateInfo::setResponseCode(const int respCode)
{
    _rep->respCode = respCode;
}

String SSLCertificateInfo::toString() const
{
    char buf[1024];

    String s;

    s.append("Subject Name:\n\t");
    s.append(_rep->subjectName);
    s.append("\n");

    s.append("Issuer Name:\n\t");
    s.append(_rep->issuerName);
    s.append("\n");

    sprintf(buf, "Depth: %u\n", _rep->depth);
    s.append(buf);

    sprintf(buf, "Error code: %u\n", _rep->errorCode);
    s.append(buf);

    sprintf(buf, "Response (preverify) code: %u\n", _rep->respCode);
    s.append(buf);

    s.append("Error string: ");
    s.append(_rep->errorString);
    s.append("\n");

    sprintf(buf, "Version number: %u\n", _rep->versionNumber);
    s.append(buf);

    sprintf(buf, "Serial number: %lu\n", (unsigned long)_rep->serialNumber);
    s.append(buf);

    s.append("Not before date: ");
    s.append((_rep->notBefore).toString());
    s.append("\n");

    s.append("Not after date: ");
    s.append((_rep->notAfter).toString());
    s.append("\n");

    return s;
}

///////////////////////////////////////////////////////////////////////////////
//
// SSLCallbackInfo
//
///////////////////////////////////////////////////////////////////////////////

SSLCallbackInfo::SSLCallbackInfo(SSLCertificateVerifyFunction* verifyCert)
{
    _rep = new SSLCallbackInfoRep();
    _rep->verifyCertificateCallback = verifyCert;
    _rep->crlStore = NULL;
}

SSLCallbackInfo::SSLCallbackInfo(
    SSLCertificateVerifyFunction* verifyCert,
    X509_STORE* crlStore)
{
    _rep = new SSLCallbackInfoRep();
    _rep->verifyCertificateCallback = verifyCert;
    _rep->crlStore = crlStore;
}

SSLCallbackInfo::SSLCallbackInfo(
    SSLCertificateVerifyFunction* verifyCert,
    X509_STORE* crlStore,
    String ipAddress)
{
    _rep = new SSLCallbackInfoRep();
    _rep->verifyCertificateCallback = verifyCert;
    _rep->crlStore = crlStore;
    _rep->ipAddress = ipAddress;
}

SSLCallbackInfo::~SSLCallbackInfo()
{
    PEG_METHOD_ENTER(TRC_SSL, "SSLCallbackInfo::~SSLCallbackInfo");
    for (Uint32 i = 0; i < _rep->peerCertificate.size(); i++)
    {
        delete _rep->peerCertificate[i];
    }
    delete _rep;
    PEG_METHOD_EXIT();
}

PEGASUS_NAMESPACE_END

