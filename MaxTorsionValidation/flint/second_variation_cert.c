/* Outward evaluation of the second-variation residual bound

     delta_q delta_r + (M_qr/2) delta_0^2 + delta_0 eta_qr.

   Input lines are
     q r delta_0 delta_q delta_r eta_qr M_qr
   where every decimal is a proved nonnegative upper bound.
*/
#include <flint/arb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void die(const char *message)
{
    fprintf(stderr, "second_variation_cert: %s\n", message);
    exit(2);
}

static void set_nonnegative(arb_t x, const char *s, slong prec)
{
    if (arb_set_str(x, s, prec) || !arb_is_finite(x) || !arb_is_nonnegative(x))
        die("invalid nonnegative decimal bound");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s BOUNDS [--prec bits]\n", argv[0]);
        return 2;
    }
    slong prec = 160;
    for (int i = 2; i < argc; ++i) {
        if (!strcmp(argv[i], "--prec") && i + 1 < argc)
            prec = atol(argv[++i]);
        else
            die("bad option");
    }
    if (prec < 32) die("precision must be at least 32 bits");
    FILE *in = fopen(argv[1], "r");
    if (!in) {
        perror(argv[1]);
        return 2;
    }

    long q, r, count = 0;
    char s0[256], sq[256], sr[256], se[256], sm[256];
    arb_t d0, dq, dr, eta, m, bound, tmp;
    arb_init(d0); arb_init(dq); arb_init(dr); arb_init(eta);
    arb_init(m); arb_init(bound); arb_init(tmp);
    int fields;
    while ((fields = fscanf(in, "%ld %ld %255s %255s %255s %255s %255s",
                            &q, &r, s0, sq, sr, se, sm)) != EOF) {
        if (fields != 7) die("malformed or truncated input record");
        set_nonnegative(d0, s0, prec);
        set_nonnegative(dq, sq, prec);
        set_nonnegative(dr, sr, prec);
        set_nonnegative(eta, se, prec);
        set_nonnegative(m, sm, prec);
        arb_mul(bound, dq, dr, prec);
        arb_sqr(tmp, d0, prec);
        arb_mul(tmp, tmp, m, prec);
        arb_mul_2exp_si(tmp, tmp, -1);
        arb_add(bound, bound, tmp, prec);
        arb_addmul(bound, d0, eta, prec);
        printf("q=%ld r=%ld hessian_error<=", q, r);
        arb_printn(bound, 18, ARB_STR_MORE);
        printf("\n");
        ++count;
    }
    if (ferror(in)) die("input read error");
    fclose(in);
    printf("summary bounds=%ld prec=%ld\n", count, (long)prec);
    arb_clear(d0); arb_clear(dq); arb_clear(dr); arb_clear(eta);
    arb_clear(m); arb_clear(bound); arb_clear(tmp);
    flint_cleanup();
    return count ? 0 : 2;
}
