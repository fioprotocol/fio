#! /bin/bash

NAME=$1
EOS_PREFIX=${PREFIX}/${SUBPREFIX}
mkdir -p ${PREFIX}/bin/
#mkdir -p ${PREFIX}/lib/cmake/${PROJECT}
mkdir -p ${EOS_PREFIX}/bin
mkdir -p ${EOS_PREFIX}/licenses/fio
#mkdir -p ${EOS_PREFIX}/include
#mkdir -p ${EOS_PREFIX}/lib/cmake/${PROJECT}
#mkdir -p ${EOS_PREFIX}/cmake
#mkdir -p ${EOS_PREFIX}/scripts

# install binaries 
cp -R ${BUILD_DIR}/bin/* ${EOS_PREFIX}/bin  || exit 1

# install licenses
cp -R ${BUILD_DIR}/licenses/fio/* ${EOS_PREFIX}/licenses || exit 1

for f in $(ls "${BUILD_DIR}/bin/"); do
   bn=$(basename $f)
   ln -sf ../${SUBPREFIX}/bin/$bn ${PREFIX}/bin/$bn || exit 1
done
echo "Generating Tarball $NAME.tar.gz..."
tar -cvzf $NAME.tar.gz ./${PREFIX}/* || exit 1
rm -r ${PREFIX} || exit 1
