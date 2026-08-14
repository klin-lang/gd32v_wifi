# GD32VW553 SoftAP — Klin emit-c. Linking an ELF needs the official SDK.
#
#   make emit KLIN=/path/to/klin/bin/klin.dart

KLIN ?= klin

.PHONY: all emit clean

all: emit

emit: out/ap.c

out/ap.c out/ap.link: ap.kl $(wildcard ../../gd32v_wifi/*.kl) $(wildcard ../../gd32v_wifi/*.[ch])
	@mkdir -p out
	@case "$(KLIN)" in \
	  *.dart) dart run "$(KLIN)" --emit-c ap.kl ;; \
	  *) "$(KLIN)" --emit-c ap.kl ;; \
	esac

clean:
	rm -rf out
