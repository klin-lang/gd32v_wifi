# GD32VW553 Wi-Fi EAP-TLS — Klin emit-c.
#
#   make emit KLIN=/path/to/klin/bin/klin.dart

KLIN ?= klin

.PHONY: all emit clean

all: emit

emit: out/eap_tls.c

out/eap_tls.c out/eap_tls.link: eap_tls.kl $(wildcard ../../gd32v_wifi/*.kl) $(wildcard ../../gd32v_wifi/*.[ch])
	@mkdir -p out
	@case "$(KLIN)" in \
	  *.dart) dart run "$(KLIN)" --emit-c eap_tls.kl ;; \
	  *) "$(KLIN)" --emit-c eap_tls.kl ;; \
	esac

clean:
	rm -rf out
