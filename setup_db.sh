#!/bin/bash
sudo DEBIAN_FRONTEND=noninteractive apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y postgresql postgresql-contrib
sudo service postgresql start
sudo -u postgres psql -c "CREATE USER vm1 WITH SUPERUSER;" || true
sudo -u postgres psql -c "CREATE DATABASE valyrian OWNER vm1;" || true
sudo -u postgres psql -d valyrian -f database/schema.sql
