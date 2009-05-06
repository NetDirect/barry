#!/bin/sh

# handy script to build the base libraries and opensync-plugin-0.4x

./buildgen.sh && \
	../handy-scripts/configure-barry.sh && \
	make -j2 && \
	make install && \
	cd opensync-plugin-0.4x/ && \
	../../handy-scripts/configure-barryopensync-0.4x.sh && \
	make -j2 && \
	make install

