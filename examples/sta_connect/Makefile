# GD32VW553 STA — Klin emit-c. Linking an ELF needs the official SDK.
#
#   make emit KLIN=/path/to/klin/bin/klin.dart

KLIN ?= klin

.PHONY: all emit clean

all: emit

emit: out/sta.c

out/sta.c out/sta.link: sta.kl $(wildcard ../../gd32v_wifi/*.kl) $(wildcard ../../gd32v_wifi/*.[ch])
	@mkdir -p out
	@case "$(KLIN)" in \
	  *.dart) dart run "$(KLIN)" --emit-c sta.kl ;; \
	  *) "$(KLIN)" --emit-c sta.kl ;; \
	esac

clean:
	rm -rf out
