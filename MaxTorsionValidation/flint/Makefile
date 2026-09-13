CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -pedantic
LDLIBS = -lflint -lm
PYTHON ?= python3
SYMBOLIC_TESTS = \
	../symbolic/verify_hessian_reduction.py \
	../symbolic/verify_pullback_coefficients.py \
	../symbolic/verify_residual_identities.py \
	../symbolic/equilateral_sector_integrals.py

.PHONY: all test test-symbolic test-negative test-regular clean
all: mode_cert residual_cert majorant_cert second_variation_cert second_lifting_cert

mode_cert: mode_cert.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

residual_cert: residual_cert.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

majorant_cert: majorant_cert.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

second_variation_cert: second_variation_cert.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

second_lifting_cert: second_lifting_cert.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

test: all test-symbolic test-negative
	./mode_cert --radius 1e-12 --expect-symbols 3 --expect-negative 4 --expect-unresolved 2 test_modes.txt
	./mode_cert --radius-file test_regular_radii.txt --exact-regular-similarities --expect-symbols 5 --expect-negative 6 --expect-unresolved 4 freefem_n5_m32_modes.txt
	./mode_cert --radius-file test_regular_n4_radii.txt --exact-regular-similarities --expect-symbols 4 --expect-negative 4 --expect-unresolved 4 test_regular_n4_modes.txt
	./residual_cert test_system.txt 128
	./majorant_cert test_majorant.txt --prec 128 --geom-radius 1e-20
	./second_variation_cert test_second_bounds.txt --prec 128
	./second_lifting_cert test_second_lifting.txt --prec 128 --geom-radius 1e-13 --direction-radius 1e-13 --check-regular-inputs

test-symbolic:
	@set -e; for script in $(SYMBOLIC_TESTS); do $(PYTHON) "$$script"; done

test-negative: all
	$(PYTHON) test_certificate_inputs.py
	@! ./mode_cert --radius 1e-12 --expect-symbols 3 --expect-negative 5 --expect-unresolved 2 test_modes.txt >/dev/null 2>&1
	@! ./second_lifting_cert test_second_lifting.txt --prec 128 --geom-radius 0 --direction-radius 1e-13 --check-regular-inputs >/dev/null 2>&1
	@! ./second_lifting_cert test_second_lifting.txt --prec 128 --geom-radius 1e-13 --direction-radius 0 --check-regular-inputs >/dev/null 2>&1

test-regular: all
	bash ../freefem/run_regular_certificates.sh 3 10

clean:
	$(RM) mode_cert residual_cert majorant_cert second_variation_cert second_lifting_cert
