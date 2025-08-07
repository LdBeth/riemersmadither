all: ppmriemer pgmrobust

CHPL = chpl --gmp none -g --fast --vectorize --inline

gilbert.dylib: gilbert.c
	cc -DAPL -shared -Wl,-install_name,gilbert.dylib -o gilbert.dylib gilbert.c

%: %.chpl
	$(CHPL) -I/opt/pkg/include -L/opt/pkg/lib $<

ppmrimer: gilbert.c gilbert.h

rdc: rdc.c
	cc -O3 -ffast-math -flto -Wall -march=native -o rdc -I/opt/pkg/include -L/opt/pkg/lib rdc.c gilbert.c -lnetpbm

part: W.c
	cc -O3 -ffast-math -flto -Wall -march=native -o part -I/opt/pkg/include -L/opt/pkg/lib W.c -lnetpbm

.PHONY: clean
clean:
	rm -rvf ppmriemer pgmrobust
