chmod +x build.sh
rm -rf build
mkdir build
cd build
cmake ..
make -j$(nproc)
cd ..
./matching_engine

