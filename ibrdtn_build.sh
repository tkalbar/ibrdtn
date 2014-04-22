#!/bin/bash
#

VERSION=0.12.0
MODULES="ibrcommon ibrdtn ibrdtnd ibrdtn-tools"

# build ibrcommon with openssl support
CONFOPTS="--with-openssl "

# download, extract and compile all archives
for MOD in ${MODULES}; do

        cd ${MOD}-${VERSION}
        ./configure ${CONFOPTS} $@

        if [ $? -gt 0 ]; then
                echo "error while configure ${MOD}"
                exit 1
        fi

        make

        if [ $? -gt 0 ]; then
                echo "error while compiling ${MOD}"
                exit 1
        fi

        sudo make install

        cd ..

done