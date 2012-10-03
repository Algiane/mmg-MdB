#include "mmg3d.h"

extern Info  info;


/* topology: set adjacent, detect Moebius, flip faces, count connected comp. */
static int setadj(pMesh mesh){
  pTria   pt,pt1;
  pPoint  ppt;
  int    *adja,*adjb,adji1,adji2,*pile,iad,ipil,ip1,ip2,gen;
  int     k,kk,iel,jel,nf,np,nr,nt,nre,ncc,ned,nvf,edg;
  char    i,ii,i1,i2,ii1,ii2,voy,tag;

  nvf = nf = ncc = ned = 0;
  pile = (int*)malloc((mesh->nt+1)*sizeof(int));
  assert(pile);
  pile[1] = 1;
  ipil    = 1;
  pt = &mesh->tria[1];
  pt->flag = 1;

  while ( ipil > 0 ) {
    ncc++;
    do {
      k  = pile[ipil--];
      pt = &mesh->tria[k];
      if ( !MG_EOK(pt) )  continue;
      adja = &mesh->adjt[3*(k-1)+1];
      for (i=0; i<3; i++) {
	i1  = inxt2[i];
	i2  = iprv2[i];
	ip1 = pt->v[i1];
	ip2 = pt->v[i2];
	if ( !mesh->point[ip1].tmp )  mesh->point[ip1].tmp = ++nvf;
	if ( !mesh->point[ip2].tmp )  mesh->point[ip2].tmp = ++nvf;
	if ( MG_EDG(pt->tag[i]) ) {
	  mesh->point[ip1].tag |= pt->tag[i];
	  mesh->point[ip2].tag |= pt->tag[i];
	}

	/* open boundary */
	if ( !adja[i] ) {
	  pt->tag[i] |= MG_GEO;
	  mesh->point[ip1].tag |= MG_GEO;
	  mesh->point[ip2].tag |= MG_GEO;
	  ned++;
	  continue;
	}
	kk = adja[i] / 3;
	ii = adja[i] % 3;
	if ( kk > k )  ned++;

	/* non manifold edge */
	if ( pt->tag[i] & MG_NOM ) {
	  mesh->point[ip1].tag |= MG_NOM;
	  mesh->point[ip2].tag |= MG_NOM;
	  continue;
	}

	/* store adjacent */
	pt1 = &mesh->tria[kk];
	if ( abs(pt1->ref) != abs(pt->ref) ) {
	  pt->tag[i]   |= MG_REF;
	  if ( !(pt->tag[i] & MG_NOM) )  pt1->tag[ii] |= MG_REF;
	  mesh->point[ip1].tag |= MG_REF;
	  mesh->point[ip2].tag |= MG_REF;
	}

	if ( pt1->flag == 0 ) {
	  pt1->flag    = ncc;
	  pile[++ipil] = kk;
	}

	/* check orientation */
	ii1 = inxt2[ii];
	ii2 = iprv2[ii];
	if ( pt1->v[ii1] == ip1 ) {
	  /* Moebius strip */
	  if ( pt1->base < 0 ) {
	    fprintf(stdout,"  ## Orientation problem (1).\n");
	    return(0);
	  }
	  /* flip orientation */
	  else {
	    pt1->base   = -pt1->base;
	    pt1->v[ii1] = ip2;
	    pt1->v[ii2] = ip1;

	    /* update adj */
	    iad   = 3*(kk-1)+1;
	    adjb  = &mesh->adjt[iad];
	    adji1 = mesh->adjt[iad+ii1];
	    adji2 = mesh->adjt[iad+ii2];
	    adjb[ii1] = adji2;
	    adjb[ii2] = adji1;
	    tag = pt1->tag[ii1];
	    pt1->tag[ii1] = pt1->tag[ii2];
	    pt1->tag[ii2] = tag;
	    edg = pt1->edg[ii1];
	    pt1->edg[ii1] = pt1->edg[ii2];
	    pt1->edg[ii2] = edg;

	    /* modif voyeurs */
	    if ( adjb[ii1] ) {
	      iel = adjb[ii1] / 3;
	      voy = adjb[ii1] % 3;
	      mesh->adjt[3*(iel-1)+1+voy] = 3*kk + ii1;
	    }
	    if ( adjb[ii2] ) {
	      iel = adjb[ii2] / 3;
	      voy = adjb[ii2] % 3;
	      mesh->adjt[3*(iel-1)+1+voy] = 3*kk + ii2;
	    }
	    nf++;
	  }
	}
      }
    }
    while ( ipil > 0 );

    /* find next unmarked triangle */
    ipil = 0;
    for (kk=1; kk<=mesh->nt; kk++) {    //old kk = k+1 ?
      pt = &mesh->tria[kk];
      if ( MG_EOK(pt) && (pt->flag == 0) ) {
	ipil = 1;
	pile[ipil] = kk;
	pt->flag   = ncc+1;
	break;
      }
    }
  }

  /* bilan */
  np = nr = nre = nt = 0;
  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) )  continue;
    nt++;
    adja = &mesh->adjt[3*(k-1)+1];
    for (i=0; i<3; i++) {
      ppt = &mesh->point[pt->v[i]];
      if ( !ppt->tmp ) {
	ppt->tmp = 1;
	np++;
      }
      if ( !MG_EDG(pt->tag[i]) )  continue;
      jel  = adja[i] / 3;
      if ( !jel || jel > k ) {
	if ( pt->tag[i] & MG_GEO )  nr++;
	if ( pt->tag[i] & MG_REF )  nre++;
      }
    }
  }
  if ( info.ddebug ) {
    fprintf(stdout,"  a- ridges: %d found.\n",nr);
    fprintf(stdout,"  a- connex: %d connected component(s)\n",ncc);
    fprintf(stdout,"  a- orient: %d flipped\n",nf);
  }
  else if ( abs(info.imprim) > 4 ) {
    gen = (2 - nvf + ned - nt) / 2;
    fprintf(stdout,"     Connected component: %d,  genus: %d,   reoriented: %d\n",ncc,gen,nf);
    fprintf(stdout,"     Edges: %d,  tagged: %d,  ridges: %d,  refs: %d\n",ned,nr+nre,nr,nre);
  }
  free(pile);
  return(1);
}

/* check for ridges: dihedral angle */
static int setdhd(pMesh mesh) {
  pTria    pt,pt1;
  double   n1[3],n2[3],dhd;
  int     *adja,k,kk,ne,nr;
  char     i,ii,i1,i2;

  ne = nr = 0;
  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) )  continue;

    /* triangle normal */
    nortri(mesh,pt,n1);
    adja = &mesh->adjt[3*(k-1)+1];
    for (i=0; i<3; i++) {
      kk  = adja[i] / 3;
      ii  = adja[i] % 3;
      if ( !kk )
	pt->tag[i] |= MG_GEO;
      else if ( k < kk ) {
	pt1 = &mesh->tria[kk];
	/* reference curve */
	if ( pt1->ref != pt->ref ) {
	  pt->tag[i]   |= MG_REF;
	  pt1->tag[ii] |= MG_REF;
	  i1 = inxt2[i];
	  i2 = inxt2[i1];
	  mesh->point[pt->v[i1]].tag |= MG_REF;
	  mesh->point[pt->v[i2]].tag |= MG_REF;
	  ne++;
	}
	/* check angle w. neighbor */
	nortri(mesh,pt1,n2);
	dhd = n1[0]*n2[0] + n1[1]*n2[1] + n1[2]*n2[2];
	if ( dhd <= info.dhd ) {
	  pt->tag[i]   |= MG_GEO;
	  pt1->tag[ii] |= MG_GEO;
	  i1 = inxt2[i];
	  i2 = inxt2[i1];
	  mesh->point[pt->v[i1]].tag |= MG_GEO;
	  mesh->point[pt->v[i2]].tag |= MG_GEO;
	  nr++;
	}
      }
    }
  }
  if ( abs(info.imprim) > 4 && nr > 0 )
    fprintf(stdout,"     %d ridges, %d edges updated\n",nr,ne);

  return(1);
}

/* check for singularities */
static int singul(pMesh mesh) {
  pTria     pt;
  pPoint    ppt,p1,p2;
  double    ux,uy,uz,vx,vy,vz,dd;
  int       list[LMAX+2],k,nc,nr,nre;
  char      i;

  nre = nc = 0;

  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];

    for (i=0; i<3; i++) {
      ppt = &mesh->point[pt->v[i]];
      if ( MG_SIN(ppt->tag) || ppt->tag & MG_NOM || !MG_EDG(ppt->tag) )  continue;

      nr = bouler(mesh,k,i,list);
      if ( nr != 2 ) {
	ppt->tag |= MG_REQ;
	nre++;
      }
      /* check ridge angle */
      else {
	p1 = &mesh->point[list[1]];
	p2 = &mesh->point[list[2]];
	ux = p1->c[0] - ppt->c[0];
	uy = p1->c[1] - ppt->c[1];
	uz = p1->c[2] - ppt->c[2];
	vx = p2->c[0] - ppt->c[0];
	vy = p2->c[1] - ppt->c[1];
	vz = p2->c[2] - ppt->c[2];
	dd = (ux*ux + uy*uy + uz*uz) * (vx*vx + vy*vy + vz*vz);
	if ( fabs(dd) > EPSD ) {
	  dd = (ux*vx + uy*vy + uz*vz) / sqrt(dd);
	  if ( dd > -info.dhd ) {
	    ppt->tag |= MG_CRN;
	    nc++;
	  }
	}
      }
    }
  }

  if ( abs(info.imprim) > 4 && nre > 0 )
    fprintf(stdout,"     %d corners, %d singular points detected\n",nc,nre);
  return(1);
}

/* compute normals at C1 vertices, for C0: tangents */
static int norver(pMesh mesh) {
  pTria     pt;
  pPoint    ppt;
  xPoint   *pxp;
  double    n[3],dd;
  int      *adja,k,kk,ng,nn,nt,nf;
  char      i,ii,i1;

  /* identify boundary points */
  ++mesh->base;
  mesh->xp = 0;
  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) )  continue;

    for (i=0; i<3; i++) {
      ppt = &mesh->point[pt->v[i]];
      if ( ppt->flag == mesh->base )  continue;
      else {
	++mesh->xp;
	ppt->flag = mesh->base;
      }
    }
  }

  /* memory to store normals for boundary points */
  mesh->xpmax  = MG_MAX(1.5*mesh->xp,NPMAX);
  mesh->xpoint = (pxPoint)calloc(mesh->xpmax+1,sizeof(xPoint));
  assert(mesh->xpoint);

  /* compute normals + tangents */
  nn = ng = nt = nf = 0;
  mesh->xp = 0;
  ++mesh->base;
  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) )  continue;

    adja = &mesh->adjt[3*(k-1)+1];
    for (i=0; i<3; i++) {
      ppt = &mesh->point[pt->v[i]];
      if ( ppt->tag & MG_CRN || ppt->tag & MG_NOM || ppt->flag == mesh->base )  continue;

      /* C1 point */
      if ( !MG_EDG(ppt->tag) ) {
	if ( !boulen(mesh,k,i,n) ) {
	  ++nf;
	  continue;
	}
	else {
	  ++mesh->xp;
	  assert(mesh->xp < mesh->xpmax);
	  ppt->xp = mesh->xp;
	  pxp = &mesh->xpoint[ppt->xp];
	  memcpy(pxp->n1,n,3*sizeof(double));
	  ppt->flag = mesh->base;
	  nn++;
	}
      }

      /* along ridge-curve */
      i1  = inxt2[i];
      if ( !MG_EDG(pt->tag[i1]) )  continue;
      else if ( !boulen(mesh,k,i,n) ) {
	++nf;
	continue;
      }
      ++mesh->xp;
      assert(mesh->xp < mesh->xpmax);
      ppt->xp = mesh->xp;
      pxp = &mesh->xpoint[ppt->xp];
      memcpy(pxp->n1,n,3*sizeof(double));

      if ( pt->tag[i1] & MG_GEO && adja[i1] > 0 ) {
	kk = adja[i1] / 3;
	ii = adja[i1] % 3;
	ii = inxt2[ii];
	if ( !boulen(mesh,kk,ii,n) ) {
	  ++nf;
	  continue;
	}
	memcpy(pxp->n2,n,3*sizeof(double));

	/* compute tangent as intersection of n1 + n2 */
	pxp->t[0] = pxp->n1[1]*pxp->n2[2] - pxp->n1[2]*pxp->n2[1];
	pxp->t[1] = pxp->n1[2]*pxp->n2[0] - pxp->n1[0]*pxp->n2[2];
	pxp->t[2] = pxp->n1[0]*pxp->n2[1] - pxp->n1[1]*pxp->n2[0];
	dd = pxp->t[0]*pxp->t[0] + pxp->t[1]*pxp->t[1] + pxp->t[2]*pxp->t[2];
	if ( dd > EPSD2 ) {
	  dd = 1.0 / sqrt(dd);
	  pxp->t[0] *= dd;
	  pxp->t[1] *= dd;
	  pxp->t[2] *= dd;
	}
	ppt->flag = mesh->base;
	++nt;
	continue;
      }

      /* compute tgte */
      ppt->flag = mesh->base;
      ++nt;
      if ( !boulec(mesh,k,i,pxp->t) ) {
	++nf;
	continue;
      }
      dd = pxp->n1[0]*pxp->t[0] + pxp->n1[1]*pxp->t[1] + pxp->n1[2]*pxp->t[2];
      pxp->t[0] -= dd*pxp->n1[0];
      pxp->t[1] -= dd*pxp->n1[1];
      pxp->t[2] -= dd*pxp->n1[2];
      dd = pxp->t[0]*pxp->t[0] + pxp->t[1]*pxp->t[1] + pxp->t[2]*pxp->t[2];
      if ( dd > EPSD2 ) {
	dd = 1.0 / sqrt(dd);
	pxp->t[0] *= dd;
	pxp->t[1] *= dd;
	pxp->t[2] *= dd;
      }
    }
  }
  if ( abs(info.imprim) > 4 && nn+nt > 0 )
    fprintf(stdout,"     %d normals,  %d tangents updated  (%d failed)\n",nn,nt,nf);

  return(1);
}

/* Define continuous geometric support at non manifold vertices, using volume information */
static int nmgeom(pMesh mesh){
  pTetra     pt;
  pPoint     p0;
  pxPoint    pxp;
  int        k,base;
  int        *adja;
  double     n[3],t[3];
  char       i,j,ip,ier;

  base = ++mesh->base;
  for (k=1; k<=mesh->ne; k++) {
    pt   = &mesh->tetra[k];
    adja = &mesh->adja[4*(k-1)+1];
    for (i=0; i<4; i++) {
      if ( adja[i] ) continue;
      for (j=0; j<3; j++) {
	ip = idir[i][j];
	p0 = &mesh->point[pt->v[ip]];
	if ( p0->flag == base )  continue;
	else if ( !(p0->tag & MG_NOM) )  continue;

	p0->flag = base;
	ier = boulenm(mesh,k,ip,i,n,t);

	if ( !ier ) {
	  p0->tag |= MG_REQ;
	  if ( p0->ref != 0 )
	    p0->ref = -abs(p0->ref);
	  else
	    p0->ref = MG_ISO;
	}
	else {
	  if ( !p0->xp ) {
	    ++mesh->xp;
	    assert(mesh->xp < mesh->xpmax);
	    p0->xp = mesh->xp;
	  }
	  pxp = &mesh->xpoint[p0->xp];
	  memcpy(pxp->n1,n,3*sizeof(double));
	  memcpy(pxp->t,t,3*sizeof(double));
	}
      }
    }
  }
  return(1);
}

/* preprocessing stage: mesh analysis */
int analys(pMesh mesh) {

  /*--- stage 1: data structures for surface */
  if ( abs(info.imprim) > 4 )
    fprintf(stdout,"  ** SURFACE ANALYSIS\n");

  /* create tetra adjacency */
  if ( !hashTetra(mesh) ) {
    fprintf(stdout,"  ## Hashing problem (1). Exit program.\n");
    return(0);
  }

  /* identify surface mesh */
  if ( !mesh->nt ) {
    if ( !bdryTria(mesh) ) {
      fprintf(stdout,"  ## Boundary problem. Exit program.\n");
      return(0);
    }
  }
  /* compatibility triangle orientation w/r tetras */
  else if ( info.iso ) {
    if ( !bdryIso(mesh) ) {
      fprintf(stdout,"  ## Failed to extract boundary. Exit program.\n");
      return(0);
    }
  }
  else if ( !bdryPerm(mesh) ) {
    fprintf(stdout,"  ## Boundary orientation problem. Exit program.\n");
    return(0);
  }

  /* create surface adjacency */
  if ( !hashTria(mesh) ) {
    fprintf(stdout,"  ## Hashing problem (2). Exit program.\n");
    return(0);
  }

  /* build hash table for geometric edges */
  if ( !hGeom(mesh) ) {
    fprintf(stdout,"  ## Hashing problem (0). Exit program.\n");
    return(0);
  }

  /*--- stage 2: surface analysis */
  if ( abs(info.imprim) > 5  || info.ddebug )
    fprintf(stdout,"  ** SETTING TOPOLOGY\n");

  /* identify connexity */
  if ( !setadj(mesh) ) {
    fprintf(stdout,"  ## Topology problem. Exit program.\n");
    return(0);
  }

  /* check for ridges */
  if ( info.dhd > ANGLIM && !setdhd(mesh) ) {
    fprintf(stdout,"  ## Geometry problem. Exit program.\n");
    return(0);
  }

  /* identify singularities */
  if ( !singul(mesh) ) {
    fprintf(stdout,"  ## Singularity problem. Exit program.\n");
    return(0);
  }

  if ( abs(info.imprim) > 4 || info.ddebug )
    fprintf(stdout,"  ** DEFINING GEOMETRY\n");

  /* define (and regularize) normals */
  if ( !norver(mesh) ) {
    fprintf(stdout,"  ## Normal problem. Exit program.\n");
    return(0);
  }

  /* set bdry entities to tetra */
  if ( !bdrySet(mesh) ) {
    fprintf(stdout,"  ## Boundary problem. Exit program.\n");
    return(0);
  }

  /* build hash table for geometric edges */
  if ( !mesh->na && !hGeom(mesh) ) {
    fprintf(stdout,"  ## Hashing problem (0). Exit program.\n");
    return(0);
  }

  /* define geometry for non manifold points */
  if ( !nmgeom(mesh) ) {
    fprintf(stdout,"  ## Problem in defining geometry for non manifold points. Exit program.\n");
    return(0);
  }

  /* release memory */
  free(mesh->tria);
  free(mesh->adjt);
  mesh->adjt = 0;
  mesh->tria = 0;
  mesh->nt   = 0;

  return(1);
}
