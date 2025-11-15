all clean:
	$(MAKE) -C src $@

release: all
	cp src/etherL12wa.sys .
	zip -r etherL12wa.zip etherL12wa.sys README.txt

.PHONY: all clean release
