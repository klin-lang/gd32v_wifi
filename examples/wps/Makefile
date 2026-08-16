# GD32VW553 Wi-Fi WPS — Klin emit-c.
#
#   make emit KLIN=/path/to/klin/bin/klin.dart

KLIN ?= klin

.PHONY: all emit clean

all: emit

emit: out/wps.c

out/wps.c out/wps.link: wps.kl $(wildcard ../../gd32v_wifi/*.kl) $(wildcard ../../gd32v_wifi/*.[ch])
	@mkdir -p out
	@case "$(KLIN)" in \
	  *.dart) dart run "$(KLIN)" --emit-c wps.kl ;; \
	  *) "$(KLIN)" --emit-c wps.kl ;; \
	esac

clean:
	rm -rf out
