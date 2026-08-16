# GD32VW553 Wi-Fi STA roaming — Klin emit-c.
#
#   make emit KLIN=/path/to/klin/bin/klin.dart

KLIN ?= klin

.PHONY: all emit clean

all: emit

emit: out/roaming.c

out/roaming.c out/roaming.link: roaming.kl $(wildcard ../../gd32v_wifi/*.kl) $(wildcard ../../gd32v_wifi/*.[ch])
	@mkdir -p out
	@case "$(KLIN)" in \
	  *.dart) dart run "$(KLIN)" --emit-c roaming.kl ;; \
	  *) "$(KLIN)" --emit-c roaming.kl ;; \
	esac

clean:
	rm -rf out
