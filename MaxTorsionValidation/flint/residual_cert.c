/* Residual certificate for an SPD finite-element linear system K x = f.

   File format (whitespace separated):
       n
       n*n entries of K, row major
       n entries of f
       n entries of approximate x
       alpha
   alpha must be a proved lower bound for lambda_min(K).  Arb encloses all
   decimal input and the computed residual.  It then proves
       ||x-x_*||_2 <= ||r||_2/alpha,
       ||x-x_*||_K <= ||r||_2/sqrt(alpha).
*/
#include <flint/arb.h>
#include <flint/arb_mat.h>
#include <stdio.h>
#include <stdlib.h>

static int read_arb(FILE *in, arb_t x, slong prec)
{
    char token[512];
    return fscanf(in, "%511s", token) == 1 && arb_set_str(x, token, prec) == 0;
}

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s SYSTEM.txt [precision_bits]\n", argv[0]);
        return 2;
    }
    slong prec = argc == 3 ? atol(argv[2]) : 128;
    FILE *in = fopen(argv[1], "r");
    if (!in) { perror(argv[1]); return 2; }
    long nlong;
    if (fscanf(in, "%ld", &nlong) != 1 || nlong <= 0) return 2;
    slong n = nlong;
    arb_mat_t K, f, x, r;
    arb_mat_init(K, n, n); arb_mat_init(f, n, 1);
    arb_mat_init(x, n, 1); arb_mat_init(r, n, 1);
    for (slong i=0;i<n;i++) for (slong j=0;j<n;j++)
        if (!read_arb(in, arb_mat_entry(K,i,j), prec)) return 2;
    for (slong i=0;i<n;i++) if (!read_arb(in, arb_mat_entry(f,i,0), prec)) return 2;
    for (slong i=0;i<n;i++) if (!read_arb(in, arb_mat_entry(x,i,0), prec)) return 2;
    arb_t alpha, norm2sq, norm2, coeff, energy, tmp;
    arb_init(alpha); arb_init(norm2sq); arb_init(norm2); arb_init(coeff);
    arb_init(energy); arb_init(tmp);
    if (!read_arb(in, alpha, prec) || !arb_is_positive(alpha)) {
        fprintf(stderr, "alpha must be proved positive\n"); return 2;
    }
    fclose(in);

    arb_mat_mul(r, K, x, prec);
    arb_mat_sub(r, r, f, prec);
    arb_zero(norm2sq);
    for (slong i=0;i<n;i++) {
        arb_sqr(tmp, arb_mat_entry(r,i,0), prec);
        arb_add(norm2sq, norm2sq, tmp, prec);
    }
    arb_sqrtpos(norm2, norm2sq, prec);
    arb_div(coeff, norm2, alpha, prec);
    arb_sqrt(tmp, alpha, prec); arb_div(energy, norm2, tmp, prec);
    printf("residual_l2="); arb_printn(norm2,16,ARB_STR_MORE);
    printf("\ncoefficient_error_l2<="); arb_printn(coeff,16,ARB_STR_MORE);
    printf("\nenergy_error<="); arb_printn(energy,16,ARB_STR_MORE); printf("\n");

    arb_mat_clear(K); arb_mat_clear(f); arb_mat_clear(x); arb_mat_clear(r);
    arb_clear(alpha); arb_clear(norm2sq); arb_clear(norm2); arb_clear(coeff);
    arb_clear(energy); arb_clear(tmp); flint_cleanup();
    return 0;
}
