/* Arb validation of a modewise second-variation lifting export.

   Candidate fluxes are
     -x/2 + curl(psi_0),
     -theta_q + curl(psi_q),
     -theta_r + curl(psi_r),
     -((div theta_q) theta_r - D theta_q theta_r) + curl(psi_z).
   Their divergences equal the four volume sources exactly.  The output is
   the second-variation residual bound (eq:second-residual-bound), including
   the algebraic corrections in eq:complete-F-radius of the manuscript.
*/
#include <flint/arb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

typedef struct { arb_struct x, y; } point_t;
typedef struct { long v[3]; arb_struct area, gx[3], gy[3]; } tri_t;
typedef struct { long a, b; } edge_t;
typedef struct { long a, b, c; } tri_key_t;

static void die(const char *s) { fprintf(stderr,"second_lifting_cert: %s\n",s); exit(2); }

static int cmp_edge(const void *aa,const void *bb)
{
    const edge_t *a=aa,*b=bb;
    if(a->a!=b->a)return a->a<b->a?-1:1;
    return a->b==b->b?0:(a->b<b->b?-1:1);
}

static int cmp_tri_key(const void *aa,const void *bb)
{
    const tri_key_t *a=aa,*b=bb;
    if(a->a!=b->a)return a->a<b->a?-1:1;
    if(a->b!=b->b)return a->b<b->b?-1:1;
    return a->c==b->c?0:(a->c<b->c?-1:1);
}

static long dsu_root(long *parent,long i)
{
    while(parent[i]!=i){parent[i]=parent[parent[i]];i=parent[i];}
    return i;
}

static void dsu_join(long *parent,long a,long b)
{
    a=dsu_root(parent,a);b=dsu_root(parent,b);if(a!=b)parent[b]=a;
}

/* Check connectivity, edge multiplicities, boundary degrees, Euler identity,
   and regular-fan counts without using coordinates.  The later canonical-
   incidence comparison establishes that the mesh is the prescribed fan
   disk, and that this boundary mask is its exact Dirichlet-node set. */
static unsigned char *certify_mesh_topology(const tri_t *tri,long nt,long nv,
                                             long n,long m,int strict_regular)
{
    if(strict_regular&&(nt!=n*m*m || nv!=1+n*m*(m+1)/2))die("unexpected regular-fan size");
    edge_t *edges=malloc((size_t)(3*nt)*sizeof(edge_t));
    tri_key_t *keys=malloc((size_t)nt*sizeof(tri_key_t));
    long *parent=malloc((size_t)nv*sizeof(long));
    long *boundary_degree=calloc((size_t)nv,sizeof(long));
    unsigned char *used=calloc((size_t)nv,1),*boundary=calloc((size_t)nv,1);
    if(!edges||!keys||!parent||!boundary_degree||!used||!boundary)
        die("topology allocation");
    for(long i=0;i<nv;++i)parent[i]=i;
    for(long j=0;j<nt;++j){
        long a=tri[j].v[0],b=tri[j].v[1],c=tri[j].v[2],s;
        if(a<0||a>=nv||b<0||b>=nv||c<0||c>=nv||a==b||b==c||c==a)
            die("invalid triangle connectivity");
        used[a]=used[b]=used[c]=1;dsu_join(parent,a,b);dsu_join(parent,b,c);
        long v[3]={a,b,c};
        if(v[0]>v[1]){s=v[0];v[0]=v[1];v[1]=s;}
        if(v[1]>v[2]){s=v[1];v[1]=v[2];v[2]=s;}
        if(v[0]>v[1]){s=v[0];v[0]=v[1];v[1]=s;}
        keys[j]=(tri_key_t){v[0],v[1],v[2]};
        long e[3][2]={{a,b},{b,c},{c,a}};
        for(int t=0;t<3;++t){if(e[t][0]>e[t][1]){s=e[t][0];e[t][0]=e[t][1];e[t][1]=s;}
            edges[3*j+t]=(edge_t){e[t][0],e[t][1]};}
    }
    qsort(keys,(size_t)nt,sizeof(tri_key_t),cmp_tri_key);
    for(long j=1;j<nt;++j)if(!cmp_tri_key(keys+j-1,keys+j))die("duplicate triangle");
    long root=dsu_root(parent,0);for(long i=0;i<nv;++i)
        if(!used[i]||dsu_root(parent,i)!=root)die("disconnected or unused mesh vertex");
    qsort(edges,(size_t)(3*nt),sizeof(edge_t),cmp_edge);
    long ne=0,nbe=0;
    for(long i=0;i<3*nt;){long j=i+1;while(j<3*nt&&!cmp_edge(edges+i,edges+j))++j;
        long multiplicity=j-i;if(multiplicity!=1&&multiplicity!=2)die("nonmanifold mesh edge");
        ++ne;if(multiplicity==1){long a=edges[i].a,b=edges[i].b;++nbe;
            boundary[a]=boundary[b]=1;++boundary_degree[a];++boundary_degree[b];}i=j;}
    long nbv=0;for(long i=0;i<nv;++i)if(boundary[i]){++nbv;if(boundary_degree[i]!=2)die("boundary is not one-cycle regular");}
    if(nv-ne+nt!=1)die("mesh Euler check failed");
    if(strict_regular&&(nbe!=n*m||nbv!=n*m))die("regular-fan boundary check failed");
    printf("exact_mesh_topology=PASS vertices=%ld triangles=%ld edges=%ld boundary_vertices=%ld\n",nv,nt,ne,nbv);
    if(!strict_regular)memset(boundary,0,(size_t)nv);
    free(edges);free(keys);free(parent);free(boundary_degree);free(used);return boundary;
}

static int read_ball(FILE *in, arb_t x, const arb_t radius, slong prec)
{
    char s[512];
    if(fscanf(in,"%511s",s)!=1 || arb_set_str(x,s,prec) || !arb_is_finite(x)) return 0;
    arb_add_error(x,radius); return 1;
}

static arb_ptr read_field(FILE *in,long n,const arb_t radius,slong prec)
{
    arb_ptr f=_arb_vec_init(n);
    for(long i=0;i<n;++i) if(!read_ball(in,f+i,radius,prec))die("truncated field");
    return f;
}

static void gradient(arb_t x,arb_t y,arb_srcptr f,const tri_t *t,slong prec)
{
    arb_zero(x);arb_zero(y);
    for(int i=0;i<3;++i){arb_addmul(x,f+t->v[i],t->gx+i,prec);arb_addmul(y,f+t->v[i],t->gy+i,prec);}
}

static void add_affine_square(arb_t total,arb_t v[3],const tri_t *t,slong prec)
{
    arb_t s,z;arb_init(s);arb_init(z);arb_zero(s);
    for(int i=0;i<3;++i){arb_sqr(z,v[i],prec);arb_add(s,s,z,prec);}
    for(int i=0;i<3;++i)for(int j=i+1;j<3;++j){arb_mul(z,v[i],v[j],prec);arb_add(s,s,z,prec);}
    arb_mul(s,s,&t->area,prec);arb_div_ui(s,s,6,prec);arb_add(total,total,s,prec);
    arb_clear(s);arb_clear(z);
}

static void affine_integral(arb_t out,arb_srcptr f,const tri_t *t,slong prec)
{
    arb_zero(out);
    for(int i=0;i<3;++i) arb_add(out,out,f+t->v[i],prec);
    arb_mul(out,out,&t->area,prec);arb_div_ui(out,out,3,prec);
}

static void assemble_triangle_geometry(const point_t *p,tri_t *tri,long nt,
                                       long nv,slong prec)
{
    arb_t det,x,y;arb_init(det);arb_init(x);arb_init(y);
    for(long j=0;j<nt;++j){
        long i0=tri[j].v[0],i1=tri[j].v[1],i2=tri[j].v[2];
        if(i0<0||i0>=nv||i1<0||i1>=nv||i2<0||i2>=nv)die("triangle index");
        arb_sub(x,&p[i1].x,&p[i0].x,prec);arb_sub(y,&p[i2].y,&p[i0].y,prec);arb_mul(det,x,y,prec);
        arb_sub(x,&p[i2].x,&p[i0].x,prec);arb_sub(y,&p[i1].y,&p[i0].y,prec);arb_submul(det,x,y,prec);
        if(arb_contains_zero(det))die("degenerate geometry");
        arb_abs(&tri[j].area,det);arb_mul_2exp_si(&tri[j].area,&tri[j].area,-1);
        long ids[3]={i0,i1,i2};
        for(int i=0;i<3;++i){int i1a=(i+1)%3,i2a=(i+2)%3;
            arb_sub(x,&p[ids[i1a]].y,&p[ids[i2a]].y,prec);arb_div(tri[j].gx+i,x,det,prec);
            arb_sub(x,&p[ids[i2a]].x,&p[ids[i1a]].x,prec);arb_div(tri[j].gy+i,x,det,prec);
        }
    }
    arb_clear(det);arb_clear(x);arb_clear(y);
}

static void symmetric_norm(arb_t out,const arb_t x,const arb_t y,const arb_t z,slong prec)
{
    arb_t t,d,s;arb_init(t);arb_init(d);arb_init(s);
    arb_add(t,x,z,prec);arb_abs(t,t);
    arb_sub(d,x,z,prec);arb_sqr(d,d,prec);arb_sqr(s,y,prec);arb_mul_ui(s,s,4,prec);
    arb_add(d,d,s,prec);arb_sqrtpos(d,d,prec);arb_add(out,t,d,prec);arb_mul_2exp_si(out,out,-1);
    arb_clear(t);arb_clear(d);arb_clear(s);
}

/* Matrices A_q, A_r, A_qr for det(F)F^{-1}F^{-T} at F=I. */
static void coefficients(arb_t Aq[3],arb_t Ar[3],arb_t Aqr[3],arb_t dqr,
                         arb_t Q[4],arb_t R[4],slong prec)
{
    arb_t tq,tr,s,tmp,u;arb_init(tq);arb_init(tr);arb_init(s);arb_init(tmp);arb_init(u);
    arb_add(tq,Q[0],Q[3],prec);arb_add(tr,R[0],R[3],prec);
    arb_mul_2exp_si(tmp,Q[0],1);arb_sub(Aq[0],tq,tmp,prec);
    arb_add(Aq[1],Q[1],Q[2],prec);arb_neg(Aq[1],Aq[1]);
    arb_mul_2exp_si(tmp,Q[3],1);arb_sub(Aq[2],tq,tmp,prec);
    arb_mul_2exp_si(tmp,R[0],1);arb_sub(Ar[0],tr,tmp,prec);
    arb_add(Ar[1],R[1],R[2],prec);arb_neg(Ar[1],Ar[1]);
    arb_mul_2exp_si(tmp,R[3],1);arb_sub(Ar[2],tr,tmp,prec);
    arb_mul(dqr,tq,tr,prec);
    arb_mul(tmp,Q[0],R[0],prec);arb_sub(dqr,dqr,tmp,prec);
    arb_mul(tmp,Q[1],R[2],prec);arb_sub(dqr,dqr,tmp,prec);
    arb_mul(tmp,Q[2],R[1],prec);arb_sub(dqr,dqr,tmp,prec);
    arb_mul(tmp,Q[3],R[3],prec);arb_sub(dqr,dqr,tmp,prec);

    /* S11=6ae+2(bg+fc+bf), S22=6dh+2(cf+bg+cg). */
    arb_mul(s,Q[0],R[0],prec);arb_mul_ui(s,s,6,prec);
    arb_mul(tmp,Q[1],R[2],prec);arb_addmul_ui(s,tmp,2,prec);
    arb_mul(tmp,R[1],Q[2],prec);arb_addmul_ui(s,tmp,2,prec);
    arb_mul(tmp,Q[1],R[1],prec);arb_addmul_ui(s,tmp,2,prec);
    arb_set(Aqr[0],dqr);arb_mul(tmp,tq,R[0],prec);arb_mul_2exp_si(tmp,tmp,1);arb_sub(Aqr[0],Aqr[0],tmp,prec);
    arb_mul(tmp,tr,Q[0],prec);arb_mul_2exp_si(tmp,tmp,1);arb_sub(Aqr[0],Aqr[0],tmp,prec);arb_add(Aqr[0],Aqr[0],s,prec);
    arb_mul(s,Q[3],R[3],prec);arb_mul_ui(s,s,6,prec);
    arb_mul(tmp,Q[2],R[1],prec);arb_addmul_ui(s,tmp,2,prec);
    arb_mul(tmp,Q[1],R[2],prec);arb_addmul_ui(s,tmp,2,prec);
    arb_mul(tmp,Q[2],R[2],prec);arb_addmul_ui(s,tmp,2,prec);
    arb_set(Aqr[2],dqr);arb_mul(tmp,tq,R[3],prec);arb_mul_2exp_si(tmp,tmp,1);arb_sub(Aqr[2],Aqr[2],tmp,prec);
    arb_mul(tmp,tr,Q[3],prec);arb_mul_2exp_si(tmp,tmp,1);arb_sub(Aqr[2],Aqr[2],tmp,prec);arb_add(Aqr[2],Aqr[2],s,prec);

    arb_zero(s);
    arb_mul(tmp,Q[0],R[1],prec);arb_add(s,s,tmp,prec);
    arb_mul(tmp,Q[1],R[3],prec);arb_addmul_ui(s,tmp,2,prec);
    arb_mul(tmp,R[0],Q[1],prec);arb_add(s,s,tmp,prec);
    arb_mul(tmp,R[1],Q[3],prec);arb_addmul_ui(s,tmp,2,prec);
    arb_mul(tmp,Q[2],R[0],prec);arb_addmul_ui(s,tmp,2,prec);
    arb_mul(tmp,Q[3],R[2],prec);arb_add(s,s,tmp,prec);
    arb_mul(tmp,R[2],Q[0],prec);arb_addmul_ui(s,tmp,2,prec);
    arb_mul(tmp,R[3],Q[2],prec);arb_add(s,s,tmp,prec);
    arb_add(u,R[1],R[2],prec);arb_mul(tmp,tq,u,prec);arb_neg(Aqr[1],tmp);
    arb_add(u,Q[1],Q[2],prec);arb_mul(tmp,tr,u,prec);arb_sub(Aqr[1],Aqr[1],tmp,prec);arb_add(Aqr[1],Aqr[1],s,prec);
    arb_clear(tq);arb_clear(tr);arb_clear(s);arb_clear(tmp);arb_clear(u);
}

static void state_error(arb_t out,arb_srcptr u,arb_srcptr psi,const point_t *p,
                        const tri_t *tri,long nt,slong prec)
{
    arb_t sum,ux,uy,sx,sy,half,tmp,v[3];arb_init(sum);arb_init(ux);arb_init(uy);
    arb_init(sx);arb_init(sy);arb_init(half);arb_init(tmp);for(int i=0;i<3;++i)arb_init(v[i]);
    arb_zero(sum);arb_one(half);arb_mul_2exp_si(half,half,-1);
    for(long j=0;j<nt;++j){gradient(ux,uy,u,tri+j,prec);gradient(sx,sy,psi,tri+j,prec);
      for(int i=0;i<3;++i){arb_mul(tmp,&p[tri[j].v[i]].x,half,prec);arb_neg(v[i],tmp);arb_add(v[i],v[i],sy,prec);arb_sub(v[i],v[i],ux,prec);}add_affine_square(sum,v,tri+j,prec);
      for(int i=0;i<3;++i){arb_mul(tmp,&p[tri[j].v[i]].y,half,prec);arb_neg(v[i],tmp);arb_sub(v[i],v[i],sx,prec);arb_sub(v[i],v[i],uy,prec);}add_affine_square(sum,v,tri+j,prec);}
    arb_sqrtpos(out,sum,prec);arb_clear(sum);arb_clear(ux);arb_clear(uy);arb_clear(sx);arb_clear(sy);arb_clear(half);arb_clear(tmp);for(int i=0;i<3;++i)arb_clear(v[i]);
}

static long canonical_gid(long n,long m,long s,long a,long b)
{
    s=(s%n+n)%n;if(a==0&&b==0)return 0;
    if(b==0)return 1+s*m+(a-1);
    if(a==0){s=(s+1)%n;return 1+s*m+(b-1);}
    long per=m*(m-1)/2,r=a+b;
    return 1+n*m+s*per+(r-2)*(r-1)/2+(a-1);
}

static tri_key_t canonical_tri_key(long a,long b,long c)
{
    long s;if(a>b){s=a;a=b;b=s;}if(b>c){s=b;b=c;c=s;}if(a>b){s=a;a=b;b=s;}
    return (tri_key_t){a,b,c};
}

/* Prove that the decimal mesh/direction balls contain the exact regular-fan
   lattice and the exact normalized real Fourier directions. */
static void validate_regular_inputs(long n,long m,long k_mode,long q_axis,long q_phase,
        long r_axis,long r_phase,point_t *p,long nv,arb_ptr qx,arb_ptr qy,
        arb_ptr rx,arb_ptr ry,tri_t *tri,long nt,
        const unsigned char *boundary,slong prec)
{
    point_t *v=malloc((size_t)n*sizeof(point_t));
    point_t *qv=malloc((size_t)n*sizeof(point_t));
    point_t *rv=malloc((size_t)n*sizeof(point_t));
    arb_ptr pq=_arb_vec_init(n),pr=_arb_vec_init(n);
    long *gid=malloc((size_t)nv*sizeof(long));unsigned char *seen=calloc((size_t)nv,1);
    if(!v||!qv||!rv||!gid||!seen)die("input validation allocation");
    arb_t pi,theta,angle,nq,nr,tmp,cand,expect,small_area,total_area,polygon_area;
    arb_init(pi);arb_init(theta);arb_init(angle);arb_init(nq);arb_init(nr);
    arb_init(tmp);arb_init(cand);arb_init(expect);arb_init(small_area);arb_init(total_area);arb_init(polygon_area);arb_const_pi(pi,prec);
    arb_mul_ui(theta,pi,2,prec);arb_div_ui(theta,theta,n,prec);arb_zero(nq);arb_zero(nr);
    for(long s=0;s<n;++s){
        arb_init(&v[s].x);arb_init(&v[s].y);arb_init(&qv[s].x);arb_init(&qv[s].y);arb_init(&rv[s].x);arb_init(&rv[s].y);
        arb_mul_ui(angle,theta,s,prec);arb_cos(&v[s].x,angle,prec);arb_sin(&v[s].y,angle,prec);
        arb_mul_ui(angle,angle,k_mode,prec);
        if(q_phase==0)arb_cos(pq+s,angle,prec);else arb_sin(pq+s,angle,prec);
        if(r_phase==0)arb_cos(pr+s,angle,prec);else arb_sin(pr+s,angle,prec);
        arb_addmul(nq,pq+s,pq+s,prec);arb_addmul(nr,pr+s,pr+s,prec);
    }
    if(arb_contains_zero(nq)||arb_contains_zero(nr))die("zero exact Fourier direction");
    arb_sqrtpos(nq,nq,prec);arb_sqrtpos(nr,nr,prec);
    for(long s=0;s<n;++s){
        arb_div(tmp,pq+s,nq,prec);
        if(q_axis==0){arb_mul(&qv[s].x,tmp,&v[s].x,prec);arb_mul(&qv[s].y,tmp,&v[s].y,prec);}
        else{arb_mul(&qv[s].x,tmp,&v[s].y,prec);arb_neg(&qv[s].x,&qv[s].x);arb_mul(&qv[s].y,tmp,&v[s].x,prec);}
        arb_div(tmp,pr+s,nr,prec);
        if(r_axis==0){arb_mul(&rv[s].x,tmp,&v[s].x,prec);arb_mul(&rv[s].y,tmp,&v[s].y,prec);}
        else{arb_mul(&rv[s].x,tmp,&v[s].y,prec);arb_neg(&rv[s].x,&rv[s].x);arb_mul(&rv[s].y,tmp,&v[s].x,prec);}
    }
    double *nom_x=malloc((size_t)n*sizeof(double));
    double *nom_y=malloc((size_t)n*sizeof(double));
    if(!nom_x||!nom_y)die("lattice nomination allocation");
    for(long s=0;s<n;++s){
        nom_x[s]=arf_get_d(arb_midref(&v[s].x),ARF_RND_NEAR);
        nom_y[s]=arf_get_d(arb_midref(&v[s].y),ARF_RND_NEAR);
    }
    double maxdist=0.0;
    for(long ip=0;ip<nv;++ip){
        double pxm=arf_get_d(arb_midref(&p[ip].x),ARF_RND_NEAR),pym=arf_get_d(arb_midref(&p[ip].y),ARF_RND_NEAR);
        double best=1e300;long bs=-1,bi=0,bj=0;
        /* This floating calculation only nominates integer coordinates.
           Invert each sector's two vertex vectors instead of searching its
           entire lattice. Nearby integer pairs cover rounding at edges.
           Acceptance still requires all Arb containment, node-bijection,
           boundary, and canonical-incidence checks below. */
        for(long s=0;s<n;++s){
            long sp=(s+1)%n;
            double ax=nom_x[s],ay=nom_y[s],bx=nom_x[sp],by=nom_y[sp];
            double det=ax*by-ay*bx;
            double fa=m*(pxm*by-pym*bx)/det,fb=m*(ax*pym-ay*pxm)/det;
            if(!isfinite(fa)||!isfinite(fb)||fa < -2.0||fb < -2.0||
               fa > (double)m+2.0||fb > (double)m+2.0)continue;
            long ca=lround(fa),cb=lround(fb);
            for(long ia=ca-1;ia<=ca+1;++ia)for(long ib=cb-1;ib<=cb+1;++ib){
                if(ia<0||ib<0||ia>m||ib>m-ia)continue;
                double cx=(ia*ax+ib*bx)/m,cy=(ia*ay+ib*by)/m;
                double dd=(cx-pxm)*(cx-pxm)+(cy-pym)*(cy-pym);
                if(dd<best){best=dd;bs=s;bi=ia;bj=ib;}
            }
        }
        if(bs<0)die("cannot nominate regular-fan lattice coordinates");
        if(sqrt(best)>maxdist) maxdist=sqrt(best);
        long sp=(bs+1)%n;
        arb_mul_si(cand,&v[bs].x,bi,prec);arb_addmul_si(cand,&v[sp].x,bj,prec);arb_div_ui(cand,cand,m,prec);if(!arb_contains(&p[ip].x,cand))die("geometry radius misses exact fan x");arb_set(&p[ip].x,cand);
        arb_mul_si(cand,&v[bs].y,bi,prec);arb_addmul_si(cand,&v[sp].y,bj,prec);arb_div_ui(cand,cand,m,prec);if(!arb_contains(&p[ip].y,cand))die("geometry radius misses exact fan y");arb_set(&p[ip].y,cand);
        arb_mul_si(expect,&qv[bs].x,bi,prec);arb_addmul_si(expect,&qv[sp].x,bj,prec);arb_div_ui(expect,expect,m,prec);if(!arb_contains(qx+ip,expect))die("direction radius misses qx");arb_set(qx+ip,expect);
        arb_mul_si(expect,&qv[bs].y,bi,prec);arb_addmul_si(expect,&qv[sp].y,bj,prec);arb_div_ui(expect,expect,m,prec);if(!arb_contains(qy+ip,expect))die("direction radius misses qy");arb_set(qy+ip,expect);
        arb_mul_si(expect,&rv[bs].x,bi,prec);arb_addmul_si(expect,&rv[sp].x,bj,prec);arb_div_ui(expect,expect,m,prec);if(!arb_contains(rx+ip,expect))die("direction radius misses rx");arb_set(rx+ip,expect);
        arb_mul_si(expect,&rv[bs].y,bi,prec);arb_addmul_si(expect,&rv[sp].y,bj,prec);arb_div_ui(expect,expect,m,prec);if(!arb_contains(ry+ip,expect))die("direction radius misses ry");arb_set(ry+ip,expect);
        gid[ip]=canonical_gid(n,m,bs,bi,bj);if(gid[ip]<0||gid[ip]>=nv||seen[gid[ip]])die("regular-fan lattice map is not bijective");seen[gid[ip]]=1;
        int expected_boundary=(bi==0&&bj==m)||(bj==0&&bi==m)||(bi>0&&bj>0&&bi+bj==m);
        if((int)boundary[ip]!=expected_boundary)die("topological and exact-lattice boundaries disagree");
    }
    free(nom_x);free(nom_y);
    for(long i=0;i<nv;++i)if(!seen[i])die("regular-fan lattice node missing");
    /* Compare the exported incidence, after the proved node bijection, with
       the canonical two-orientation triangulation of every sector. */
    tri_key_t *actual=malloc((size_t)nt*sizeof(tri_key_t)),*expected=malloc((size_t)nt*sizeof(tri_key_t));
    if(!actual||!expected)die("incidence validation allocation");
    for(long j=0;j<nt;++j)actual[j]=canonical_tri_key(gid[tri[j].v[0]],gid[tri[j].v[1]],gid[tri[j].v[2]]);
    long pos=0;for(long s=0;s<n;++s){
        for(long a=0;a<=m-1;++a)for(long b=0;b<=m-1-a;++b)
            expected[pos++]=canonical_tri_key(canonical_gid(n,m,s,a,b),canonical_gid(n,m,s,a+1,b),canonical_gid(n,m,s,a,b+1));
        for(long a=0;a<=m-2;++a)for(long b=0;b<=m-2-a;++b)
            expected[pos++]=canonical_tri_key(canonical_gid(n,m,s,a+1,b),canonical_gid(n,m,s,a+1,b+1),canonical_gid(n,m,s,a,b+1));
    }
    if(pos!=nt)die("canonical triangle count");
    qsort(actual,(size_t)nt,sizeof(tri_key_t),cmp_tri_key);
    qsort(expected,(size_t)nt,sizeof(tri_key_t),cmp_tri_key);
    for(long j=0;j<nt;++j)if(cmp_tri_key(actual+j,expected+j))die("exported mesh is not the canonical regular fan");
    /* Replace the exported geometry by the proved analytic fan and assemble
       its element data directly from Arb trigonometric balls. */
    assemble_triangle_geometry(p,tri,nt,nv,prec);
    /* Every element of the symmetric m-by-m fan is congruent.  Its area and
       the total polygon area are analytic functions of pi and are checked
       against the independently reconstructed triangle geometry. */
    arb_sin(small_area,theta,prec);arb_div_ui(small_area,small_area,2*(ulong)m*(ulong)m,prec);
    arb_zero(total_area);for(long j=0;j<nt;++j){if(!arb_contains(&tri[j].area,small_area))die("noncongruent regular-fan triangle area");arb_add(total_area,total_area,&tri[j].area,prec);}
    arb_mul_ui(polygon_area,small_area,n*(ulong)m*(ulong)m,prec);
    if(!arb_contains(total_area,polygon_area))die("mesh area misses exact regular polygon");
    printf("exact_regular_fan_inputs=PASS max_midpoint_distance=%.3e\n",maxdist);
    for(long s=0;s<n;++s){arb_clear(&v[s].x);arb_clear(&v[s].y);arb_clear(&qv[s].x);arb_clear(&qv[s].y);arb_clear(&rv[s].x);arb_clear(&rv[s].y);}free(v);free(qv);free(rv);free(gid);free(seen);free(actual);free(expected);_arb_vec_clear(pq,n);_arb_vec_clear(pr,n);
    arb_clear(pi);arb_clear(theta);arb_clear(angle);arb_clear(nq);arb_clear(nr);arb_clear(tmp);arb_clear(cand);arb_clear(expect);arb_clear(small_area);arb_clear(total_area);arb_clear(polygon_area);
}

static void interior_residual_norm(arb_t out,arb_srcptr residual,
                                   const unsigned char *boundary,long nv,slong prec)
{
    arb_t sum;arb_init(sum);arb_zero(sum);
    for(long i=0;i<nv;++i)if(!boundary[i])arb_addmul(sum,residual+i,residual+i,prec);
    arb_sqrtpos(out,sum,prec);arb_clear(sum);
}

static void read_mode_reference(arb_t out,const char *path,long wanted_k,
                                const char *entry,slong prec)
{
    FILE *in=fopen(path,"r");if(!in)die("cannot open reference mode file");
    char tag[64],sa[256],sd[256],sbr[256],sbi[256];long k;int found=0;
    while(fscanf(in,"%63s",tag)==1){
        if(strcmp(tag,"MODE_DATA")){char discard[2048];if(!fgets(discard,sizeof(discard),in))break;continue;}
        if(fscanf(in,"%ld %255s %255s %255s %255s",&k,sa,sd,sbr,sbi)!=5)
            die("malformed reference MODE_DATA");
        if(k==wanted_k){const char *s=!strcmp(entry,"rr")?sa:!strcmp(entry,"tt")?sd:!strcmp(entry,"rt_re")?sbr:!strcmp(entry,"rt_im")?sbi:NULL;
            if(!s||arb_set_str(out,s,prec)||!arb_is_finite(out))die("bad reference entry");
            found=1;break;}
    }
    fclose(in);if(!found)die("missing reference mode");
}

int main(int argc,char **argv)
{
    if(argc<2){fprintf(stderr,"usage: %s EXPORT [--prec bits] [--geom-radius r] [--direction-radius r] (--check-regular-inputs | --diagnostic-nonstrict) [--reference FILE --entry rr|tt|rt_re|rt_im --max-radius r]\n",argv[0]);return 2;}
    const char *geom_s="1e-13",*dir_s="1e-13",*reference=NULL,*entry=NULL,*max_radius_s=NULL;slong prec=192;int check_inputs=0,diagnostic=0;
    for(int i=2;i<argc;++i){if(!strcmp(argv[i],"--prec")&&i+1<argc)prec=atol(argv[++i]);
      else if(!strcmp(argv[i],"--geom-radius")&&i+1<argc)geom_s=argv[++i];
      else if(!strcmp(argv[i],"--direction-radius")&&i+1<argc)dir_s=argv[++i];
      else if(!strcmp(argv[i],"--reference")&&i+1<argc)reference=argv[++i];
      else if(!strcmp(argv[i],"--entry")&&i+1<argc)entry=argv[++i];
      else if(!strcmp(argv[i],"--max-radius")&&i+1<argc)max_radius_s=argv[++i];
      else if(!strcmp(argv[i],"--check-regular-inputs"))check_inputs=1;
      else if(!strcmp(argv[i],"--diagnostic-nonstrict"))diagnostic=1;else die("bad option");}
    if(prec<32)die("precision must be at least 32 bits");
    if(check_inputs==diagnostic)die("choose exactly one of --check-regular-inputs and --diagnostic-nonstrict");
    if((reference||entry||max_radius_s)&&!(reference&&entry&&max_radius_s&&check_inputs))
        die("reference certification requires --reference, --entry, --max-radius, and --check-regular-inputs");
    FILE *in=fopen(argv[1],"r");if(!in){perror(argv[1]);return 2;}
    char tag[64];long n,m,k_mode,q_axis,q_phase,r_axis,r_phase,nv,nt;
    if(fscanf(in,"%63s %ld %ld %ld %ld %ld %ld %ld %ld %ld",tag,&n,&m,&k_mode,&q_axis,&q_phase,&r_axis,&r_phase,&nv,&nt)!=10||strcmp(tag,"TORSION_SECOND_V2")||n<3||m<1||nv<1||nt<1||k_mode<0||k_mode>=n||(q_axis!=0&&q_axis!=1)||(r_axis!=0&&r_axis!=1)||(q_phase!=0&&q_phase!=1)||(r_phase!=0&&r_phase!=1))die("bad header");
    if(m>LONG_MAX/m)die("regular-fan dimensions overflow");
    long mm=m*m,per_nodes=m*(m+1)/2;
    if(n>LONG_MAX/mm||n>(LONG_MAX-1)/per_nodes||n*mm>LONG_MAX/3)die("regular-fan dimensions overflow");
    if(check_inputs&&(nt!=n*mm||nv!=1+n*per_nodes))die("bad regular-fan dimensions");
    if(k_mode==0&&(q_axis==0||r_axis==0))
        die("scale Hessian formula requires vanishing first area variations");
    if(reference){
        int metadata_ok=(!strcmp(entry,"rr")&&q_axis==0&&q_phase==0&&r_axis==0&&r_phase==0)
          ||(!strcmp(entry,"tt")&&q_axis==1&&q_phase==0&&r_axis==1&&r_phase==0)
          ||(!strcmp(entry,"rt_re")&&q_axis==0&&q_phase==0&&r_axis==1&&r_phase==0)
          ||(!strcmp(entry,"rt_im")&&q_axis==0&&q_phase==0&&r_axis==1&&r_phase==1);
        if(!metadata_ok)die("reference entry does not match direction metadata");
    }
    arb_t geom,dir,zero;arb_init(geom);arb_init(dir);arb_init(zero);arb_zero(zero);
    if(arb_set_str(geom,geom_s,prec)||arb_set_str(dir,dir_s,prec)||
       !arb_is_finite(geom)||!arb_is_finite(dir)||
       !arb_is_nonnegative(geom)||!arb_is_nonnegative(dir))die("bad radius");
    point_t *p=malloc((size_t)nv*sizeof(point_t));tri_t *tri=malloc((size_t)nt*sizeof(tri_t));if(!p||!tri)die("allocation");
    for(long i=0;i<nv;++i){arb_init(&p[i].x);arb_init(&p[i].y);if(!read_ball(in,&p[i].x,geom,prec)||!read_ball(in,&p[i].y,geom,prec))die("vertices");}
    for(long j=0;j<nt;++j){if(fscanf(in,"%ld %ld %ld",tri[j].v,tri[j].v+1,tri[j].v+2)!=3)die("triangles");arb_init(&tri[j].area);for(int i=0;i<3;++i){arb_init(tri[j].gx+i);arb_init(tri[j].gy+i);}}
    unsigned char *boundary=certify_mesh_topology(tri,nt,nv,n,m,check_inputs);
    if(diagnostic)fprintf(stderr,"second_lifting_cert: DIAGNOSTIC ONLY (no certified regular-fan Dirichlet submatrix)\n");
    arb_t x,tmp;arb_init(x);arb_init(tmp);
    assemble_triangle_geometry(p,tri,nt,nv,prec);
    arb_ptr qx=read_field(in,nv,dir,prec),qy=read_field(in,nv,dir,prec),rx=read_field(in,nv,dir,prec),ry=read_field(in,nv,dir,prec);
    arb_ptr u=read_field(in,nv,zero,prec),uq=read_field(in,nv,zero,prec),ur=read_field(in,nv,zero,prec),zh=read_field(in,nv,zero,prec);
    arb_ptr psi0=read_field(in,nv,zero,prec),psiq=read_field(in,nv,zero,prec),psir=read_field(in,nv,zero,prec),psiz=read_field(in,nv,zero,prec);
    if(fscanf(in,"%63s",tag)==1)die("trailing input data");
    fclose(in);
    /* Work on the exact interior-node submatrices.  FreeFEM is only a
       floating-point candidate generator; its tiny boundary roundoff values
       are discarded and the four Dirichlet fields are extended by exact zero. */
    for(long i=0;i<nv;++i)if(boundary[i]){arb_zero(u+i);arb_zero(uq+i);arb_zero(ur+i);arb_zero(zh+i);}
    if(check_inputs)validate_regular_inputs(n,m,k_mode,q_axis,q_phase,r_axis,r_phase,
            p,nv,qx,qy,rx,ry,tri,nt,boundary,prec);

    arb_t d0,sumq,sumr,sumz,opq,opr,mqr,fluxq,fluxr,eta,dq,dr,bound;
    arb_t area,intu,jh,jcenter,ha,fcenter,ferror,enclosure;
    arb_init(d0);arb_init(sumq);arb_init(sumr);arb_init(sumz);arb_init(opq);arb_init(opr);arb_init(mqr);arb_init(fluxq);arb_init(fluxr);arb_init(eta);arb_init(dq);arb_init(dr);arb_init(bound);
    arb_init(area);arb_init(intu);arb_init(jh);arb_init(jcenter);arb_init(ha);arb_init(fcenter);arb_init(ferror);arb_init(enclosure);
    arb_zero(sumq);arb_zero(sumr);arb_zero(sumz);arb_zero(opq);arb_zero(opr);arb_zero(mqr);arb_zero(area);arb_zero(intu);arb_zero(jcenter);arb_zero(ha);state_error(d0,u,psi0,p,tri,nt,prec);
    arb_ptr res0=_arb_vec_init(nv),resq=_arb_vec_init(nv),resr=_arb_vec_init(nv),resz=_arb_vec_init(nv),patch=_arb_vec_init(nv);
    arb_t Q[4],R[4],Aq[3],Ar[3],Aqr[3],dqr,ux,uy,uqx,uqy,urx,ury,zx,zy,sx,sy,tx,ty,norm,temp,v[3];
    for(int i=0;i<4;++i){arb_init(Q[i]);arb_init(R[i]);}for(int i=0;i<3;++i){arb_init(Aq[i]);arb_init(Ar[i]);arb_init(Aqr[i]);arb_init(v[i]);}
    arb_init(dqr);arb_init(ux);arb_init(uy);arb_init(uqx);arb_init(uqy);arb_init(urx);arb_init(ury);arb_init(zx);arb_init(zy);arb_init(sx);arb_init(sy);arb_init(tx);arb_init(ty);arb_init(norm);arb_init(temp);
    for(long j=0;j<nt;++j){
      gradient(Q[0],Q[1],qx,tri+j,prec);gradient(Q[2],Q[3],qy,tri+j,prec);gradient(R[0],R[1],rx,tri+j,prec);gradient(R[2],R[3],ry,tri+j,prec);coefficients(Aq,Ar,Aqr,dqr,Q,R,prec);
      arb_add(area,area,&tri[j].area,prec);affine_integral(temp,u,tri+j,prec);arb_add(intu,intu,temp,prec);arb_addmul(ha,dqr,&tri[j].area,prec);
      symmetric_norm(norm,Aq[0],Aq[1],Aq[2],prec);arb_max(opq,opq,norm,prec);symmetric_norm(norm,Ar[0],Ar[1],Ar[2],prec);arb_max(opr,opr,norm,prec);symmetric_norm(norm,Aqr[0],Aqr[1],Aqr[2],prec);arb_max(mqr,mqr,norm,prec);
      gradient(ux,uy,u,tri+j,prec);gradient(uqx,uqy,uq,tri+j,prec);gradient(urx,ury,ur,tri+j,prec);gradient(zx,zy,zh,tri+j,prec);
      for(int i=0;i<3;++i){long node=tri[j].v[i];arb_add(patch+node,patch+node,&tri[j].area,prec);arb_mul(temp,ux,tri[j].gx+i,prec);arb_addmul(temp,uy,tri[j].gy+i,prec);arb_mul(temp,temp,&tri[j].area,prec);arb_div_ui(norm,&tri[j].area,3,prec);arb_sub(temp,temp,norm,prec);arb_add(res0+node,res0+node,temp,prec);}
      affine_integral(temp,u,tri+j,prec);arb_addmul(jcenter,dqr,temp,prec);arb_add(tx,Q[0],Q[3],prec);affine_integral(temp,ur,tri+j,prec);arb_addmul(jcenter,tx,temp,prec);
      arb_mul(tx,Aqr[0],ux,prec);arb_addmul(tx,Aqr[1],uy,prec);arb_mul(ty,Aqr[1],ux,prec);arb_addmul(ty,Aqr[2],uy,prec);arb_mul(temp,tx,ux,prec);arb_addmul(temp,ty,uy,prec);arb_mul(temp,temp,&tri[j].area,prec);arb_mul_2exp_si(temp,temp,-1);arb_sub(jcenter,jcenter,temp,prec);
      arb_mul(tx,Aq[0],ux,prec);arb_addmul(tx,Aq[1],uy,prec);arb_mul(ty,Aq[1],ux,prec);arb_addmul(ty,Aq[2],uy,prec);arb_mul(temp,tx,urx,prec);arb_addmul(temp,ty,ury,prec);arb_mul(temp,temp,&tri[j].area,prec);arb_sub(jcenter,jcenter,temp,prec);
      gradient(sx,sy,psiq,tri+j,prec);arb_mul(tx,Aq[0],ux,prec);arb_addmul(tx,Aq[1],uy,prec);arb_add(tx,tx,uqx,prec);arb_mul(ty,Aq[1],ux,prec);arb_addmul(ty,Aq[2],uy,prec);arb_add(ty,ty,uqy,prec);
      arb_add(norm,Q[0],Q[3],prec);for(int i=0;i<3;++i){long node=tri[j].v[i];arb_mul(temp,tx,tri[j].gx+i,prec);arb_addmul(temp,ty,tri[j].gy+i,prec);arb_mul(temp,temp,&tri[j].area,prec);arb_mul(enclosure,norm,&tri[j].area,prec);arb_div_ui(enclosure,enclosure,3,prec);arb_sub(temp,temp,enclosure,prec);arb_add(resq+node,resq+node,temp,prec);}
      for(int i=0;i<3;++i){arb_neg(v[i],qx+tri[j].v[i]);arb_add(v[i],v[i],sy,prec);arb_sub(v[i],v[i],tx,prec);}add_affine_square(sumq,v,tri+j,prec);for(int i=0;i<3;++i){arb_neg(v[i],qy+tri[j].v[i]);arb_sub(v[i],v[i],sx,prec);arb_sub(v[i],v[i],ty,prec);}add_affine_square(sumq,v,tri+j,prec);
      gradient(sx,sy,psir,tri+j,prec);arb_mul(tx,Ar[0],ux,prec);arb_addmul(tx,Ar[1],uy,prec);arb_add(tx,tx,urx,prec);arb_mul(ty,Ar[1],ux,prec);arb_addmul(ty,Ar[2],uy,prec);arb_add(ty,ty,ury,prec);
      arb_add(norm,R[0],R[3],prec);for(int i=0;i<3;++i){long node=tri[j].v[i];arb_mul(temp,tx,tri[j].gx+i,prec);arb_addmul(temp,ty,tri[j].gy+i,prec);arb_mul(temp,temp,&tri[j].area,prec);arb_mul(enclosure,norm,&tri[j].area,prec);arb_div_ui(enclosure,enclosure,3,prec);arb_sub(temp,temp,enclosure,prec);arb_add(resr+node,resr+node,temp,prec);}
      for(int i=0;i<3;++i){arb_neg(v[i],rx+tri[j].v[i]);arb_add(v[i],v[i],sy,prec);arb_sub(v[i],v[i],tx,prec);}add_affine_square(sumr,v,tri+j,prec);for(int i=0;i<3;++i){arb_neg(v[i],ry+tri[j].v[i]);arb_sub(v[i],v[i],sx,prec);arb_sub(v[i],v[i],ty,prec);}add_affine_square(sumr,v,tri+j,prec);
      arb_set(tx,zx);arb_addmul(tx,Aq[0],urx,prec);arb_addmul(tx,Aq[1],ury,prec);arb_addmul(tx,Ar[0],uqx,prec);arb_addmul(tx,Ar[1],uqy,prec);arb_addmul(tx,Aqr[0],ux,prec);arb_addmul(tx,Aqr[1],uy,prec);
      arb_set(ty,zy);arb_addmul(ty,Aq[1],urx,prec);arb_addmul(ty,Aq[2],ury,prec);arb_addmul(ty,Ar[1],uqx,prec);arb_addmul(ty,Ar[2],uqy,prec);arb_addmul(ty,Aqr[1],ux,prec);arb_addmul(ty,Aqr[2],uy,prec);gradient(sx,sy,psiz,tri+j,prec);
      for(int i=0;i<3;++i){long node=tri[j].v[i];arb_mul(temp,tx,tri[j].gx+i,prec);arb_addmul(temp,ty,tri[j].gy+i,prec);arb_mul(temp,temp,&tri[j].area,prec);arb_mul(enclosure,dqr,&tri[j].area,prec);arb_div_ui(enclosure,enclosure,3,prec);arb_sub(temp,temp,enclosure,prec);arb_add(resz+node,resz+node,temp,prec);}
      arb_add(temp,Q[0],Q[3],prec);for(int i=0;i<3;++i){arb_mul(v[i],temp,rx+tri[j].v[i],prec);arb_neg(v[i],v[i]);arb_addmul(v[i],Q[0],rx+tri[j].v[i],prec);arb_addmul(v[i],Q[1],ry+tri[j].v[i],prec);arb_add(v[i],v[i],sy,prec);arb_sub(v[i],v[i],tx,prec);}add_affine_square(sumz,v,tri+j,prec);
      for(int i=0;i<3;++i){arb_mul(v[i],temp,ry+tri[j].v[i],prec);arb_neg(v[i],v[i]);arb_addmul(v[i],Q[2],rx+tri[j].v[i],prec);arb_addmul(v[i],Q[3],ry+tri[j].v[i],prec);arb_sub(v[i],v[i],sx,prec);arb_sub(v[i],v[i],ty,prec);}add_affine_square(sumz,v,tri+j,prec);
    }
    arb_sqrtpos(fluxq,sumq,prec);arb_sqrtpos(fluxr,sumr,prec);arb_sqrtpos(eta,sumz,prec);arb_mul(dq,opq,d0,prec);arb_add(dq,dq,fluxq,prec);arb_mul(dr,opr,d0,prec);arb_add(dr,dr,fluxr,prec);
    arb_t rn0,rnq,rnr,rnz,minpatch,alpha,sqrtalpha,ae0,aeq,aer,aez,lambda;
    arb_t eps0,epsq,epsr,d0tot,dqtot,drtot,etatot;
    arb_t center_error,algebraic_ferror,continuous_ferror,jcenter_raw,jh_raw;
    arb_init(rn0);arb_init(rnq);arb_init(rnr);arb_init(rnz);arb_init(minpatch);arb_init(alpha);arb_init(sqrtalpha);arb_init(ae0);arb_init(aeq);arb_init(aer);arb_init(aez);arb_init(lambda);
    arb_init(eps0);arb_init(epsq);arb_init(epsr);arb_init(d0tot);arb_init(dqtot);arb_init(drtot);arb_init(etatot);
    arb_init(center_error);arb_init(algebraic_ferror);arb_init(continuous_ferror);arb_init(jcenter_raw);arb_init(jh_raw);
    interior_residual_norm(rn0,res0,boundary,nv,prec);interior_residual_norm(rnq,resq,boundary,nv,prec);interior_residual_norm(rnr,resr,boundary,nv,prec);interior_residual_norm(rnz,resz,boundary,nv,prec);
    int havepatch=0;for(long i=0;i<nv;++i)if(!boundary[i]){if(!havepatch){arb_set(minpatch,patch+i);havepatch=1;}else arb_min(minpatch,minpatch,patch+i,prec);}if(!havepatch)die("no interior nodes");
    /* The circumradius-one polygon lies in (-1,1)^2.  Dirichlet domain
       monotonicity therefore gives lambda_1(P_n)>=pi^2/2, avoiding any
       tabulated Bessel zero in this proof-critical constant. */
    arb_const_pi(lambda,prec);arb_sqr(lambda,lambda,prec);arb_mul_2exp_si(lambda,lambda,-1);
    arb_div_ui(minpatch,minpatch,12,prec);arb_mul(alpha,lambda,minpatch,prec);if(!arb_is_positive(alpha))die("failed stiffness lower bound");arb_sqrtpos(sqrtalpha,alpha,prec);arb_div(ae0,rn0,sqrtalpha,prec);arb_div(aeq,rnq,sqrtalpha,prec);arb_div(aer,rnr,sqrtalpha,prec);arb_div(aez,rnz,sqrtalpha,prec);

    /* Algebraic transfer, in the spirit of PolyaHessIntervalU.  The residual
       vectors are evaluated on the exact interior-node submatrices.  ae0 is
       the state algebraic energy error.  The differentiated right sides also
       depend on the state, hence the elementary coupling terms below. */
    arb_set(eps0,ae0);arb_mul(epsq,opq,eps0,prec);arb_add(epsq,epsq,aeq,prec);arb_mul(epsr,opr,eps0,prec);arb_add(epsr,epsr,aer,prec);
    /* The exact state Galerkin error is no larger than the error to any
       conforming candidate, so d0 itself is the sharper delta_0. */
    arb_set(d0tot,d0);arb_add(dqtot,dq,epsq,prec);arb_add(drtot,dr,epsr,prec);
    arb_set(etatot,eta);arb_addmul(etatot,opq,epsr,prec);arb_addmul(etatot,opr,epsq,prec);arb_addmul(etatot,mqr,eps0,prec);
    arb_mul(bound,dqtot,drtot,prec);arb_sqr(temp,d0tot,prec);arb_mul(temp,temp,mqr,prec);arb_mul_2exp_si(temp,temp,-1);arb_add(bound,bound,temp,prec);arb_addmul(bound,d0tot,etatot,prec);

    /* Residual-corrected centers.  With rho=right side-matrix*candidate and
       res=-rho in the arrays above,
         Ccorr=Ca+rho_r(uq)+rho_0(zh),
         Jcorr=ell(u)-a(u,u)/2.
       The remaining errors are exactly quadratic in the algebraic errors. */
    arb_set(jcenter_raw,jcenter);for(long i=0;i<nv;++i)if(!boundary[i]){arb_submul(jcenter,resr+i,uq+i,prec);arb_submul(jcenter,res0+i,zh+i,prec);}
    arb_mul_2exp_si(jh,intu,-1);arb_set(jh_raw,jh);arb_zero(temp);for(long i=0;i<nv;++i)if(!boundary[i])arb_addmul(temp,res0+i,u+i,prec);arb_mul_2exp_si(temp,temp,-1);arb_sub(jh,jh,temp,prec);
    arb_mul(center_error,aez,eps0,prec);arb_sqr(temp,eps0,prec);arb_mul(temp,temp,mqr,prec);arb_mul_2exp_si(temp,temp,-1);arb_add(center_error,center_error,temp,prec);arb_addmul(center_error,epsq,epsr,prec);

    arb_sqr(temp,area,prec);arb_div(fcenter,jcenter,temp,prec);arb_mul(temp,temp,area,prec);arb_mul(enclosure,jh,ha,prec);arb_mul_2exp_si(enclosure,enclosure,1);arb_div(enclosure,enclosure,temp,prec);arb_sub(fcenter,fcenter,enclosure,prec);
    arb_sqr(temp,area,prec);arb_div(continuous_ferror,bound,temp,prec);arb_mul(temp,temp,area,prec);arb_sqr(enclosure,d0tot,prec);arb_abs(x,ha);arb_mul(enclosure,enclosure,x,prec);arb_div(enclosure,enclosure,temp,prec);arb_add(continuous_ferror,continuous_ferror,enclosure,prec);
    arb_sqr(temp,area,prec);arb_div(algebraic_ferror,center_error,temp,prec);arb_mul(temp,temp,area,prec);arb_sqr(enclosure,eps0,prec);arb_mul(enclosure,enclosure,x,prec);arb_div(enclosure,enclosure,temp,prec);arb_add(algebraic_ferror,algebraic_ferror,enclosure,prec);
    arb_add(ferror,continuous_ferror,algebraic_ferror,prec);arb_set(enclosure,fcenter);arb_add_error(enclosure,ferror);
    printf("delta0_candidate=");arb_printn(d0,18,ARB_STR_MORE);printf("\ndelta0_total=");arb_printn(d0tot,18,ARB_STR_MORE);printf("\nflux_q=");arb_printn(fluxq,18,ARB_STR_MORE);printf("\ndelta_q_candidate=");arb_printn(dq,18,ARB_STR_MORE);printf("\ndelta_q_total=");arb_printn(dqtot,18,ARB_STR_MORE);printf("\nflux_r=");arb_printn(fluxr,18,ARB_STR_MORE);printf("\ndelta_r_candidate=");arb_printn(dr,18,ARB_STR_MORE);printf("\ndelta_r_total=");arb_printn(drtot,18,ARB_STR_MORE);printf("\neta_qr_candidate=");arb_printn(eta,18,ARB_STR_MORE);printf("\neta_qr_total=");arb_printn(etatot,18,ARB_STR_MORE);printf("\nM_qr=");arb_printn(mqr,18,ARB_STR_MORE);printf("\nalgebraic_energy_state<=");arb_printn(ae0,12,ARB_STR_MORE);printf("\nalgebraic_residual_q<=");arb_printn(aeq,12,ARB_STR_MORE);printf("\nalgebraic_energy_q_total<=");arb_printn(epsq,12,ARB_STR_MORE);printf("\nalgebraic_residual_r<=");arb_printn(aer,12,ARB_STR_MORE);printf("\nalgebraic_energy_r_total<=");arb_printn(epsr,12,ARB_STR_MORE);printf("\nalgebraic_residual_z<=");arb_printn(aez,12,ARB_STR_MORE);printf("\nJ_hessian_raw_center=");arb_printn(jcenter_raw,18,ARB_STR_MORE);printf("\nJ_hessian_corrected_center=");arb_printn(jcenter,18,ARB_STR_MORE);printf("\nJ_raw_center=");arb_printn(jh_raw,18,ARB_STR_MORE);printf("\nJ_corrected_center=");arb_printn(jh,18,ARB_STR_MORE);printf("\nJ_hessian_continuous_error<=");arb_printn(bound,18,ARB_STR_MORE);printf("\nJ_hessian_algebraic_center_error<=");arb_printn(center_error,18,ARB_STR_MORE);printf("\nHA_qr=");arb_printn(ha,18,ARB_STR_MORE);printf("\nF_hessian_center=");arb_printn(fcenter,18,ARB_STR_MORE);printf("\nF_hessian_continuous_error<=");arb_printn(continuous_ferror,18,ARB_STR_MORE);printf("\nF_hessian_algebraic_error<=");arb_printn(algebraic_ferror,18,ARB_STR_MORE);printf("\nF_hessian_error<=");arb_printn(ferror,18,ARB_STR_MORE);printf("\nF_hessian_enclosure=");arb_printn(enclosure,18,ARB_STR_MORE);printf("\n");
    if(reference){
        arb_t maxrad,refcenter,refball,conjugate_enclosure;arb_init(maxrad);arb_init(refcenter);arb_init(refball);arb_init(conjugate_enclosure);
        if(arb_set_str(maxrad,max_radius_s,prec)||!arb_is_finite(maxrad)||
           !arb_is_nonnegative(maxrad))die("bad maximum entry radius");
        read_mode_reference(refcenter,reference,k_mode,entry,prec);arb_set(refball,refcenter);arb_add_error(refball,maxrad);
        if(!arb_contains(refball,enclosure))die("certified entry misses reference ball");
        read_mode_reference(refcenter,reference,(n-k_mode)%n,entry,prec);arb_set(refball,refcenter);arb_add_error(refball,maxrad);arb_set(conjugate_enclosure,enclosure);
        if(!strcmp(entry,"rt_im"))arb_neg(conjugate_enclosure,conjugate_enclosure);
        if(!arb_contains(refball,conjugate_enclosure))die("certified conjugate entry misses reference ball");
        printf("ENTRY_CERTIFIED k=%ld conjugate_k=%ld entry=%s max_radius=%s\n",k_mode,(n-k_mode)%n,entry,max_radius_s);
        arb_clear(maxrad);arb_clear(refcenter);arb_clear(refball);arb_clear(conjugate_enclosure);
    }
    /* The operating-system cleanup at exit releases the remaining Arb vectors. */
    flint_cleanup();return 0;
}
