This toolkit provides programs implementing several versions of
private set intersection (PSI) and authorized private set intersection
(APSI) schemes.

The mapping between code files and corresponding protocols in the
paper are shown below:

brsa_psi_{client.h|client.cc|server.h|server.cc} <---> DT10-2
bcckls_psi_{client.h|client.cc|server.h|server.cc} <---> JL09
fnp_psi_{client.h|client.cc|server.h|server.cc} <---> FNP04
jl10_psi_{client.h|client.cc|server.h|server.cc} <---> jl10
rsa_psi_opt_{client.h|client.cc|server.h|server.cc} <---> DT10-1

rsa_apsi_opt_{client.h|client.cc|server.h|server.cc} <---> DT10-APSI
ibe_apsi_{client.h|client.cc|server.h|server.cc} <---> IBE-APSI


In order to compile the code, GMP (5.0.1), openssl (1.0.0) and PBC
(0.5.7) must be installed in the system. Please change Makefile if
these libraries are not installed in the default directory.

To download GMP, visit http://gmplib.org/
To download OpenSSL, visit http://www.openssl.org/source/
To download PBC, visit http://crypto.stanford.edu/pbc/download.html


To run the program, ./SetInt[Adapt] -p PROTOCOL -cf CLIENT_FILE -sf SERVER_FILE -n BITS

PROTOCOL: protocol name. For program SetInt, this parameter can be
FNP_PSI, DT10_1_PSI, DT10_APSI. For program SetIntAdapt, it can be
IBE_APSI, DT10_2_PSI, JL10_PSI, JL09_PSI.  
CLIENT_FILE: client set input file 
SERVER_FILE: server set input file

This toolkit has been tested on x86_64 GNU/Linux but no warranties are
provided. Please report any bugs, questions, comments, and
contributions to yanbinl@uci.edu

Cheers,

Yanbin LU (yanbinl@uci.edu)

