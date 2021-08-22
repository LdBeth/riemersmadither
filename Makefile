gilbert.dylib: gilbert.c
	cc -DAPL -shared -Wl,-install_name,gilbert.dylib -o gilbert.dylib gilbert.c

ppmriemer: ppmriemer.chpl, gilbert.c, gilbert.h
	chpl --fast -I/opt/pkg/include -L/opt/pkg/lib ppmriemer.chpl
