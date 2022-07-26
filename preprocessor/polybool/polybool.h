//	polybool.h - main PolyBoolean header
//
//	This file is a part of PolyBoolean software library
//	(C) 1998-1999 Michael Leonov
//	Consult your license regarding permissions and restrictions
//

// ==========================================================================================
//  Polyboolean modificado para operacoes booleanas de uniao e subtracao de no fit polygons
// ==========================================================================================
//
// Lista de alteracoes em polybool.h:
//
// - PAREA: Alteracao na estrutura
// Adicionado PLINE2 NFP_lines para armazenar lista de arestas internas
// Adicionado PLINE2 NFP_nodes para armazenar lista de arestas internas
// - VNODE: Alteracao na estrutura
// Adicionado flag NFPshared para determinar se o no NFP e compartilhado
//
// Lista de Funcoes adicionadas:
//
// - CopyNFP - Realiza a copia para as estruturas de arestas e vertices NFP
// - CreateNPFLine - Cria uma aresta NFP a partir de dois pontos no grid
// - CreateNPFNode - Cria um vertice NFP a partir de dois pontos no grid
// - InclPNFPline - Inclui uma aresta NFP em um poligono
// - InclPNFPnode - Inclui um vertice NFP em um poligono

#ifndef _POLYBOOL_H_
#define _POLYBOOL_H_

#include "pbdefs.h"
#include "pbgeom.h"
#include <stdlib.h>
//#include <loki/SmallObj.h>

//#define LOKI_ALLOCATOR_PARAMETERS Loki::SingleThreaded, 4096, 128, 4, Loki::NoDestroy

namespace POLYBOOLEAN
{

struct VLINK;
struct PAREA;

struct VNODE2// : public Loki::SmallValueObject<LOKI_ALLOCATOR_PARAMETERS>
{
	VNODE2 *next, *prev;
    UINT	Flags;
	bool NFPshared;
	// TEST: Marcar no como intersecao
	bool intersection;

// Flags reserved for internal use
	enum {
		RESERVED = 0x00FFu
	};

	union
	{
		VECT2   p;
		GRID2	g;		// vertex coordinates
	};

    union	// temporary fields
    {
        struct
        {
            VLINK *i, *o;
        } lnk;

        int     i;
        void  * v;
        VNODE2* vn;
    };

	static VNODE2 * New(const GRID2 & g);

	VNODE2() {
		this->next = 0;
		this->prev = 0;
		this->Flags = 0;
		this->i = 0;
		this->v = 0;
		this->vn = 0;
	}


	void Incl(VNODE2 * after) {
		(((next = after->next)->prev = this)->prev = after)->next = this;
	}

	void Excl() {
		if(next == NULL)
			prev->next = NULL;
		if(prev == NULL)
			next->prev = NULL;
		if(prev != NULL && next != NULL)
			(prev->next = next)->prev = prev;
	}

}; // struct VNODE2

struct PLINE2
{
    PLINE2*	next;	// next contour
    VNODE2*	head;	// first vertex
    UINT  Count;	// number of vertices
	UINT  Flags;
	union
	{
		VECT2	vMin;
		GRID2	gMin;
	};
	union
	{
		VECT2	vMax;
		GRID2	gMax;
	};

	enum {
		RESERVED = 0x00FF,
		ORIENT   = 0x0100,
		DIR      = 0x0100,
		INV      = 0x0000
	};

	bool IsOuter() const
	{ return (Flags & ORIENT) == DIR; }

	static
	void    Del(PLINE2 ** c);
	static
	void	DelNFP_lines(PLINE2 ** c);
	static
	void	DelNFP_nodes(PLINE2 ** c);

	static
	PLINE2 * New(const GRID2 & g);

	PLINE2 *	Copy(bool bMakeLinks = false) const;
	PLINE2 *	CopyNFP(bool bMakeLinks = false) const;

	static
	void	CreateNPFLine(PLINE2 ** pline,  const GRID2 & g1,  const GRID2 & g2);
	static
	void	CreateNPFNode(PLINE2 ** pline,  const GRID2 & g);
	static
	void	Incl(PLINE2 ** pline, const GRID2 & g);

	//	calculate orientation and bounding box
	//	removes points lying on the same line
	//	returns if contour is valid, i.e. Count >= 3 and Area != 0
	bool    Prepare();

	//	invert contour
	void	Invert();

	// put pline either into area or holes depending on its orientation
	static
	void Put(PLINE2 * pline, PAREA ** area, PLINE2 ** holes);
	
	// added by Thiago C. Martins
	double calcArea();
};

/************************* PAREA ***************************/

struct PTRIA2
{
    VNODE2   *v0, *v1, *v2;
};

struct PAREA
{
    PAREA *	f, * b;
    PLINE2 *	cntr;
	PLINE2 * NFP_lines;
	PLINE2 * NFP_nodes;
    PTRIA2 *	tria;
    UINT	tnum;

	static PAREA * New();
	static void    Del(PAREA ** p);
	PAREA * Copy() const;

	void Excl() { (b->f = f)->b = b; }

	enum PBOPCODE
	{
		UN,	// union
		IS,	// intersection
		SB,	// difference
		XR	// symmetrical difference
	};

	static int	Boolean(const PAREA * a, const PAREA * b, PAREA ** r, PBOPCODE nOpCode);

// PolyBoolean0 operates destructively on a and b
	static int	Boolean0(PAREA * a, PAREA * b, PAREA ** r, PBOPCODE nOpCode);

	static int	Triangulate(PAREA * area);

	static void	InclParea(PAREA ** list, PAREA *a);
	static void InclPline(PAREA ** area, PLINE2 * pline);
	static void InclPNFPline(PAREA ** area, PLINE2 * pline);
	static void InclPNFPnode(PAREA ** area, PLINE2 * pline);
	static void InsertHoles(PAREA ** area, PLINE2 ** holes);
#ifndef NDEBUG
	bool CheckDomain();	// check if coordinates are within 20 bit grid
#endif // NDEBUG
}; // struct PAREA

bool    GridInPline(const GRID2 * g, const PLINE2 * outer);
bool    GridInParea(const GRID2 * g, const PAREA * outer);
bool    PlineInPline(const PLINE2* p, const PLINE2 * outer);
bool    PlineInParea(const PLINE2* p, const PAREA * outer);

} // namespace POLYBOOLEAN

#endif // _POLYBOOL_H_

