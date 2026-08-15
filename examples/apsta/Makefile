# GD32VW553 Wi-Fi APSTA — Klin emit-c.
#
#   make emit KLIN=/path/to/klin/bin/klin.dart

KLIN ?= klin

.PHONY: all emit clean

all: emit

emit: out/apsta.c

out/apsta.c out/apsta.link: apsta.kl $(wildcard ../../gd32v_wifi/*.kl) $(wildcard ../../gd32v_wifi/*.[ch])
	@mkdir -p out
	@case "$(KLIN)" in \
	  *.dart) dart run "$(KLIN)" --emit-c apsta.kl ;; \
	  *) "$(KLIN)" --emit-c apsta.kl ;; \
	esac

clean:
	rm -rf out
