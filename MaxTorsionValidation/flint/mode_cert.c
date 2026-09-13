/* Certify eigenvalue signs of 2-by-2 Hermitian Fourier symbols with Arb.

   Input lines have the form
       MODE_DATA k a d Re(b) Im(b)
   and describe [[a,b],[conj(b),d]].  --radius is a rigorous absolute
   enclosure radius for EACH of a,d,Re(b),Im(b), not a heuristic tolerance.
   Alternatively, --radius-file reads one MODE_RADIUS k radius record per
   Fourier mode.  --exact-regular-similarities inserts the exact regular-
   polygon k=0 kernel, the exact translation zero in k=1,n-1, and the exact
   zero Nyquist off-diagonal.
*/
#include <flint/arb.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *name)
{
    fprintf(stderr, "usage: %s [--prec bits] [--radius decimal | --radius-file FILE] [--exact-regular-similarities] [--expect-symbols n] [--expect-negative n] [--expect-unresolved n] FILE\n", name);
    exit(2);
}

static void print_ball(const arb_t x)
{
    arb_printn(x, 16, ARB_STR_MORE);
}

int main(int argc, char **argv)
{
    slong prec = 128;
    const char *radius_text = "0";
    const char *radius_file = NULL;
    const char *path = NULL;
    int radius_given = 0, exact_regular = 0;
    long expect_symbols=-1,expect_negative=-1,expect_unresolved=-1;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--prec") && i + 1 < argc) prec = atol(argv[++i]);
        else if (!strcmp(argv[i], "--radius") && i + 1 < argc) { radius_text = argv[++i]; radius_given=1; }
        else if (!strcmp(argv[i], "--radius-file") && i + 1 < argc) radius_file=argv[++i];
        else if (!strcmp(argv[i], "--exact-regular-similarities")) exact_regular=1;
        else if (!strcmp(argv[i], "--expect-symbols") && i + 1 < argc) expect_symbols=atol(argv[++i]);
        else if (!strcmp(argv[i], "--expect-negative") && i + 1 < argc) expect_negative=atol(argv[++i]);
        else if (!strcmp(argv[i], "--expect-unresolved") && i + 1 < argc) expect_unresolved=atol(argv[++i]);
        else if (!strcmp(argv[i], "-")) path = argv[i];
        else if (argv[i][0] == '-') usage(argv[0]);
        else path = argv[i];
    }
    if (!path || prec < 32) usage(argv[0]);
    if (expect_symbols < -1 || expect_negative < -1 || expect_unresolved < -1)
        usage(argv[0]);
    if ((radius_file && radius_given) || (radius_file && expect_symbols < 0) ||
        (exact_regular && expect_symbols < 3))
        usage(argv[0]);
    unsigned char *seen = NULL;
    if (expect_symbols >= 0) {
        seen = calloc((size_t)expect_symbols, 1);
        if (!seen && expect_symbols) { fprintf(stderr,"mode_cert: allocation failed\n"); return 2; }
    }

    FILE *in = !strcmp(path, "-") ? stdin : fopen(path, "r");
    if (!in) { perror(path); return 2; }

    arb_t radius, a, d, br, bi, trace, delta, disc, root, lo, hi, tmp;
    arb_init(radius); arb_init(a); arb_init(d); arb_init(br); arb_init(bi);
    arb_init(trace); arb_init(delta); arb_init(disc); arb_init(root);
    arb_init(lo); arb_init(hi); arb_init(tmp);
    if (arb_set_str(radius, radius_text, prec) || !arb_is_finite(radius) ||
        !arb_is_nonnegative(radius)) {
        fprintf(stderr, "invalid nonnegative radius: %s\n", radius_text);
        return 2;
    }
    arb_ptr mode_radii = NULL;
    unsigned char *radius_seen = NULL;
    if (radius_file) {
        mode_radii = _arb_vec_init(expect_symbols);
        radius_seen = calloc((size_t)expect_symbols,1);
        FILE *rf=fopen(radius_file,"r");
        if(!mode_radii||!radius_seen||!rf) {
            fprintf(stderr,"mode_cert: cannot allocate/read radius file\n"); return 2;
        }
        char line[1024],rtag[64],rs[256],extra[2];long rk;
        while(fgets(line,sizeof(line),rf)) {
            char *s=line;while(isspace((unsigned char)*s))++s;
            if(!*s||*s=='#')continue;
            int got=sscanf(s,"%63s %ld %255s %1s",rtag,&rk,rs,extra);
            if(got!=3||strcmp(rtag,"MODE_RADIUS")||rk<0||rk>=expect_symbols||
               radius_seen[rk]||arb_set_str(mode_radii+rk,rs,prec)||
               !arb_is_finite(mode_radii+rk)||
               !arb_is_nonnegative(mode_radii+rk)) {
                fprintf(stderr,"mode_cert: malformed mode radius\n");return 2;
            }
            radius_seen[rk]=1;
        }
        fclose(rf);
        for(long i=0;i<expect_symbols;++i)if(!radius_seen[i]) {
            fprintf(stderr,"mode_cert: missing radius for Fourier index %ld\n",i);return 2;
        }
    }

    char tag[64], sa[256], sd[256], sbr[256], sbi[256];
    long k;
    long symbols = 0, certified_negative = 0, unresolved = 0;
    while (fscanf(in, "%63s", tag) == 1) {
        if (strcmp(tag, "MODE_DATA")) {
            char discard[2048];
            if (!fgets(discard, sizeof(discard), in)) break;
            continue;
        }
        if (fscanf(in, "%ld %255s %255s %255s %255s", &k, sa, sd, sbr, sbi) != 5)
            { fprintf(stderr,"mode_cert: malformed MODE_DATA record\n"); return 2; }
        if (seen) {
            if (k < 0 || k >= expect_symbols || seen[k]) {
                fprintf(stderr,"mode_cert: duplicate or unexpected Fourier index %ld\n",k);
                return 2;
            }
            seen[k]=1;
        }
        if (arb_set_str(a, sa, prec) || arb_set_str(d, sd, prec) ||
            arb_set_str(br, sbr, prec) || arb_set_str(bi, sbi, prec) ||
            !arb_is_finite(a) || !arb_is_finite(d) ||
            !arb_is_finite(br) || !arb_is_finite(bi)) {
            fprintf(stderr, "cannot parse MODE_DATA for k=%ld\n", k);
            return 2;
        }
        if(mode_radii)arb_set(radius,mode_radii+k);
        if(exact_regular&&k==0) {
            arb_zero(a);arb_zero(d);arb_zero(br);arb_zero(bi);
        } else {
            arb_add_error(a, radius); arb_add_error(d, radius);
            if(exact_regular&&expect_symbols%2==0&&k==expect_symbols/2) {
                arb_zero(br);arb_zero(bi);
            } else {
                arb_add_error(br, radius); arb_add_error(bi, radius);
            }
        }

        arb_add(trace, a, d, prec);
        if(exact_regular&&k==0) {
            arb_zero(lo);arb_zero(hi);
        } else if(exact_regular&&(k==1||k==expect_symbols-1)) {
            /* Translation invariance makes one eigenvalue exactly zero; the
               transverse eigenvalue is the trace. */
            arb_set(lo,trace);arb_zero(hi);
        } else {
            /* eigenvalues = (a+d +/- sqrt((a-d)^2+4|b|^2))/2 */
            arb_sub(delta, a, d, prec);
            arb_sqr(disc, delta, prec);
            arb_sqr(tmp, br, prec); arb_mul_2exp_si(tmp, tmp, 2);
            arb_add(disc, disc, tmp, prec);
            arb_sqr(tmp, bi, prec); arb_mul_2exp_si(tmp, tmp, 2);
            arb_add(disc, disc, tmp, prec);
            /* disc is a sum of squares; sqrtpos safely intersects a tiny
               roundoff enclosure crossing zero with the nonnegative half-line. */
            arb_sqrtpos(root, disc, prec);
            arb_sub(lo, trace, root, prec); arb_mul_2exp_si(lo, lo, -1);
            arb_add(hi, trace, root, prec); arb_mul_2exp_si(hi, hi, -1);
        }

        int neg_lo = arb_is_negative(lo), neg_hi = arb_is_negative(hi);
        certified_negative += neg_lo + neg_hi;
        unresolved += (!neg_lo && !arb_is_positive(lo));
        unresolved += (!neg_hi && !arb_is_positive(hi));
        ++symbols;
        printf("k=%ld radius=",k);print_ball(radius);
        printf(" lambda_minus="); print_ball(lo);
        printf(" %s lambda_plus=", neg_lo ? "NEG" : "?"); print_ball(hi);
        printf(" %s\n", neg_hi ? "NEG" : "?");
    }
    if (in != stdin) fclose(in);
    if (seen) for(long i=0;i<expect_symbols;++i) if(!seen[i]) {
        fprintf(stderr,"mode_cert: missing Fourier index %ld\n",i); return 2;
    }
    printf("summary symbols=%ld certified_negative=%ld unresolved=%ld radius=%s prec=%ld\n",
           symbols, certified_negative, unresolved,mode_radii?"per-mode":radius_text,prec);
    if ((expect_symbols>=0 && symbols!=expect_symbols) ||
        (expect_negative>=0 && certified_negative!=expect_negative) ||
        (expect_unresolved>=0 && unresolved!=expect_unresolved)) {
        fprintf(stderr,"mode_cert: asserted certificate counts failed\n");
        return 3;
    }
    if(expect_symbols>=0||expect_negative>=0||expect_unresolved>=0)
        printf("MODE_CERTIFIED expected_symbols=%ld expected_negative=%ld expected_unresolved=%ld\n",
               expect_symbols,expect_negative,expect_unresolved);

    arb_clear(radius); arb_clear(a); arb_clear(d); arb_clear(br); arb_clear(bi);
    arb_clear(trace); arb_clear(delta); arb_clear(disc); arb_clear(root);
    if(mode_radii)_arb_vec_clear(mode_radii,expect_symbols);
    arb_clear(lo); arb_clear(hi); arb_clear(tmp); flint_cleanup();
    free(radius_seen);
    free(seen);
    return 0;
}
