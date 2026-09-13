/* Validated residual majorants for the torsion and material PDEs.

   The input is exported by torsion_hessian.edp -majorant 1
   -export-majorant FILE.  All P1 nodal values are treated as exact decimal
   candidates.  Vertex coordinate balls enclose the exact regular fan mesh.

   Exactly equilibrated fluxes are reconstructed as
       y = -x/2 + curl(psi_w),
       q_{i,c} = -phi_i e_c + curl(psi_{i,c}).
   Since every psi and phi is one global continuous P1 function, these fluxes
   are H(div)-conforming and have exactly the required divergence.  No mixed
   linear-system validation is needed.
*/
#include <flint/arb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { arb_struct x, y; } point_t;
typedef struct {
    long v[3];
    arb_struct area;
    arb_struct gx[3], gy[3];
} tri_t;

static void die(const char *msg)
{
    fprintf(stderr, "majorant_cert: %s\n", msg);
    exit(2);
}

static int read_arb(FILE *in, arb_t x, const arb_t radius, slong prec)
{
    char token[512];
    if (fscanf(in, "%511s", token) != 1 || arb_set_str(x, token, prec)) return 0;
    arb_add_error(x, radius);
    return 1;
}

static arb_ptr read_field(FILE *in, long nv, slong prec)
{
    arb_t zero;
    arb_init(zero); arb_zero(zero);
    arb_ptr f = _arb_vec_init(nv);
    for (long i=0; i<nv; ++i)
        if (!read_arb(in, f+i, zero, prec)) die("truncated field data");
    arb_clear(zero);
    return f;
}

static void grad_field(arb_t gx, arb_t gy, arb_srcptr f,
                       const tri_t *t, slong prec)
{
    arb_zero(gx); arb_zero(gy);
    for (int k=0; k<3; ++k) {
        arb_addmul(gx, f+t->v[k], t->gx+k, prec);
        arb_addmul(gy, f+t->v[k], t->gy+k, prec);
    }
}

/* Add integral_T r^2 for an affine r specified at the three vertices. */
static void add_affine_square(arb_t total, arb_t r[3], const tri_t *t,
                              slong prec)
{
    arb_t s, z;
    arb_init(s); arb_init(z); arb_zero(s);
    for (int i=0; i<3; ++i) {
        arb_sqr(z, r[i], prec); arb_add(s, s, z, prec);
    }
    for (int i=0; i<3; ++i) for (int j=i+1; j<3; ++j) {
        arb_mul(z, r[i], r[j], prec); arb_add(s, s, z, prec);
    }
    arb_mul(s, s, &t->area, prec); arb_div_ui(s, s, 6, prec);
    arb_add(total, total, s, prec);
    arb_clear(s); arb_clear(z);
}

static void energy_norm(arb_t norm, arb_srcptr f, const tri_t *tri,
                        long nt, slong prec)
{
    arb_t sum, gx, gy, z;
    arb_init(sum); arb_init(gx); arb_init(gy); arb_init(z); arb_zero(sum);
    for (long k=0; k<nt; ++k) {
        grad_field(gx, gy, f, tri+k, prec);
        arb_sqr(z, gx, prec); arb_addmul(z, gy, gy, prec);
        arb_addmul(sum, z, &tri[k].area, prec);
    }
    arb_sqrtpos(norm, sum, prec);
    arb_clear(sum); arb_clear(gx); arb_clear(gy); arb_clear(z);
}

static void state_majorant(arb_t delta, arb_srcptr u, arb_srcptr psi,
                            const point_t *p, const tri_t *tri,
                            long nt, slong prec)
{
    arb_t sum, ux, uy, psix, psiy, half, r[3], z;
    arb_init(sum); arb_init(ux); arb_init(uy); arb_init(psix); arb_init(psiy);
    arb_init(half); arb_init(z); for (int i=0;i<3;++i) arb_init(r[i]);
    arb_zero(sum); arb_set_si(half, 1); arb_mul_2exp_si(half, half, -1);
    for (long j=0; j<nt; ++j) {
        grad_field(ux,uy,u,tri+j,prec); grad_field(psix,psiy,psi,tri+j,prec);
        for (int k=0;k<3;++k) {
            arb_mul(z, &p[tri[j].v[k]].x, half, prec);
            arb_neg(r[k], z); arb_add(r[k], r[k], psiy, prec); arb_sub(r[k],r[k],ux,prec);
        }
        add_affine_square(sum,r,tri+j,prec);
        for (int k=0;k<3;++k) {
            arb_mul(z, &p[tri[j].v[k]].y, half, prec);
            arb_neg(r[k], z); arb_sub(r[k], r[k], psix, prec); arb_sub(r[k],r[k],uy,prec);
        }
        add_affine_square(sum,r,tri+j,prec);
    }
    arb_sqrtpos(delta,sum,prec);
    arb_clear(sum); arb_clear(ux); arb_clear(uy); arb_clear(psix); arb_clear(psiy);
    arb_clear(half); arb_clear(z); for (int i=0;i<3;++i) arb_clear(r[i]);
}

/* component=0 for Ux, component=1 for Uy */
static void material_majorant(arb_t delta, int component, arb_srcptr u,
        arb_srcptr phi, arb_srcptr U, arb_srcptr psi,
        const tri_t *tri, long nt, slong prec)
{
    arb_t sum, wx,wy,px,py,Ux,Uy,sx,sy,dot,Gx,Gy,targetx,targety,r[3];
    arb_init(sum); arb_init(wx);arb_init(wy);arb_init(px);arb_init(py);
    arb_init(Ux);arb_init(Uy);arb_init(sx);arb_init(sy);arb_init(dot);
    arb_init(Gx);arb_init(Gy);arb_init(targetx);arb_init(targety);
    for(int i=0;i<3;++i) arb_init(r[i]);
    arb_zero(sum);
    for(long j=0;j<nt;++j) {
        grad_field(wx,wy,u,tri+j,prec); grad_field(px,py,phi,tri+j,prec);
        grad_field(Ux,Uy,U,tri+j,prec); grad_field(sx,sy,psi,tri+j,prec);
        arb_mul(dot,wx,px,prec); arb_addmul(dot,wy,py,prec);
        if(component==0) {
            arb_set(Gx,dot);
            arb_mul(Gy,px,wy,prec); arb_neg(Gy,Gy); arb_addmul(Gy,wx,py,prec);
        } else {
            arb_mul(Gx,py,wx,prec); arb_neg(Gx,Gx); arb_addmul(Gx,wy,px,prec);
            arb_set(Gy,dot);
        }
        arb_sub(targetx,Ux,Gx,prec); arb_sub(targety,Uy,Gy,prec);
        for(int k=0;k<3;++k) {
            arb_set(r[k],sy); arb_sub(r[k],r[k],targetx,prec);
            if(component==0) arb_sub(r[k],r[k],phi+tri[j].v[k],prec);
        }
        add_affine_square(sum,r,tri+j,prec);
        for(int k=0;k<3;++k) {
            arb_neg(r[k],sx); arb_sub(r[k],r[k],targety,prec);
            if(component==1) arb_sub(r[k],r[k],phi+tri[j].v[k],prec);
        }
        add_affine_square(sum,r,tri+j,prec);
    }
    arb_sqrtpos(delta,sum,prec);
    arb_clear(sum);arb_clear(wx);arb_clear(wy);arb_clear(px);arb_clear(py);
    arb_clear(Ux);arb_clear(Uy);arb_clear(sx);arb_clear(sy);arb_clear(dot);
    arb_clear(Gx);arb_clear(Gy);arb_clear(targetx);arb_clear(targety);
    for(int i=0;i<3;++i)arb_clear(r[i]);
}

static void print_ball(const char *name, const arb_t x)
{
    printf("%s=",name); arb_printn(x,18,ARB_STR_MORE); printf("\n");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr,"usage: %s EXPORT [--prec bits] [--geom-radius r]\n",argv[0]);
        return 2;
    }
    const char *path=argv[1], *geom_text="1e-13"; slong prec=160;
    for(int i=2;i<argc;++i) {
        if(!strcmp(argv[i],"--prec") && i+1<argc) prec=atol(argv[++i]);
        else if(!strcmp(argv[i],"--geom-radius") && i+1<argc) geom_text=argv[++i];
        else die("bad option");
    }
    FILE *in=fopen(path,"r"); if(!in){perror(path);return 2;}
    char tag[64]; long n,nv,nt;
    if(fscanf(in,"%63s %ld %ld %ld",tag,&n,&nv,&nt)!=4 ||
       strcmp(tag,"TORSION_MAJORANT_V1") || n<3 || nv<1 || nt<1) die("bad header");
    arb_t geom,zero; arb_init(geom);arb_init(zero);arb_zero(zero);
    if(arb_set_str(geom,geom_text,prec)||arb_is_negative(geom))die("bad geometry radius");
    point_t *p=malloc((size_t)nv*sizeof(point_t));
    tri_t *tri=malloc((size_t)nt*sizeof(tri_t)); if(!p||!tri)die("allocation failed");
    for(long i=0;i<nv;++i){arb_init(&p[i].x);arb_init(&p[i].y);
        if(!read_arb(in,&p[i].x,geom,prec)||!read_arb(in,&p[i].y,geom,prec))die("vertices");}
    for(long j=0;j<nt;++j){
        if(fscanf(in,"%ld %ld %ld",tri[j].v,tri[j].v+1,tri[j].v+2)!=3)die("triangles");
        arb_init(&tri[j].area); for(int k=0;k<3;++k){arb_init(tri[j].gx+k);arb_init(tri[j].gy+k);}
    }
    /* Geometry and P1 basis gradients. */
    arb_t det,a,b,z;arb_init(det);arb_init(a);arb_init(b);arb_init(z);
    for(long j=0;j<nt;++j){
        long i0=tri[j].v[0],i1=tri[j].v[1],i2=tri[j].v[2];
        if(i0<0||i0>=nv||i1<0||i1>=nv||i2<0||i2>=nv)die("triangle index");
        arb_sub(a,&p[i1].x,&p[i0].x,prec);arb_sub(b,&p[i2].y,&p[i0].y,prec);arb_mul(det,a,b,prec);
        arb_sub(a,&p[i2].x,&p[i0].x,prec);arb_sub(b,&p[i1].y,&p[i0].y,prec);arb_submul(det,a,b,prec);
        if(arb_contains_zero(det))die("geometry interval contains degenerate triangle");
        arb_abs(&tri[j].area,det);arb_mul_2exp_si(&tri[j].area,&tri[j].area,-1);
        long ids[3]={i0,i1,i2};
        for(int k=0;k<3;++k){int k1=(k+1)%3,k2=(k+2)%3;
            arb_sub(a,&p[ids[k1]].y,&p[ids[k2]].y,prec);arb_div(tri[j].gx+k,a,det,prec);
            arb_sub(a,&p[ids[k2]].x,&p[ids[k1]].x,prec);arb_div(tri[j].gy+k,a,det,prec);}
    }
    arb_ptr u=read_field(in,nv,prec), psiw=read_field(in,nv,prec);
    arb_ptr *phi=malloc(n*sizeof(arb_ptr)),*ux=malloc(n*sizeof(arb_ptr)),*uy=malloc(n*sizeof(arb_ptr));
    arb_ptr *psix=malloc(n*sizeof(arb_ptr)),*psiy=malloc(n*sizeof(arb_ptr));
    if(!phi||!ux||!uy||!psix||!psiy)die("allocation failed");
    for(long i=0;i<n;++i){phi[i]=read_field(in,nv,prec);ux[i]=read_field(in,nv,prec);
        uy[i]=read_field(in,nv,prec);psix[i]=read_field(in,nv,prec);psiy[i]=read_field(in,nv,prec);}
    fclose(in);

    arb_t dw,gradw,flux,du,csc,theta,pi,sin_theta,area,umax,dumax,tmp,tmp2;
    arb_init(dw);arb_init(gradw);arb_init(flux);arb_init(du);arb_init(csc);
    arb_init(theta);arb_init(pi);arb_init(sin_theta);arb_init(area);arb_init(umax);
    arb_init(dumax);arb_init(tmp);arb_init(tmp2);arb_zero(umax);arb_zero(dumax);
    state_majorant(dw,u,psiw,p,tri,nt,prec);energy_norm(gradw,u,tri,nt,prec);
    print_ball("delta_w",dw);print_ball("grad_w_h",gradw);
    arb_const_pi(pi,prec);arb_mul_ui(theta,pi,2,prec);arb_div_ui(theta,theta,n,prec);
    arb_sin(sin_theta,theta,prec);arb_inv(csc,sin_theta,prec);
    for(long i=0;i<n;++i){
        material_majorant(flux,0,u,phi[i],ux[i],psix[i],tri,nt,prec);
        arb_mul(du,csc,dw,prec);arb_add(du,du,flux,prec);
        printf("vertex=%ld component=x flux=",i);arb_printn(flux,15,ARB_STR_MORE);
        printf(" delta_U<=");arb_printn(du,15,ARB_STR_MORE);printf("\n");arb_max(dumax,dumax,du,prec);
        energy_norm(tmp,ux[i],tri,nt,prec);arb_max(umax,umax,tmp,prec);
        material_majorant(flux,1,u,phi[i],uy[i],psiy[i],tri,nt,prec);
        arb_mul(du,csc,dw,prec);arb_add(du,du,flux,prec);
        printf("vertex=%ld component=y flux=",i);arb_printn(flux,15,ARB_STR_MORE);
        printf(" delta_U<=");arb_printn(du,15,ARB_STR_MORE);printf("\n");arb_max(dumax,dumax,du,prec);
        energy_norm(tmp,uy[i],tri,nt,prec);arb_max(umax,umax,tmp,prec);
    }
    print_ball("max_delta_U",dumax);print_ball("max_grad_U_h",umax);
    /* Same conservative entrywise propagation as the FreeFEM benchmark. */
    arb_mul_ui(area,sin_theta,n,prec);arb_mul_2exp_si(area,area,-1);
    arb_t qerr,CF,phi2,gram,geomerr,dJ,hj,hf,symbol,jlower;
    arb_init(qerr);arb_init(CF);arb_init(phi2);arb_init(gram);arb_init(geomerr);
    arb_init(dJ);arb_init(hj);arb_init(hf);arb_init(symbol);arb_init(jlower);
    arb_mul_ui(tmp,gradw,2,prec);arb_add(tmp,tmp,dw,prec);arb_mul(qerr,dw,tmp,prec);
    arb_set_str(jlower,"2.4",prec);arb_sqrt(CF,area,prec);arb_sqrt(tmp,pi,prec);
    arb_mul(tmp,tmp,jlower,prec);arb_div(CF,CF,tmp,prec);
    arb_sqr(phi2,csc,prec);
    arb_mul(gram,dumax,umax,prec);arb_mul_2exp_si(gram,gram,1);arb_addmul(gram,dumax,dumax,prec);
    arb_mul(geomerr,phi2,qerr,prec);arb_mul_2exp_si(geomerr,geomerr,1);
    arb_sqrt(tmp,area,prec);arb_mul(tmp,tmp,CF,prec);arb_mul(tmp,tmp,dw,prec);
    arb_mul(tmp,tmp,phi2,prec);arb_mul_2exp_si(tmp,tmp,1);arb_add(geomerr,geomerr,tmp,prec);
    arb_add(hj,gram,geomerr,prec);arb_sqr(dJ,dw,prec);arb_mul_2exp_si(dJ,dJ,-1);
    arb_sqr(tmp,area,prec);arb_div(hf,hj,tmp,prec);
    arb_mul(tmp,tmp,area,prec);arb_div(tmp2,dJ,tmp,prec);arb_add(hf,hf,tmp2,prec);
    arb_mul(tmp,tmp,area,prec);arb_sqr(tmp2,sin_theta,prec);arb_mul(tmp2,tmp2,dJ,prec);
    arb_mul_2exp_si(tmp2,tmp2,1);arb_div(tmp2,tmp2,tmp,prec);arb_add(hf,hf,tmp2,prec);
    arb_mul_ui(symbol,hf,n,prec);arb_sqrt_ui(tmp,2,prec);arb_mul(symbol,symbol,tmp,prec);
    print_ball("crude_hessJ_entry",hj);print_ball("crude_hessF_entry",hf);
    print_ball("crude_symbol_entry_radius",symbol);

    for(long i=0;i<nv;++i){arb_clear(&p[i].x);arb_clear(&p[i].y);}free(p);
    for(long j=0;j<nt;++j){arb_clear(&tri[j].area);for(int k=0;k<3;++k){arb_clear(tri[j].gx+k);arb_clear(tri[j].gy+k);}}free(tri);
    _arb_vec_clear(u,nv);_arb_vec_clear(psiw,nv);for(long i=0;i<n;++i){_arb_vec_clear(phi[i],nv);
      _arb_vec_clear(ux[i],nv);_arb_vec_clear(uy[i],nv);_arb_vec_clear(psix[i],nv);_arb_vec_clear(psiy[i],nv);}
    free(phi);free(ux);free(uy);free(psix);free(psiy);
    flint_cleanup();
    return 0;
}
