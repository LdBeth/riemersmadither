all: ppmriemer pgmrobust

CHPL = chpl --gmp none --fast

gilbert.dylib: gilbert.c
	cc -DAPL -shared -Wl,-install_name,gilbert.dylib -o gilbert.dylib gilbert.c

%: %.chpl
	$(CHPL) -I/opt/pkg/include -L/opt/pkg/lib $<

ppmrimer: gilbert.c gilbert.h

.PHONY: clean
clean:
	rm -rvf ppmriemer pgmrobust
