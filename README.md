# GP Compute

GP Compute (GPC) is desgined to be an elastic, resilient and secure computing framework for the Greenplum Database (GPDB).

## Installation

1. Build and install GPDB.
2. Clone the repo and configure GPDB as a shared library. 
```bash
git clone https://github.com/xuebinsu/gp-compute.git
cd gp-compute
git submodule init
git submodule update
cd lib/gpdb/
CFLAGS+=-fPIC ./configure --disable-orca --prefix=gpdb/install/path/
```
3. Build and install GP Compute. This step may take a long time since it need to build GPDB as a shared library.
```bash
cd ../../src/
make install
```
4. Install GP Compute as an extension of GPDB using SQL:
```SQL
CREATE EXTENSION gp_compute;
```

## A Minimal Running Example

1. To start a GPC server, open a bash session and run
```bash
gp_compute_server
```
2. After the server starts, you can now create and execute UDFs using SQL:
```SQL
CREATE OR REPLACE FUNCTION rint4(i int4) RETURNS int4 AS $$
return (as.integer(i))
$$ LANGUAGE gp_compute;

select rint4(12345678);
```
