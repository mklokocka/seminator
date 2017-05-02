LTL2LDBA_NAME=ltl2ldba-0.9
OWL_VERSION=1.0.0
OWL_NAME=owl-$OWL_VERSION

#if [ ! -f $LTL2LDBA_NAME/bin/ltl2ldba ]; then
#    wget https://www7.in.tum.de/~sickert/distributions/$LTL2LDBA_NAME.zip
#    unzip $LTL2LDBA_FILE.zip
#    rm -f $LTL2LDBA_FILE.zip
#fi

if [ ! -f $OWL_NAME/bin/ltl2ldba ]; then
    wget https://www7.in.tum.de/~sickert/distributions/$OWL_NAME.zip
    unzip $OWL_NAME.zip
    rm -f $OWL_NAME.zip 
fi


