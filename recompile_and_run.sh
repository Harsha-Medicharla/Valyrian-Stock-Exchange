#!/bin/bash
sudo -u postgres psql -c "ALTER USER vm1 WITH PASSWORD 'password';"
cd build
make APIServer
cd ..
bash run_test.sh
