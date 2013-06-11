#
#  Set up OpenSSL certificates for the tog-pegasus cimserver
#
#  Creates a default ssl.cnf file.
#  Generates a self-signed certificate for use by the cimserver.
#
cnfChanged=0;
if [ ! -e $PEGASUS_CONFIG_DIR/ssl.cnf ]; then
    mkdir -p ${PEGASUS_INSTALL_LOG%/*}
    mkdir -p $PEGASUS_CONFIG_DIR
    echo "[ req ]" > $PEGASUS_CONFIG_DIR/ssl.cnf
    echo "distinguished_name     = req_distinguished_name"  >> \
            $PEGASUS_CONFIG_DIR/ssl.cnf
    echo "prompt                 = no"  >> $PEGASUS_CONFIG_DIR/ssl.cnf
    echo "[ req_distinguished_name ]" >> $PEGASUS_CONFIG_DIR/ssl.cnf
    echo "C                      = UK" >> $PEGASUS_CONFIG_DIR/ssl.cnf
    echo "ST                     = Berkshire" >> $PEGASUS_CONFIG_DIR/ssl.cnf
    echo "L                      = Reading" >> $PEGASUS_CONFIG_DIR/ssl.cnf
    echo "O                      = The Open Group" >> \
            $PEGASUS_CONFIG_DIR/ssl.cnf
    echo "OU                     = The OpenPegasus Project" >> \
            $PEGASUS_CONFIG_DIR/ssl.cnf
    DN=`hostname`;
    if [ -z "$DN" ] || [ "$DN" = "(none)" ]; then
            DN='localhost.localdomain';
    fi;
    FQDN=`{ host -W1 $DN 2>/dev/null || echo "$DN has address "; } |\
            grep 'has address' | head -1 | sed 's/\ .*$//'`;
    if [ -z "$FQDN" ] ; then
        FQDN="$DN";
    fi;
    # cannot use 'hostname --fqdn' because this can hang indefinitely
    echo "CN                     = $FQDN"  >> $PEGASUS_CONFIG_DIR/ssl.cnf
    chmod 400 $PEGASUS_CONFIG_DIR/ssl.cnf
    chown root $PEGASUS_CONFIG_DIR/ssl.cnf
    chgrp root $PEGASUS_CONFIG_DIR/ssl.cnf
    cnfChanged=1;
fi
if [ $cnfChanged -eq 1 ] || \
         [ ! -e $PEGASUS_PEM_DIR/$PEGASUS_SSL_CERT_FILE ] || \
         [ ! -e $PEGASUS_PEM_DIR/$PEGASUS_SSL_KEY_FILE ]; then
    /usr/bin/openssl req -x509 -days 3650 -newkey rsa:2048 \
         -nodes -config $PEGASUS_CONFIG_DIR/ssl.cnf   \
         -keyout $PEGASUS_PEM_DIR/key.pem \
         -out $PEGASUS_PEM_DIR/cert.pem 2>>$PEGASUS_INSTALL_LOG
    chmod 700 $PEGASUS_PEM_DIR/*.pem
    cp -fp $PEGASUS_PEM_DIR/cert.pem \
        $PEGASUS_PEM_DIR/$PEGASUS_SSL_CERT_FILE
    cp -fp $PEGASUS_PEM_DIR/key.pem \
        $PEGASUS_PEM_DIR/$PEGASUS_SSL_KEY_FILE
    chmod 400 $PEGASUS_PEM_DIR/$PEGASUS_SSL_KEY_FILE
    chmod 444 $PEGASUS_PEM_DIR/$PEGASUS_SSL_CERT_FILE 
    rm -f $PEGASUS_PEM_DIR/key.pem $PEGASUS_PEM_DIR/cert.pem
fi;
if [ ! -e $PEGASUS_PEM_DIR/$PEGASUS_SSL_TRUSTSTORE ]; then
    cp -fp $PEGASUS_PEM_DIR/$PEGASUS_SSL_CERT_FILE \
        $PEGASUS_PEM_DIR/$PEGASUS_SSL_TRUSTSTORE
    chmod 444 $PEGASUS_PEM_DIR/$PEGASUS_SSL_TRUSTSTORE;
fi;
