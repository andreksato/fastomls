//	polybool.cpp - polygon Boolean operations
//
//	This file is a part of PolyBoolean software library
//	(C) 1998-1999 Michael Leonov
//	Consult your license regarding permissions and restrictions
//


// ==========================================================================================
//  Polyboolean modificado para operacoes booleanas de uniao e subtracao de no fit polygons
// ==========================================================================================
//
// Lista de alteracoes em polybool.cpp:
//
// - VLINK: adicionado Flag L_FNFPLINE que identifica que pertence a uma aresta interna assim
//           como as funcoes SetNFPLine e IsNFPLine
// - VNODE2: adicionado mascara E_INNODE para identificar se o vertice ja foi coletado como
//            no interno assim como as funcoes SetInNode e IsInNode. A funcao SetAllInNode 
//            marca todos os vertices que pertencem a mesma connective list.
// - ELABEL: Criado o label E_ONEDGE para ser aplicado em vertices NFP que estao sobre ares-
//            tas do outro poligono e E_ONNODE para ser aplicado em vertices NFP que estao 
//			  sobre nos NFP do outro poligono.
//
//
// Lista de Funcoes modificadas:
//
// - InitArea - Reseta as flags das arestas e vertices NFP
// - VertCnt - Foi adicionada a contagem dos vertices das arestas internas
// - Area2Segms - Tambem sao criadas estruturas SEGM2 para as arestas internas
// - DoBoolean - Antes de realizar o sort marca os descritores de arestas e vertices NFP
// - Boolean0 - Realiza a contagem antes depois do sweep e antes da operacao booelana
// - SortDesc - 1: Verifica se para um cross-vertice, há uma aresta interna e apenas arestas do
//                  mesmo poligono (aresta interna se encontra com o contorno do proprio 
//                  poligono. Remove.
//              2: Monta os descritores para as arestas internas e marca se e ISECTED.
//				3: Correcao para o caso de haver uma aresta auto-compartilhada entre duas
//				   areas de um mesmo poligono
// - LabelParea - 1: Chama funcao para aplicar labels para arestas internas
//                2: Chama funcao para aplicar labels para nos internos
// - DoLabel - Modificacao para pular a checagem de descritores de arestas internas na busca
//              dos descritores vizinhos
// - CollectParea - 1: Chama funcao para realizar collect de arestas internas
//                  2: Chama funcao para realizar collect de nos internos
// - CollectPline - Detecta criacao de arestas e vertices NFP geradas por contornos
//                   externos
// - Jump - Nao considerar nos de arestas NFP para realizar o Jump nos cross vertices
//
// Lista de Funcoes adicionadas:
//
// - MarkNFPLinks - Para todas as arestas internas de PAREA: marca os descritores como de 
//    arestas internas e desliga os links in do primeiro vertice e o link out do ultimo vertice
// - LabelNFPLine - Aplica label a uma aresta interna semelhante ao LabelPLine. Mudanca na
//    mada da funcao LabelNFPIsected
// - LabelNFPIsected - Aplica label a uma aresta interna isected semelhante a LabelIsected. 
//    Mudanca no loop, na chamada de DoNFPLabel e condicao para primeiro no da aresta interna.
// - DoNFPLabel - Aplica label para um no correspondente a um trecho da aresta interna. Muitas
//    modificacoes em relacao a DoNFPLabel. Verificar comentarios no codigo.
// - DoNFPShared - Aplica label para aresta interna compartilhada com outra aresta interna ou
//    contorno. No caso para compartilhamento de duas arestas internas, aplicamos label SHARED1
//    e para o outro caso SHARED2. Note que so se deve aplicar label SHARED para o outro no se
//    for SHARED1 e que nao importa mais a orientacao das arestas no compartilhamento.
// - PointOnLine - copia da funcao PointOnLine de pbpolys.cpp
// - LabelNFPNode - Aplica label para um no interno
// - CollectNFPPline - Realiza a coleta de arestas internas e a geracao de nos internos pelo
//    cruzamento de arestas internas
// - CollectNFPNode - Realiza a coleta de nos internos
// - DetectInsideCrossing - Detecta geracao de arestas NFP
// - GenerateNFP - Detecta geracao de arestas e nos NFP

#include "pbarea.h"
#include "ObjHeap.h"
#include "Sort.h"

//DEBUG: Include files for debugging parts
//#include <fstream>
#include "pbio.h"

using std::min;
using std::max;

namespace POLYBOOLEAN
{

// ------------------------------------------------------------------------------------------
// --> Retorna o quadrante do ponto (dx, dy)
local int GetQuadrant(pbINT dx, pbINT dy)
{
	assert(dx != 0 or dy != 0);
	if (dx >  0 and dy >= 0)	return 0;
	if (dx <= 0 and dy >  0)	return 1;
	if (dx <  0 and dy <= 0)	return 2;
assert (dx >= 0 and dy <  0);	return 3;
} // GetQuadrant

// ------------------------------------------------------------------------------------------

// --> Estrutura da Connective List L(x) (Lista duplamente ligada de Descriptors)
struct VLINK
{
    // --> Ponteiros para os vizinhos n (proximo) e p (anterior)
	VLINK *	n, * p;

	// --> No associado ao descriptor
    VNODE2*	vn;

	// --> Distancias em x e y para determinacao do angulo
	pbINT	dx;
	pbINT	dy;
	
	// --> Flags
	UINT	m_flags;

	// --> No shared que, junto com o no vn, constitui uma aresta compartilhada
    VNODE2*	shared;	// shared edge

	// --> Flags
	enum LFLAGS
	{
		// --> Determina se esta entrando no no (D+(vn))
		L_FIN		= 0x00000001,
		// --> Determina se foi validado 
		L_FVALID	= 0x00000002,
		// --> Determina se pertence ao primeiro poligono da operacao booleana
		L_FFIRST	= 0x00000004,
		// --> Determina se o uma aresta interna
		L_FNFPLINE	= 0x00000008
	};

	// --> Funcoes para operar com as flags
	void SetNFPLine(bool bNFPLine)
	{
		if (bNFPLine)
			m_flags |= L_FNFPLINE;
		else
			m_flags &= ~L_FNFPLINE;
	} // SetIn

	bool IsNFPLine() const
	{
		return (m_flags & L_FNFPLINE);
	} // IsIn
	
	void SetIn(bool bIn)
	{
		if (bIn)
			m_flags |= L_FIN;
		else
			m_flags &= ~L_FIN;
	} // SetIn

	bool IsIn() const
	{
		return (m_flags & L_FIN);
	} // IsIn

	void SetValid(bool bValid)
	{
		if (bValid)
			m_flags |= L_FVALID;
		else
			m_flags &= ~L_FVALID;
	} // SetValid

	bool IsValid() const
	{
		return (m_flags & L_FVALID);
	} // IsValid

	void SetFirst(bool bFirst)
	{
		if (bFirst)
			m_flags |= L_FFIRST;
		else
			m_flags &= ~L_FFIRST;
	} // SetFirst

	bool IsFirst() const
	{
		return (m_flags & L_FFIRST);
	} // IsFirst

	VLINK *& nxt() { return n; }

	// --> Funcao de comparacao para realizar o sort
	static int Compare(const VLINK & a, const VLINK & b)
	{
		// --> Obtem os quadrantes
		int aq = GetQuadrant(a.dx, a.dy);
		int bq = GetQuadrant(b.dx, b.dy);

		// --> Se os quadrantes forem difeirentes, a e maior se 
		// --> o quadrante de a for maior que o de b
		if (aq != bq)
			return aq - bq;

		// --> Calcula o valor de sen(a - b) e armazena em nSign
		pbDINT nSign =	(pbDINT)a.dy * b.dx -
						(pbDINT)a.dx * b.dy;

		// --> se nSign for menor do que 0 : sen(a - b) < 0 => a < b
		if (nSign < 0)	return -1;

		// --> se nSign for maior do que 0 : sen(a - b) > 0 => a > b
		if (nSign > 0)	return +1;

		// --> caso a == b retorna o que pertence ao primeiro poligono
		return a.IsFirst() - b.IsFirst();
	} // Compare
}; // struct VLINK

// ------------------------------------------------------------------------------------------
typedef ObjStorageClass<VLINK, 16, err_no_memory>	LNK_HEAP;

// ------------------------------------------------------------------------------------------
struct POLYBOOL
{
    LNK_HEAP	m_LinkHeap;
}; // class POLYBOOL

// ------------------------------------------------------------------------------------------

// DEBUG: Print Connective Lists
//local void printLinks(PAREA *area, const char* fname) {
//	std::ofstream links(fname);
//	PAREA *pa = area;
//	do {
//		links << "New Area" << std::endl;
//
//		for (PLINE2 * pline = pa->cntr; pline != NULL; pline = pline->next) {
//			links << "New Contour" << std::endl;
//			VNODE2 * vn = pline->head;
//			do {
//				links << "Node " << "( " << vn->g.x << ", " << vn->g.y << ")" << std::endl;
//				links << "Link in: ";
//				if(vn->lnk.i == NULL) links << "not found." << std::endl;
//				else links << "dx = "   << (vn->lnk.i)->dx << ". dy = " << (vn->lnk.i)->dy 
//					<< std::endl;
//				links << "Link out: ";
//				if(vn->lnk.o == NULL) links << "not found." << std::endl;
//				else links << "dx = "   << (vn->lnk.o)->dx << ". dy = " << (vn->lnk.o)->dy 
//					<< std::endl;
//
//			} while ((vn = vn->next) != pline->head);
//		}
//
//		links << "NFP Lines" << std::endl;
//		for (PLINE2 * pline = pa->NFP_lines; pline != NULL; pline = pline->next) {	
//			links << "New NFP Line" << std::endl;
//			for(VNODE2 * vn = pline->head; vn->next != NULL; vn = vn->next) {
//				links << "Node " << "( " << vn->g.x << ", " << vn->g.y << ")" << std::endl;
//				links << "Link in: ";
//				if(vn->lnk.i == NULL) links << "not found." << std::endl;
//				else links << "dx = "   << (vn->lnk.i)->dx << ". dy = " << (vn->lnk.i)->dy 
//					<< std::endl;
//				links << "Link out: ";
//				if(vn->lnk.o == NULL) links << "not found." << std::endl;
//				else links << "dx = "   << (vn->lnk.o)->dx << ". dy = " << (vn->lnk.o)->dy 
//					<< std::endl;
//			}
//		}
//
//		links << "NFP Nodes" << std::endl;
//		for (PLINE2 * pline = pa->NFP_nodes; pline != NULL; pline = pline->next) {
//			links << "New NFP Node" << std::endl;
//			VNODE2 * vn = pline->head;
//			links << "Node " << "( " << vn->g.x << ", " << vn->g.y << ")" << std::endl;
//			links << "Link in: ";
//			if(vn->lnk.i == NULL) links << "not found." << std::endl;
//			else links << "dx = "   << (vn->lnk.i)->dx << ". dy = " << (vn->lnk.i)->dy 
//				<< std::endl;
//			links << "Link out: ";
//			if(vn->lnk.o == NULL) links << "not found." << std::endl;
//			else links << "dx = "   << (vn->lnk.o)->dx << ". dy = " << (vn->lnk.o)->dy 
//				<< std::endl;
//		}
//	
//	} while ((pa = pa->f) != area);
//}

// ------------------------------------------------------------------------------------------
// --> Labels de classificacao de arestas
enum ELABEL
{
    E_INSIDE,
    E_OUTSIDE,
    E_SHARED1,
    E_SHARED2,
    E_SHARED3,
	E_ONEDGE,
	E_ONNODE,
    E_UNKNOWN
};
// --> Mascaras para flags de VNODE2
const UINT E_MASK = 0x0007; // edge label mask
const UINT E_MARK = 0x0008; // vertex is already included into result
const UINT E_FST  = 0x0010; // vertex belongs to 1st PAREA
const UINT E_INNODE = 0x0020; // vertex was already inserted as an inside node

local bool IsInNode(const VNODE2 * vn)
{
	return (vn->Flags & E_INNODE);
}

local void SetInNode(VNODE2 * vn)
{
	vn->Flags |= E_INNODE;
}

// ------------------------------------------------------------------------------------------
// --> Labels de classificacao de contornos
enum PLABEL
{
    P_OUTSIDE,
    P_INSIDE,
    P_UNKNOWN,
    P_ISECTED
};
// --> Mascaras para flags de PLINE2
const UINT P_MASK = 0x0003; // pline label mask

// ------------------------------------------------------------------------------------------
// --> Aplica label de aresta em um no
local void SetVnodeLabel(VNODE2 * vn, ELABEL nLabel)
{
	vn->Flags = (vn->Flags & ~E_MASK) | (nLabel & E_MASK);
} // SetVnodeLabel

// ------------------------------------------------------------------------------------------
// --> Obtem label de aresta de um no
local ELABEL GetVnodeLabel(const VNODE2 * vn)
{
	return (ELABEL)(vn->Flags & E_MASK);
} // GetVnodeLabel

// ------------------------------------------------------------------------------------------
// --> Aplica label de aresta a um no
local 
void SetPlineLabel(PLINE2 * pline, PLABEL nLabel)
{
	pline->Flags = (pline->Flags & ~P_MASK) | (nLabel & P_MASK);
} // SetPlineLabel

// ------------------------------------------------------------------------------------------
// --> Obtem label de um contorno
local PLABEL GetPlineLabel(const PLINE2 * pline)
{
	return (PLABEL)(pline->Flags & P_MASK);
} // GetPlineLabel

// ------------------------------------------------------------------------------------------
// --> Aplica label a um contorno
local bool IsFirst(const VNODE2 * vn)
{
	return (vn->Flags & E_FST);
} // IsFirst

// ------------------------------------------------------------------------------------------
// --> Retorna se o no ja foi marcado (ja esta incluido no resultado)
local bool IsMarked(const VNODE2 * vn)
{
	return (vn->Flags & E_MARK);
} // IsMarked

// ------------------------------------------------------------------------------------------
// --> Realiza a marcacao do no (inclui no resultado)
local void SetMarked(VNODE2 * vn)
{
	vn->Flags |= E_MARK;
} // SetMarked

// ------------------------------------------------------------------------------------------
// --> Marca o no como sendo do primeiro ou segundo poligono da operacao booleana
local void SetFirst(VNODE2 * vn, bool bFirst)
{
	if (bFirst)
		vn->Flags |= E_FST;
	else
		vn->Flags &= ~E_FST;
} // SetFirst

#define SETBITS(n,mask,s) (((n)->Flags & ~(mask)) | ((s) & (mask)))

//////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------
// --> Verifica se o descriptor se refere a uma aresta auto compartilhada
local bool HasShared3(const VLINK * a)
{
	if (a == NULL or a->n == NULL)
		return false;
	VLINK * b = a->n;

	return (a->dx == b->dx and a->dy == b->dy and a->IsFirst() == b->IsFirst());
} // HasShared3

// ------------------------------------------------------------------------------------------
// --> Realiza o desligamento do descriptor em relacao ao no correspondente
local void LnkUntie(const VLINK * l)
{
	if (l->IsIn())
		l->vn->lnk.i = NULL;
	else
		l->vn->lnk.o = NULL;
} // LnkUntie

// ------------------------------------------------------------------------------------------
// --> Aplica o label de aresta auto compartilhada e realiza o desligamento do descriptor
local void MarkAsDone(const VLINK * a)
{
	if (a->IsIn())
	{
		SetVnodeLabel(a->vn->prev, E_SHARED3);
		a->vn->lnk.i = NULL;
	}
	else
	{
		SetVnodeLabel(a->vn, E_SHARED3);
		a->vn->lnk.o = NULL;
	}
} // MarkAsDone

// ------------------------------------------------------------------------------------------
// --> Corrige os links das arestas internas
static 
void MarkNFPLinks(PAREA * area) {
	PAREA * pa = area;
	do {
		// --> Modificacao para arestas internas
		for (PLINE2 * pline = pa->NFP_lines; pline != NULL; pline = pline->next) {			
			for (VNODE2 * vn = pline->head; vn != NULL; vn = vn->next) {
				VLINK * inlink, * outlink;			
				inlink = vn->lnk.i; outlink = vn->lnk.o;
				
				// --> Remove o link in do ponto inicial
				if(vn == pline->head) {
					LnkUntie(inlink);
					inlink->p->n = inlink->n;
					inlink->n->p = inlink->p;
					inlink->n = NULL; inlink->p = NULL;
					outlink->SetNFPLine(true);
					continue;
				}
				// --> Remove o link out do ponto final
				if(vn->next == NULL) {
					LnkUntie(outlink);
					outlink->p->n = outlink->n;
					outlink->n->p = outlink->p;
					outlink->n = NULL; outlink->p = NULL;
					inlink->SetNFPLine(true); 
					continue;
				}

				// --> Marca os descritores como sendo de arestas internas
				inlink->SetNFPLine(true);
				outlink->SetNFPLine(true);
			}
		}
	} while ((pa = pa->f) != area);
}

// ------------------------------------------------------------------------------------------
// --> Valida as interseccoes e realiza o sort por ordem decrescente de angulo dos descriptors
static 
void SortDesc(PAREA * area)
{
	PAREA * pa = area;
	do {
		for (PLINE2 * pline = pa->cntr; pline != NULL; pline = pline->next)
		{
			VNODE2 * vn = pline->head;

			// --> Para cada no do contorno
			do {
				// --> Se a Connective List esta vazia (nao ha interseccoes neste ponto)
				// --> , segue para o proximo no
				if (vn->lnk.i == NULL and vn->lnk.o == NULL)
					continue;

				VLINK * head;
				if		(vn->lnk.i == NULL)
					head = vn->lnk.o;
				else if (vn->lnk.o == NULL)
					head = vn->lnk.i;
				// --> Se a interseccao ocorrer apenas no ponto final de duas arestas do  
				// --> mesmo poligono,interseccao e trivial e nao constitui um cross-vertice.
				// --> Neste caso deve-se limpar a connective list
				else if (vn->lnk.o->n == vn->lnk.i and vn->lnk.o->p == vn->lnk.i)
				{	
					vn->lnk.o = vn->lnk.i = NULL;
					continue;
				}
				// -->  Caso contrario, atribuir a header da connective list ao descriptor
				// -->  in do no
				else
					head = vn->lnk.i;

				// --> Se a header ja foi validada, atribuir label isected para o contorno
				// -->  e prosseguir para o proximo no
				if (head->IsValid())
				{
					SetPlineLabel(pline, P_ISECTED);
					continue;
				}

				// -->  Montagem dos descriptors (determinacao de dx e dy e flag L_FFIRST)
				VLINK * l = head;
				do {
					VNODE2 * vnext = (l->IsIn()) ? l->vn->prev : l->vn->next;
					l->dx = vnext->g.x - vn->g.x;
					l->dy = vnext->g.y - vn->g.y;
					l->SetFirst(IsFirst(l->vn));
				} while ((l = l->n) != head);

				// -->  Desfaz a lista circular (desliga o ultimo elemento antes da header)
				(l->p)->n = NULL;

				// -->  Realiza o sort por ordem decrescente de angulo
				MergeSort<VLINK>(&l);
								
				// -->  Faz o desligamento das arestas auto compartilhadas
				
				// --> pp recebe o endereco do header
				VLINK ** pp = &l, * c;

				// FIX
				bool shared3Found = false;
				// -->  Para cada elemento da connective list
				while ((c = *pp) != NULL)
				{
					// --> Se nao for aresta auto-compartilhada
					// --> Ou se for auto-compartilhada com aresta NFP:
					// --> ir para o próximo descritor
					if (not HasShared3(c) || c->IsNFPLine() || c->n->IsNFPLine())
						// --> pp recebe o endereco do proximo descriptor
						pp = &c->n;
					else
					{
						// --> Modificacao 3 para arestas auto-compartilhadas
						shared3Found = true;

						// --> Aplica o label isected para o contorno
						SetPlineLabel(pline, P_ISECTED);

						// --> Aplica o label de aresta auto compartilhada e realiza o 
						// --> desligamento do descriptor do vertice
						MarkAsDone(c);

						// --> Aplica o label de aresta auto compartilhada e realiza o 
						// --> desligamento do descriptor do vertice
						MarkAsDone(c->n);

						// --> Modifica o objeto apontado por pp
						// --> exclui-se os descritores da lista
						*pp = c->n->n;
					}				
				}

				// --> Nao ha arestas auto-compartilhadas
				if (l == NULL)
					continue;

				//---------------------------------------------------------------------------
				// --> TODO: Necessita verificacao para arestas auto-compartilhadas + arestas
				// -->   internas
				
				// --> Ha apenas uma aresta NFP neste ponto (não consiste em cross-vertice)
				if(l->n == NULL) {
					LnkUntie(l);
					continue;
				}
				
				// --> Ha apenas dois descritores. Neste caso existem duas possibilidades
				// -->  - E uma interseccao trivial (cruzamento de arestas do mesmo poligono)
				// -->    e, portanto, não é um cross-vertice
				// -->  - E uma intersecção entre duas areas de um mesmo poligono, em que os
				// -->    outros dois descritores foram removidos por representarem arestas 
				// -->    auto compartilhada. Neste caso considera-se uma interseccao real.
				// --> Modificacao 3 para arestas auto-compartilhadas
				if (l->n->n == NULL  && !shared3Found && l->IsFirst() != l->n->IsFirst()) {
					LnkUntie(l);
					LnkUntie(l->n);
					continue;
				}

				// --> Modificacao 1 para arestas internas
				// --> Flag para verificar se cross-vertice possui algum no de aresta interna
				bool hasNFP = false; 
				// --> Flag para verificar se cross-vertice possui nos de apenas um poligono
				bool samepolygon = true;
				bool polygon = head->IsFirst();
				for(VLINK * k = l; k != NULL; k = k->n) {
					if(k->IsNFPLine()) hasNFP = true;
					if(k->IsFirst() != polygon) samepolygon = false;
				} 
				// --> Se ambas as condicoes forem verificadas desligar todos os descritores
				// -->  dos nos
				// --> FIXME: eliminar interse~ccoes falsas com NFP ou no inicio desta funcao
				// --> ou no sweep
				if(samepolygon && !shared3Found && hasNFP) {
					for(VLINK * k = l; k != NULL; k = k->n)
						LnkUntie(k);
					continue;
				}
				// --> Fim da Modificacao 1 para arestas internas

				//---------------------------------------------------------------------------
				
				// --> Agora, restaram apenas interseccoes reais
				
				// --> Aplica o label isected para o contorno
				SetPlineLabel(pline, P_ISECTED);
				
				// --> Marca os descriptors como validos e reconstroi a lista duplamente ligada
				// --> Connective List

				// TEST: Marcar no como intersecao
				VLINK * s = l, * q;
				while ((q = s->n) != NULL)
				{
					s->SetValid(true);
					s->vn->intersection = true;
					q->p = s, s = s->n;
					
				}
				// FIX ?
				s->SetValid(true);
				s->vn->intersection = true;
				(l->p = s)->n = l;

			} while ((vn = vn->next) != pline->head);
		}	

		// --> Modificacao 2 para arestas internas
		// --> Monta os descritores das arestas internas
		for (PLINE2 * pline = pa->NFP_lines; pline != NULL; pline = pline->next) {
			for (VNODE2 * vn = pline->head; vn != NULL; vn = vn->next) {
				// --> Busca por um header na Connective List
				VLINK * head;
				if (vn->lnk.i == NULL and vn->lnk.o == NULL)
					continue;
				if		(vn->lnk.i == NULL)
					head = vn->lnk.o;
				else if (vn->lnk.o == NULL)
					head = vn->lnk.i;
				else
					head = vn->lnk.i;
				VLINK * l = head;

				// --> Calcula os parametros dx e dy e tambem o flag L_FFIRST
				do {
					VNODE2 * vnext = (l->IsIn()) ? l->vn->prev : l->vn->next;
					l->dx = vnext->g.x - vn->g.x;
					l->dy = vnext->g.y - vn->g.y;
					l->SetFirst(IsFirst(l->vn));
				} while ((l = l->n) != head);

				// --> Se possui descritores, a aresta pode ser marcada como ISECTED
				SetPlineLabel(pline, P_ISECTED);
			}
		}
		// --> Modificacao 2 para arestas internas

	} while ((pa = pa->f) != area);
} // SortDesc

///////////////////// Routines for making labels /////////////////

// ------------------------------------------------------------------------------------------
// --> Aplica os labels para o caso de arestas compartilhadas
local bool DoShared(VLINK * lnk, VLINK * chk)
{
	// --> Verifica se os pontos finais das arestas se coincidem
	if (lnk->dx != chk->dx or lnk->dy != chk->dy)
		return false;

	// --> Como coincidem, o par de arestas e compartilhado
	ELABEL	nLabel;
	VNODE2*	vn0;
	VNODE2*	vn1;

	// --> Verifica se ambas as arestas possuem a mesma orientacao
	if (chk->IsIn() == lnk->IsIn())
	{
		// --> Define o label como SHARED1
		nLabel = E_SHARED1;

		// --> Identifica os vertices da aresta
		if (chk->IsIn())
			vn0 = chk->vn->prev,vn1 = lnk->vn->prev;
		else
			vn0 = chk->vn,		vn1 = lnk->vn;
	}
	else
	{
		// --> Define o label como SHARED2
		nLabel = E_SHARED2;

		// --> Identifica os vertices da aresta
		if (chk->IsIn())
			vn0 = chk->vn->prev,vn1 = lnk->vn;
		else
			vn0 = chk->vn,		vn1 = lnk->vn->prev;
	}

	// --> Aplica o label em ambos os nos (um em cada poligono)
	SetVnodeLabel(vn0, nLabel);
	SetVnodeLabel(vn1, nLabel);

	// --> Determina o outro vertice da aresta
	
	// --> Alteração para corrigir bug Tangram_scale0
	// --> Old Code: 
	// vn0->lnk.o->shared = vn1;	
	// vn1->lnk.o->shared = vn0;	
	if(vn0->lnk.o != NULL) vn0->lnk.o->shared = vn1;
	if(vn1->lnk.o != NULL) vn1->lnk.o->shared = vn0;
	return true;
} // DoShared

// ------------------------------------------------------------------------------------------
// --> Aplica os labels para o caso de arestas internas compartilhadas
local bool DoNFPShared(VLINK * lnk, VLINK * chk) {
	if (lnk->dx != chk->dx or lnk->dy != chk->dy)
		return false;
	
	// --> Como coincidem, o par de arestas e compartilhado
	VNODE2*	vn0;
	VNODE2*	vn1;
	
	// --> Modificacao para ignorar orientacao de cada uma das arestas
	// --> Identifica os vertices da aresta
	if(lnk->IsIn())
		vn1 = lnk->vn->prev;
	else
		vn1 = lnk->vn;

	if (chk->IsIn())
		vn0 = chk->vn->prev;			
	else
		vn0 = chk->vn;
	// --> Fim da Modificacao para ignorar orientacao de cada uma das arestas
	
	// --> Modificacao para compartilhamento de duas arestas internas
	// --> Compartilhamento de duas arestas internas
	if(chk->IsNFPLine()) {
		// --> Aplica o label em ambos os nos (um em cada poligono)
		SetVnodeLabel(vn0, E_SHARED1);
		SetVnodeLabel(vn1, E_SHARED1);
		// --> Marca os nos compartilhados para auxiliar o collect
		vn0->lnk.o->shared = vn1;
		vn1->lnk.o->shared = vn0;
	}
	// --> Compartilhamento de aresta interna com contorno
	else {
		SetVnodeLabel(vn1, E_SHARED2);
	}
	// --> Fim da Modificacao para compartilhamento de duas arestas internas

	return true;
} // DoNFPShared

// ------------------------------------------------------------------------------------------
// --> Aplica label para um no dado o seu descriptor
local bool DoLabel(VNODE2 * vn, VLINK * lnk)
{	
	// --> Busca pelo proximo descriptor da connective list que pertence ao outro poligono
	VLINK * n = NULL;
	for (VLINK * vl = lnk->n; vl != lnk; vl = vl->n)
	{
		// --> Modificacao para pular checagem de no de aresta interna
		if (vl->IsFirst() != lnk->IsFirst() && not vl->IsNFPLine())
		// --> Fim da Modificacao para pular checagem de no de aresta interna
		{
			n = vl;
			break;
		}
	}
	// --> Se nao encontrar retorne falso
	if (n == NULL)
		return false;

	// --> Busca pelo descriptor anterior da connective list que pertence ao outro poligono
	VLINK * p = NULL;	
	for (VLINK * vl = lnk->p; vl != lnk; vl = vl->p)
	{
		// --> Modificacao para pular checagem de no de aresta interna
		if (vl->IsFirst() != lnk->IsFirst()  && not vl->IsNFPLine())
		// --> Fim da Modificacao para pular checagem de no de aresta interna
		{
			p = vl;
			break;
		}
	}
	// --> Deve ter encontrado descriptor anterior (p) e nao pode ser igual ao proximo (n)
	assert(p != NULL and n != p);
	//assert(p != NULL);

	// --> Verfifica se a aresta e compartilhada
	if (DoShared(lnk, n) or DoShared(lnk, p))
		return true;
	/*if(n != p && DoShared(lnk, p))
		return true;*/
	// --> Verifica se o lnk esta dentro do poligono definido por p e n e armazena em bInside
	bool bInside = n->IsIn() and not p->IsIn();
	// --> Aplica o label INSIDE ou OUTSIDE de acordo com bInside
	SetVnodeLabel(vn, (bInside) ? E_INSIDE : E_OUTSIDE);
	return true;
} // DoLabel 

// --> Aplica label para um no dado o seu descriptor
local bool DoNFPLabel(VNODE2 * vn, VLINK * lnk, PAREA * other)
{		
	// --> Busca pelo proximo descriptor da connective list que pertence ao outro poligono
	VLINK * n = NULL;
	for (VLINK * vl = lnk->n; vl != lnk; vl = vl->n)
	{
		// --> Modificacao para incluir busca incluindo aresta interna
		if (vl->IsFirst() != lnk->IsFirst() )
		// --> Fim da Modificacao para incluir busca incluindo aresta interna
		{
			n = vl;
			break;
		}
	}
	// --> Se nao encontrar retorne falso
	if (n == NULL)
		return false;
	// --> Busca pelo descriptor anterior da connective list que pertence ao outro poligono
	VLINK * p = NULL;	
	for (VLINK * vl = lnk->p; vl != lnk; vl = vl->p)
	{
		// --> Modificacao para incluir busca incluindo aresta interna
		if (vl->IsFirst() != lnk->IsFirst()  )
		// --> Fim da Modificacao para incluir busca incluindom aresta interna
		{
			p = vl;
			break;
		}
	}
	// --> Deve ter encontrado descriptor anterior (p) e nao pode ser igual ao proximo (n)
	//assert(p != NULL and n != p);
	assert(p != NULL);
	
	// --> Verifica se a aresta e compartilhada
	//if (DoNFPShared(lnk, n) or DoNFPShared(lnk, p))
	if (DoNFPShared(lnk, p))
		return true;
	if (n != p && DoNFPShared(lnk, n))
		return true;
	
	// --> Se a aresta nao e compartilhada verifica se esta no interior do outro poligono

	// --> Modificacao para verificar se a aresta esta no interior do outro poligono

	// --> Busca pelo proximo descritor da connective list que pertence ao contorno do outro
	// -->  poligono
	n = NULL;
	for (VLINK * vl = lnk->n; vl != lnk; vl = vl->n) {		
		if (vl->IsFirst() != lnk->IsFirst()  && not vl->IsNFPLine()) {
			n = vl; break;
		}
	}
	// --> Se nao encontrou interseccao com aresta do poligono (apenas com aresta interna)
	//  a aresta  esta no interior do poligono
	if (n == NULL) {
		SetVnodeLabel(vn, GridInParea(&vn->g, other) ? E_INSIDE : E_OUTSIDE);
		return true;
	}
	// --> Busca pelo descritor anterior da connective list que pertence ao contorno do outro
	// -->  poligono
	p = NULL;	
	for (VLINK * vl = lnk->p; vl != lnk; vl = vl->p) {		
		if (vl->IsFirst() != lnk->IsFirst()  && not vl->IsNFPLine()) {
			p = vl; break;
		}
	}	
	assert(p != NULL and n != p);
	// --> Fim da Modificacao para verificar se a aresta esta no interior do outro poligono

	// --> Verifica se a aresta esta dentro do poligono definido por p e n e armazena em bInside
	bool bInside = n->IsIn() and not p->IsIn();
	// --> Aplica o label INSIDE ou OUTSIDE de acordo com bInside
	SetVnodeLabel(vn, (bInside) ? E_INSIDE : E_OUTSIDE);
	return true;
} // DoNFPLabel

// ------------------------------------------------------------------------------------------
// --> Aplica label em todos os nos de um contorno
static void LabelIsected(PLINE2 * pline, PAREA * other)
{
    VNODE2 * vn = pline->head;
    // --> Para cada no do contorno
	do {
		// --> Obtem o label atual do no
		ELABEL nLabel = GetVnodeLabel(vn);

		// --> Se o label ja foi aplicado, prosseguir para o proximo no
		if (nLabel != E_UNKNOWN)
			continue;

		// --> Busca por um cross-vertice na aresta atual (no atual e proximo) e obtem o seu
		// --> descriptor
		VLINK * lnk = NULL;
		if		(vn->lnk.o != NULL)
			lnk = vn->lnk.o;
		else if (vn->next->lnk.i != NULL)
			lnk = vn->next->lnk.i;

		// --> Caso encontrar um cross-vertice, aplicar label e prosseguir para o proximo no
		if (lnk != NULL and DoLabel(vn, lnk))
			continue;

		// --> Obtem o label do no anterior e armazena em nPrev
		ELABEL nPrev = GetVnodeLabel(vn->prev);

		// --> Verifica se o label nPrev e conhecido e e diferente de SHARED3 
		// -->  (auto compartilhado)
		if (nPrev != E_UNKNOWN and nPrev != E_SHARED3)
			// --> Aplica o label do no anterior no no atual
			SetVnodeLabel(vn, nPrev);
		else
            // --> Verifica se a aresta esta dentro ou fora do outro poligono e aplica label
			SetVnodeLabel(vn, GridInParea(&vn->g, other) ? E_INSIDE : E_OUTSIDE);
    } while ((vn = vn->next) != pline->head);
} // LabelIsected

// ------------------------------------------------------------------------------------------
// --> Aplica label em todos os nos de uma aresta interna
static void LabelNFPIsected(PLINE2 * pline, PAREA * other)
{
    // --> Para cada no da aresta
	for(VNODE2 * vn = pline->head; vn->next != NULL; vn = vn->next) {
		// --> Obtem o label atual do no
		ELABEL nLabel = GetVnodeLabel(vn);

		// --> Se o label ja foi aplicado, prosseguir para o proximo no
		if (nLabel != E_UNKNOWN)
			continue;

		// --> Busca por um cross-vertice na aresta atual (no atual e proximo) e obtem o seu
		// --> descriptor
		VLINK * lnk = NULL;
		if		(vn->lnk.o != NULL)
			lnk = vn->lnk.o;		
		else if (vn->next->lnk.i != NULL)
			lnk = vn->next->lnk.i;

		// --> Caso encontrar um cross-vertice, aplicar label e prosseguir para o proximo no
		if (lnk != NULL and DoNFPLabel(vn, lnk, other))
			continue;

		// --> Obtem o label do no anterior e armazena em nPrev		
		// Modificacao para primeiro no da aresta interna
		ELABEL nPrev;
		if(vn->prev != NULL)
			nPrev = GetVnodeLabel(vn->prev);
		else
			nPrev = E_UNKNOWN;
		// Fim da Modificacao para primeiro no da aresta interna

		// --> Verifica se o label nPrev e conhecido e e diferente de SHARED3 
		// -->  (auto compartilhado)
		if (nPrev != E_UNKNOWN and nPrev != E_SHARED3)
			// --> Aplica o label do no anterior no no atual
			SetVnodeLabel(vn, nPrev);
		else
            // --> Verifica se a aresta esta dentro ou fora do outro poligono e aplica label
			SetVnodeLabel(vn, GridInParea(&vn->g, other) ? E_INSIDE : E_OUTSIDE);
    }
} // LabelNFPIsected

// ------------------------------------------------------------------------------------------
// --> Aplica label a um contorno
local void LabelPline(PLINE2 * pline, PAREA * other)
{
    // --> Se o contorno for ISECTED, aplicar os labels nos nos do contorno
	if (GetPlineLabel(pline) == P_ISECTED)
        LabelIsected(pline, other);

	// --> Caso contrario verficar se o contorno esta dentro ou fora do outro poligono e
	// --> aplicar label
    else
        pline->Flags = SETBITS(pline, P_MASK,
			PlineInParea(pline, other) ? P_INSIDE : P_OUTSIDE);
} // LabelPline

// ------------------------------------------------------------------------------------------
// --> Aplica label a uma aresta interna
local void LabelNFPLine(PLINE2 * pline, PAREA * other)
{
    // --> Se o contorno for ISECTED, aplicar os labels nos nos da aresta
	if (GetPlineLabel(pline) == P_ISECTED)
        LabelNFPIsected(pline, other);

	// --> Caso contrario verficar se a aresta esta dentro ou fora do outro poligono e
	// --> aplicar label
    else
        pline->Flags = SETBITS(pline, P_MASK,
			PlineInParea(pline, other) ? P_INSIDE : P_OUTSIDE);
} // LabelNFPLine

// Retorna se b esta sobre a reta (a,c)
local
bool PointOnLine(const GRID2 & a, const GRID2 & b, const GRID2 & c)
{
	return	(pbDINT)(b.x - a.x) * (c.y - b.y) ==
			(pbDINT)(b.y - a.y) * (c.x - b.x);
} // PointOnLine

// ------------------------------------------------------------------------------------------
// --> Retorna se b esta sobre o segmento de reta (a,c)
local
bool PointOnSeg(const GRID2 & a, const GRID2 & b, const GRID2 & c) {
	if(b.x < min(a.x, c.x) || b.x > max(a.x, c.x)
	|| b.y < min(a.y, c.y) || b.y > max(a.y, c.y))
		return false;	
	return PointOnLine(a,b,c);
}

// ------------------------------------------------------------------------------------------
// --> Aplica label a um no interno
local void LabelNFPNode(PLINE2 * pline, PAREA * other)
{
	VNODE2 * vn = pline->head;

	// ======================================================================================
	bool onNFPLine = false;
	// --> Verifica se o no esta sobre algum no interno do outro poligono
	for (PLINE2 * NFPpline = other->NFP_nodes; NFPpline != NULL; NFPpline = NFPpline->next) {		
		if(vn->g.x == NFPpline->head->g.x && vn->g.y == NFPpline->head->g.y) {	
			onNFPLine = true;
			if(vn->NFPshared == false) {
				vn->NFPshared = true;
				NFPpline->head->NFPshared = true;
			}
			else
				SetMarked(vn);
			break;
		}
	}
	// --> Se estiver sobre no interno, aplica o label ONEDGE
	if(onNFPLine) {
		SetVnodeLabel(vn, E_ONNODE);
		return;
	}
	// ======================================================================================
	onNFPLine = false;
	// --> Verifica se o no esta sobre alguma aresta interna do outro poligono
	for (PLINE2 * NFPpline = other->NFP_lines; NFPpline != NULL; NFPpline = NFPpline->next) {
		for(VNODE2 * NFPvn = NFPpline->head; NFPvn->next != NULL; NFPvn = NFPvn->next) {
			if(PointOnSeg(NFPvn->g, vn->g, NFPvn->next->g)) {
				onNFPLine = true;
				break;
			}
		}
	}
	// --> Se estiver sobre aresta interna, aplica o label ONEDGE
	if(onNFPLine) {
		SetVnodeLabel(vn, E_ONEDGE);
		return;
	}
	// ======================================================================================
	bool onContour = false;
		// --> Verifica se o no esta sobre alguma aresta do contorno do outro poligono		
		for(PLINE2 * cntrpline = other->cntr; cntrpline != NULL && !onContour; cntrpline = 
		cntrpline->next) {
			VNODE2 * cntrvn = cntrpline->head; 
			do {
				// Verifica se o ponto esta na reta
				if(PointOnSeg(cntrvn->g, vn->g,cntrvn->next->g))
					onContour = true;
			} while ((cntrvn = cntrvn->next) != cntrpline->head && !onContour);
		}	
	if(onContour) {
		SetVnodeLabel(vn, E_ONEDGE);
		return;
	}
	// ======================================================================================

	// --> Verifica se o no esta no interior do outro poligono
	bool isInside = GridInParea(&vn->g, other);
	if(isInside)
		// --> Se nao estiver sobre aresta interna, aplica o label INSIDE
		SetVnodeLabel(vn, E_INSIDE);	
	else 
		// --> Se estiver no exterior do outro poligono aplica o label OUTSIDE
		SetVnodeLabel(vn, E_OUTSIDE);	
} // LabelNFPNode

// ------------------------------------------------------------------------------------------
// --> Aplica label a um poligono
local void LabelParea(PAREA * area, PAREA * other)
{
    PAREA * pa = area;
    // --> Para todo contorno do poligono, aplica os labels
	do {
		for (PLINE2 * pline = pa->cntr; pline != NULL; pline = pline->next)
            LabelPline(pline, other);

		// --> Modificacao 1 para aplicar label nas arestas internas
		for (PLINE2 * pline = pa->NFP_lines; pline != NULL; pline = pline->next)
			LabelNFPLine(pline, other);            
		// --> Fim da Modificacao 1 para aplicar label nas arestas internas

		// --> Modificacao 2 para aplicar label nos nos internos
		for (PLINE2 * pline = pa->NFP_nodes; pline != NULL; pline = pline->next)
			LabelNFPNode(pline, other);
		
		// --> Fim da Modificacao 2 para aplicar label nos nos internos

    } while ((pa = pa->f) != area);
} // LabelParea

////////////////////// Collect ////////////////////////////////

enum DIRECTION { FORW, BACKW };

// EdgeRule
typedef bool	(*EDGERULE)(const VNODE2 *, DIRECTION *);
typedef bool	(*CNTRRULE)(const PLINE2  *, DIRECTION *);

// ------------------------------------------------------------------------------------------
// --> Regra de inclusao da aresta no resultado para a operacao de UNIAO
static bool EdgeRuleUn(const VNODE2 * vn, DIRECTION * dir)
{
	ELABEL nLabel = GetVnodeLabel(vn);
	if (nLabel == E_OUTSIDE or nLabel == E_SHARED1)
	{
		*dir = FORW;
		return true;
	}
	return false;
} // EdgeRuleUn

// ------------------------------------------------------------------------------------------
// --> Regra de inclusao da aresta no resultado para a operacao de INTERSECCAO
static bool EdgeRuleIs(const VNODE2 * vn, DIRECTION * dir)
{
	ELABEL nLabel = GetVnodeLabel(vn);
	if (nLabel == E_INSIDE or nLabel == E_SHARED1)
	{
		*dir = FORW;
		return true;
	}
	return false;
} // EdgeRuleIs

// ------------------------------------------------------------------------------------------
// --> Regra de inclusao da aresta no resultado para a operacao de SUBTRACAO
static bool EdgeRuleSb(const VNODE2 * vn, DIRECTION * dir)
{
	ELABEL nLabel = GetVnodeLabel(vn);
	if (IsFirst(vn))
	{
		if (nLabel == E_OUTSIDE or nLabel == E_SHARED2)
		{
			*dir = FORW;
			return true;
		}
	}
	else
	{
		if (nLabel == E_INSIDE or nLabel == E_SHARED2)
		{
			*dir = BACKW;
			return true;
		}
	}
	return false;
} // EdgeRuleSb

// ------------------------------------------------------------------------------------------
// --> Regra de inclusao da aresta no resultado para a operacao de XOR
static bool EdgeRuleXr(const VNODE2 * vn, DIRECTION * dir)
{
	ELABEL nLabel = GetVnodeLabel(vn);
	if		(nLabel == E_OUTSIDE)
	{
		*dir = FORW;
		return true;
	}
	else if (nLabel == E_INSIDE)
	{
		*dir = BACKW;
		return true;
	}
	return false;
} // EdgeRuleXr

// ------------------------------------------------------------------------------------------
// --> Regra de inclusao de contorno no resultado para a operacao de UNIAO
static bool CntrRuleUn(const PLINE2 * pline, DIRECTION * dir)
{
	PLABEL nLabel = GetPlineLabel(pline);
	if (nLabel == P_OUTSIDE)
	{
		*dir = FORW;
		return true;
	}
	return false;
} // CntrRuleUn

// ------------------------------------------------------------------------------------------
// --> Regra de inclusao de contorno no resultado para a operacao de INTERSECCAO
static bool CntrRuleIs(const PLINE2 * pline, DIRECTION * dir)
{
	PLABEL nLabel = GetPlineLabel(pline);
	if (nLabel == P_INSIDE)
	{
		*dir = FORW;
		return true;
	}
	return false;
} // CntrRuleIs

// ------------------------------------------------------------------------------------------
// --> Regra de inclusao de contorno no resultado para a operacao de SUBTRACAO
static bool CntrRuleSb(const PLINE2 * pline, DIRECTION * dir)
{
	PLABEL nLabel = GetPlineLabel(pline);
	if (IsFirst(pline->head))
	{
		if (nLabel == P_OUTSIDE)
		{
			*dir = FORW;
			return true;
		}
	}
	else
	{
		if (nLabel == P_INSIDE)
		{
			*dir = BACKW;
			return true;
		}
	}
	return false;
} // CntrRuleSb

// ------------------------------------------------------------------------------------------
// --> Regra de inclusao de contorno no resultado para a operacao de XOR
static bool CntrRuleXr(const PLINE2 * pline, DIRECTION * dir)
{
	PLABEL nLabel = GetPlineLabel(pline);
	if		(nLabel == P_OUTSIDE)
	{
		*dir = FORW;
		return true;
	}
	else if (nLabel == P_INSIDE)
	{
		*dir = BACKW;
		return true;
	}
	return false;
} // CntrRuleXr
	
// ------------------------------------------------------------------------------------------
// --> Detecta criacao de no interno
local bool DetectInsideCrossing(VNODE2 * vn, PAREA::PBOPCODE nOpCode) {
	// --> Verifica se e cross-vertice
	if((vn->lnk.i == NULL && vn->lnk.o == NULL) || IsInNode(vn))
		return false;

	// --> Busca por um descritor
	VLINK * lnk = NULL;
	if		(vn->lnk.o != NULL)
		lnk = vn->lnk.o;
	else if (vn->lnk.i != NULL)
		lnk = vn->lnk.i;	

	if(nOpCode == PAREA::UN) {		
		VLINK *curlnk = lnk;
		ELABEL curlabel = E_INSIDE;
		bool samepolygon = true;
		// --> Varre Connective List e verifica se existem apenas arestas INSIDE nos 2 
		// --> poligonos
		do {
			// --> Obtem o label da aresta e atualiza a contagem
			if(curlnk->IsIn())
				curlabel = GetVnodeLabel(curlnk->vn->prev);				
			else
				curlabel = GetVnodeLabel(curlnk->vn);
			if(curlnk->IsFirst() != curlnk->n->IsFirst())
				samepolygon = false;
		// --> Repete ate o fim da connective list ou ate encontrar uma aresta diferente de 
		// --> INSIDE
		} while ((curlnk = curlnk->n) != lnk && curlabel == E_INSIDE);	
		return (not samepolygon && curlabel == E_INSIDE);
	}
	if(nOpCode == PAREA::SB) {
		VLINK *curlnk = lnk;
		bool detectioncondition = true;
		ELABEL curlabel;
		bool samepolygon = true;
		do {
			// --> Obtem o label da aresta e atualiza a contagem
			if(curlnk->IsIn())
				curlabel = GetVnodeLabel(curlnk->vn->prev);				
			else
				curlabel = GetVnodeLabel(curlnk->vn);
			if( not (curlnk->IsFirst() && curlabel == E_INSIDE) &&
				not (not curlnk->IsFirst() && curlabel == E_OUTSIDE))
					detectioncondition = false;
			if(curlnk->IsFirst() != curlnk->n->IsFirst())
				samepolygon = false;
		// --> Repete ate o fim da connective list ou ate encontrar uma aresta diferente de 
		// --> INSIDE
		} while ((curlnk = curlnk->n) != lnk && detectioncondition);	
		return (not samepolygon && detectioncondition);
	}

	return false;

} // DetectInsideCrossing

// ------------------------------------------------------------------------------------------
// --> Marca todos o nos da Connective List como InNode
local void SetAllInNode(VNODE2 *vn) {
	VLINK * lnk = NULL;
	if		(vn->lnk.o != NULL)
		lnk = vn->lnk.o;
	else if (vn->lnk.i != NULL)
		lnk = vn->lnk.i;
	VLINK * currlnk = lnk;
	do {
		SetInNode(currlnk->vn);
	}
	while ((currlnk = currlnk->n) != lnk);
} // SetAllInNode

// ------------------------------------------------------------------------------------------
// --> Detecta criacao de arestas e nos NFP
local void GenerateNFP(VNODE2 * vn, PAREA ** r, PAREA::PBOPCODE nOpCode) {
	if (nOpCode == PAREA::UN) {
		// --> Modificacao 1 para coleta arestas internas geradas por dois poligonos 
		// -->  (SHARED2)
		if (GetVnodeLabel(vn) == E_SHARED2 && not IsInNode(vn)) {
			PLINE2 *p;
			PLINE2::CreateNPFLine(&p,  vn->g,  vn->next->g);
			PAREA::InclPNFPline(r, p);
			// --> Marca o no correspondente
			SetInNode(vn->lnk.o->shared);
		}
		// --> Fim da Modificacao 1 para coleta arestas internas geradas por dois poligonos 
		// -->  (SHARED2)

		// --> Modificacao 2 para Coleta nos internos geradas por dois poligonos
		if(DetectInsideCrossing(vn, nOpCode)) {
			PLINE2 *p;
			PLINE2::CreateNPFNode(&p, vn->g);
			PAREA::InclPNFPnode(r, p);
			// --> Marca todos os nos do cross-vertice
			SetAllInNode(vn);
		}
		// --> Fim da Modificacao 2 para Coleta nos internos geradas por dois poligonos
	}
	if (nOpCode == PAREA::SB) {
		// --> Modificacao 1 para coleta arestas internas geradas por dois poligonos 
		// -->  (SHARED1)
		if (GetVnodeLabel(vn) == E_SHARED1 && not IsInNode(vn)) {
			PLINE2 *p;
			PLINE2::CreateNPFLine(&p,  vn->g,  vn->next->g);
			PAREA::InclPNFPline(r, p);
			// --> Marca o no correspondente
			SetInNode(vn->lnk.o->shared);
		}
		// --> Fim da Modificacao 1 para coleta arestas internas geradas por dois poligonos 
		// -->  (SHARED1)

		// --> Modificacao 2 para Coleta nos internos geradas por dois poligonos
		if(DetectInsideCrossing(vn, nOpCode)) {
			PLINE2 *p;
			PLINE2::CreateNPFNode(&p, vn->g);
			PAREA::InclPNFPnode(r, p);
			// --> Marca todos os nos do cross-vertice
			SetAllInNode(vn);
		}
		// --> Fim da Modificacao 2 para Coleta nos internos geradas por dois poligonos
	}
}

// ------------------------------------------------------------------------------------------
// --> Determina o proximo no quando se encontra em uma interseccao (jump rules)
local VNODE2 * Jump(VNODE2 * cur, DIRECTION * cdir, EDGERULE eRule)
{
	// --> Determina a aresta correspondente ao no cur
	VLINK *start = (*cdir == FORW) ? cur->lnk.i : cur->lnk.o;

	// --> Verifica se o no cur e um cross-vertice
	if (start != NULL)		
		// --> Para cada elemento da Connective list do no cur diferente de start, em ordem 
		// -->  decrescente de angulo (sentido horario)
		for (VLINK *n = start->p; n != start; n = n->p)
		{
			// --> Se as arestas sao compartilhadas, prosseguir para o proximo descriptor

			// --> Adicionado modificacao para pular a checagem dos nos das arestas internas
			if ((start->dx == n->dx and start->dy == n->dy) || n->IsNFPLine())
				continue;
			// --> Fim da modificacao para nao checar o nos das arestas internas

			// --> Verifica se a aresta obedece a regra de inclusao da operacao e, caso 
			// --> obedeca, retornar o no correspondente
			if (n->IsIn() and eRule(n->vn->prev, cdir) or
			   !n->IsIn() and eRule(n->vn, cdir))
				return n->vn;
		}

	// --> Se nao encontrar o proximo, retornar o proprio no cur
	return cur;
} // Jump

// ------------------------------------------------------------------------------------------
// --> Realiza o collect das arestas de um contorno (inclui no resultado final)
static void CollectVnode(VNODE2 * start, PLINE2 ** result, EDGERULE edgeRule, DIRECTION 
initdir)
{
    // --> No inicial da aresta
	VNODE2*	V = start;

	// --> No final da aresta
	VNODE2*	E = (initdir == FORW) ? start : start->prev;

	// --> Direcao da aresta: original (FORWARD) ou inversa (BACKWARD)
    DIRECTION   dir  = initdir;
    
	do {
        // --> Inclui o no inicial da aresta
		PLINE2::Incl(result, V->g);

        // --> Marcar o ponto final da aresta
		SetMarked(E);
        
        // --> Para aresta compartilhada, marcar também a aresta vizinha (que e compartilhada
		// -->  com VE) para nao incluir duas vezes a mesma aresta
		if (GetVnodeLabel(E) == E_SHARED1 or
			GetVnodeLabel(E) == E_SHARED2)			
			// --> Alteração para corrigir bug Tangram_scale0
			// --> Old Code: 
			// SetMarked(E->lnk.o->shared);
            if(E->lnk.o != NULL) SetMarked(E->lnk.o->shared);
		
		// --> Segue para a proxima aresta, se necessario utilizar as jump rules
        V = Jump((dir == FORW) ? V->next : V->prev, &dir, edgeRule);		
        E = (dir == FORW) ? V : V->prev;
		// assert(GetVnodeLabel(E) != E_SHARED3);

    } while (not IsMarked(E));
} // CollectVnode

// ------------------------------------------------------------------------------------------
// --> Realiza o collect de um contorno (inclui no resultado final)
static void CollectPline(PLINE2 * pline, PAREA ** r, PLINE2 ** holes, PAREA::PBOPCODE 
nOpCode)
{
	// --> Verifica se o contorno e ISECTED
	PLABEL nLabel = GetPlineLabel(pline);
    if (nLabel == P_ISECTED)
    {
		// --> Para contorno ISECTED utiliza-se das regras de inclusao de arestas
		static const EDGERULE edgeRule[] =
		{
			EdgeRuleUn,		//	PBO_UNITE,
			EdgeRuleIs,		//	PBO_ISECT,
			EdgeRuleSb,		//	PBO_SUB,
			EdgeRuleXr		//	PBO_XOR
		};
		
		// --> Para todo no do contorno
		VNODE2 *vn = pline->head;
		do {
			DIRECTION dir;
			// --> Verfica se o no ja foi adicionado e se obedece as regras de inclusao e 
			// --> obtem a direcao
			if (not IsMarked(vn) and edgeRule[nOpCode](vn, &dir))
			{				
				PLINE2 *p = NULL;
				// --> Realiza o collect das arestas a partir de vn e armazena no contorno p
				CollectVnode((dir == FORW) ? vn : vn->next, &p, edgeRule[nOpCode], dir);

				// --> Prepara o contorno p e adiciona no resultado
				if (p->Prepare())
					PLINE2::Put(p, r, holes);				
				// --> Se nao foi possivel preparar. Apaga o contorno				
				else
					PLINE2::Del(&p);
			}
		} while ((vn = vn->next) != pline->head);

		// --> Modificacao para geracao de arestas e nos NFP
		do {
			GenerateNFP(vn, r, nOpCode);
		} while ((vn = vn->next) != pline->head);
		// --> Fim da modificacao para geracao de arestas e nos NFP


    } // nLabel == P_ISECTED
    else
    {
		// --> Para contorno NAO ISECTED utiliza-se das regras de inclusao de contorno
		static const CNTRRULE cntrRule[] = 
		{
			CntrRuleUn,		//	PBO_UNITE,
			CntrRuleIs,		//	PBO_ISECT,
			CntrRuleSb,		//	PBO_SUB,
			CntrRuleXr		//	PBO_XOR
		};

		// --> Verfica se o contorno obedece as regras de inclusao e obtem a direcao
		DIRECTION dir;
		if (cntrRule[nOpCode](pline, &dir))
		{
			// --> Cria uma copia do contorno
			PLINE2 * copy = pline->Copy();

			// --> Prepara o contorno p e verifica se foi bem sucedida a preparacao
			if (not copy->Prepare())
				// --> Se nao foi possivel preparar. Apaga o contorno
				PLINE2::Del(&copy);
			else
			{
				// --> Se a direcao do contorno for inversa, inverter o contorno.
				if (dir == BACKW)
					copy->Invert();

				// --> Inclui o contorno no resultado
				PLINE2::Put(copy, r, holes);
			}
		}
    }
} // CollectPline

// --> Realiza o collect dos nos internos (inclui no resultado final)
static void CollectNFPNode(PLINE2 * pline, PAREA ** r,  PAREA::PBOPCODE nOpCode) {
	VNODE2 * vn = pline->head;
	if (nOpCode == PAREA::UN) {
		// --> Inclui o no no resultado se ele for OUTSIDE
		if(GetVnodeLabel(vn) == E_OUTSIDE || GetVnodeLabel(vn) == E_ONEDGE ||
		(GetVnodeLabel(vn) == E_ONNODE && not IsMarked(vn))) {
			PLINE2 *p;
			PLINE2::CreateNPFNode(&p, vn->g);
			PAREA::InclPNFPnode(r, p);
		}
	}
	// --> FIXME: Does not detect inside crossings or NFP line generation
	if (nOpCode == PAREA::IS) {
		// --> Inclui o no no resultado se ele for OUTSIDE
		if(GetVnodeLabel(vn) == E_INSIDE || GetVnodeLabel(vn) == E_ONEDGE ||
		(GetVnodeLabel(vn) == E_ONNODE && not IsMarked(vn))) {
			PLINE2 *p;
			PLINE2::CreateNPFNode(&p, vn->g);
			PAREA::InclPNFPnode(r, p);
		}
	}

	if (nOpCode == PAREA::SB) {
		ELABEL curlabel = GetVnodeLabel(vn);
		if(
		(IsFirst(vn) && curlabel == E_OUTSIDE) ||			
		(not IsFirst(vn) && curlabel == E_INSIDE) ||
		curlabel == E_ONEDGE ||
		(curlabel == E_ONNODE && not IsMarked(vn))
		) {
			PLINE2 *p;
			PLINE2::CreateNPFNode(&p, vn->g);
			PAREA::InclPNFPnode(r, p);
		}
	}
}
	
// --> Realiza o collect das arestas internas (inclui no resultado final)
static void CollectNFPPline(PLINE2 * pline, PAREA ** r, PAREA::PBOPCODE nOpCode) {
	if (nOpCode == PAREA::UN) {
		// --> Para todos os nos da aresta interna
		for(VNODE2 * vn = pline->head; vn->next != NULL; vn = vn->next) {
			ELABEL curlabel = GetVnodeLabel(vn);
			// --> Verifica se a aresta ja foi adicionada
			if(not IsMarked(vn)) {
				// --> Adiciona aresta se ela for OUTSIDE ou compartilhada
				if(curlabel == E_OUTSIDE || curlabel == E_SHARED1 || curlabel == E_SHARED2 )
				{
					PLINE2 *p;
					PLINE2::CreateNPFLine(&p,  vn->g,  vn->next->g);
					PAREA::InclPNFPline(r, p);
					SetMarked(vn);
					// --> Se for compartilhada com outra aresta interna, marcar como 
					// --> incluida a outra aresta compartilhada
					if(curlabel == E_SHARED1)
						SetMarked(vn->lnk.o->shared);
				}
			}

			// --> Coleta nos internos geradas por arestas internas		
			if(not IsMarked(vn) && DetectInsideCrossing(vn, nOpCode)) {
				PLINE2 *p;
				PLINE2::CreateNPFNode(&p, vn->g);
				PAREA::InclPNFPnode(r, p);
				// --> Marca todos os nos do cross-vertice
				SetAllInNode(vn);								
			}
			if(not IsMarked(vn->next) && DetectInsideCrossing(vn->next, nOpCode)) {
				PLINE2 *p;
				PLINE2::CreateNPFNode(&p, vn->next->g);
				PAREA::InclPNFPnode(r, p);
				// --> Marca todos os nos do cross-vertice
				SetAllInNode(vn->next);								
			}
		}
	}
	// --> FIXME: Does not detect inside crossings or NFP line generation
	if (nOpCode == PAREA::IS) {
		// --> Para todos os nos da aresta interna
		for(VNODE2 * vn = pline->head; vn->next != NULL; vn = vn->next) {
			ELABEL curlabel = GetVnodeLabel(vn);
			// --> Verifica se a aresta ja foi adicionada
			if(not IsMarked(vn)) {
				// --> Regras para coleta de arestas
				if(curlabel == E_INSIDE || curlabel == E_SHARED1 || curlabel == E_SHARED2) {								
					PLINE2 *p;
					PLINE2::CreateNPFLine(&p,  vn->g,  vn->next->g);
					// TEST: Marcar no como intersecao
					p->head->next->intersection = vn->next->intersection;
					p->head->intersection = vn->intersection;
					PAREA::InclPNFPline(r, p);
					SetMarked(vn);
					// --> Se for compartilhada com outra aresta interna, marcar como 
					// --> incluida a outra aresta compartilhada
					if(curlabel == E_SHARED1)
						SetMarked(vn->lnk.o->shared);
				}
			}
		}
	}

	if (nOpCode == PAREA::SB) {
		// --> Para todos os nos da aresta interna
		for(VNODE2 * vn = pline->head; vn->next != NULL; vn = vn->next) {
			ELABEL curlabel = GetVnodeLabel(vn);
			// --> Verifica se a aresta ja foi adicionada
			if(not IsMarked(vn)) {
				// --> Regras para coleta de arestas
				if((curlabel == E_OUTSIDE && IsFirst(vn)) ||
				   (curlabel == E_INSIDE && not IsFirst(vn))
				   || curlabel == E_SHARED1 || curlabel == E_SHARED2) {								
					PLINE2 *p;
					PLINE2::CreateNPFLine(&p,  vn->g,  vn->next->g);
					PAREA::InclPNFPline(r, p);
					SetMarked(vn);
					// --> Se for compartilhada com outra aresta interna, marcar como 
					// --> incluida a outra aresta compartilhada
					if(curlabel == E_SHARED1)
						SetMarked(vn->lnk.o->shared);
				}
			}
			// --> Coleta nos internos geradas por duas arestas internas		
			if(not IsMarked(vn) && DetectInsideCrossing(vn, PAREA::SB)) {
				PLINE2 *p;
				PLINE2::CreateNPFNode(&p, vn->g);
				PAREA::InclPNFPnode(r, p);
				// --> Marca todos os nos do cross-vertice
				SetAllInNode(vn);
			}
			if(not IsMarked(vn->next) && DetectInsideCrossing(vn->next, PAREA::SB)) {
				PLINE2 *p;
				PLINE2::CreateNPFNode(&p, vn->next->g);
				PAREA::InclPNFPnode(r, p);
				// --> Marca todos os nos do cross-vertice
				SetAllInNode(vn->next);								
			}
		}
	}
}

// ------------------------------------------------------------------------------------------
// --> Realiza o collect de um poligono
local void CollectParea(PAREA * area, PAREA ** r, PLINE2 ** holes, PAREA::PBOPCODE nOpCode)
{
	PAREA * pa = area;
	// --> Para todo contorno do poligono, realiza collect
    do {
        for (PLINE2 * pline = pa->cntr; pline != NULL; pline = pline->next)
            CollectPline(pline, r, holes, nOpCode);

		// --> Modificacao 1 para realizar o Collect das arestas internas
		for (PLINE2 * pline = pa->NFP_lines; pline != NULL; pline = pline->next)
			CollectNFPPline(pline, r, nOpCode);
		// --> Fim da Modificacao 1 para realizar o Collect das arestas internas

		// --> Modificacao 2 para realizar o Collect dos nos internos
		for (PLINE2 * pline = pa->NFP_nodes; pline != NULL; pline = pline->next)
			CollectNFPNode(pline, r, nOpCode);
		// --> Fim da Modificacao 2 para realizar o Collect dos nos internos
    } while ((pa = pa->f) != area);
} // CollectParea

// ------------------------------------------------------------------------------------------
// --> Realiza a operacao booleana nOpCode para os poligonos a e b (a nOpCode b)
static void DoBoolean(PAREA * a, PAREA * b, PAREA ** r, PAREA::PBOPCODE nOpCode)
{
	
	// --> Modificacao
	MarkNFPLinks(a);
	MarkNFPLinks(b);
	// --> Fim da Modificacao
	
	// --> Valida as interseccoes e realiza o sort por ordem crescente de angulo dos 
	// --> descriptors dos poligonos a e b
	SortDesc(a);
	SortDesc(b);
	
	//DEBUG: Print Connect Lists for polygons A and B
	//printLinks(a, "PolygonA_links.txt");
	//printLinks(b, "PolygonB_links.txt");

	// --> Aplica os labels nos poligonos a e b
	LabelParea(a, b);
	LabelParea(b, a);
	
	// --> Realiza o collect nos poligonos a e b e armazena os contornos externos em r e os
	// -->  internos em holes
	PLINE2 * holes = NULL;
    CollectParea(a, r, &holes, nOpCode);
    CollectParea(b, r, &holes, nOpCode);

	// --> Tenta inserir os contornos internos em r para obter o poligono resultante
	try
	{
		PAREA::InsertHoles(r, &holes);
	}
	catch (int)
	{
		PLINE2::Del(&holes);
		PAREA::Del(r);
		throw;
	}
} // DoBoolean

//////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------
// --> Retorna se a esta a esquerda de b ou a.x == b.x e a.y < b.y
local bool Left(const GRID2 & a, const GRID2 & b)
{ return (a.x < b.x) or (a.x == b.x and a.y < b.y); }

// ------------------------------------------------------------------------------------------
// --> Obtem segmentos SEGM2 a partir de um poligono PAREA
static void Area2Segms(PAREA * area, SEGM2 ** pSegm)
{
	if (area == NULL)
		return;
	
	PAREA * pa = area;
	do {
		for (PLINE2 * pline = pa->cntr; pline != NULL; pline = pline->next)
		{
			VNODE2 * vn = pline->head;
			do {
				SEGM2 * s = *pSegm; (*pSegm)++;

				s->m_bRight = Left(vn->g, vn->next->g);
				if (s->m_bRight)
					s->r = (s->l = vn)->next;
				else
					s->l = (s->r = vn)->next;
			} while ((vn = vn->next) != pline->head);
		}

		// --> Modificacao para incluir arestas internas
		for (PLINE2 * pline = pa->NFP_lines; pline != NULL; pline = pline->next)
		{
			for (VNODE2 * vn = pline->head; vn->next != NULL; vn = vn->next) {
				SEGM2 * s = *pSegm; (*pSegm)++;

				s->m_bRight = Left(vn->g, vn->next->g);
				if (s->m_bRight)
					s->r = (s->l = vn)->next;
				else
					s->l = (s->r = vn)->next;
			}
		}
		// --> Fim da Modificacao para incluir arestas internas

	} while ((pa = pa->f) != area);
} // Area2Segms


// ------------------------------------------------------------------------------------------
// --> Obtem o numero de vertices de um poligono
static UINT VertCnt(const PAREA * area)
{
	if (area == NULL)
		return 0;
	
	UINT nCnt = 0;
	const PAREA * pa = area;
	do {
		for (const PLINE2 * pline = pa->cntr; pline != NULL; pline = pline->next)
		{
			const VNODE2 * vn = pline->head;
			do {
				nCnt++;
			} while ((vn = vn->next) != pline->head);
		}
		
		// --> Modificacao para incluir arestas internas
		// --> Realiza a contagem dos nos de todas as arestas internas
		for (const PLINE2 * pline = pa->NFP_lines; pline != NULL; pline = pline->next) {
			for (VNODE2 * vn = pline->head; vn->next != NULL; vn = vn->next)	
				nCnt++;			
		}
		// --> Modificacao para incluir arestas internas

	} while ((pa = pa->f) != area);
	return nCnt;
} // VertCnt

// ------------------------------------------------------------------------------------------
// --> Inicializa um poligono
static void InitArea(PAREA * area, bool bFirst)
{
	PAREA * pa = area;
	do {
		// --> Para todo contorno do poligono pa
		for (PLINE2 * pline = pa->cntr; pline != NULL; pline = pline->next)
		{
            // --> Reseta as flags do contorno
			pline->Flags = SETBITS(pline, PLINE2::RESERVED, P_UNKNOWN);

			// --> Para todo no do contorno pline
			VNODE2 * vn = pline->head;
			do {
                // --> Reseta as flags do no
				vn->Flags = SETBITS(vn, VNODE2::RESERVED, E_UNKNOWN);
				
				// --> Aplica a flag L_FFIRST
				SetFirst(vn, bFirst);

                // --> Desliga os descriptors do no
				vn->lnk.i = vn->lnk.o = NULL;
			} while ((vn = vn->next) != pline->head);
		}

		// --> Modificacao para arestas internas
		for (PLINE2 * pline = pa->NFP_lines; pline != NULL; pline = pline->next)
		{
            // --> Reseta as flags da aresta interna
			pline->Flags = SETBITS(pline, PLINE2::RESERVED, P_UNKNOWN);

			// --> Para todo no da aresta
			for (VNODE2 * vn = pline->head; vn != NULL; vn = vn->next) {
                // --> Reseta as flags do no
				vn->Flags = SETBITS(vn, VNODE2::RESERVED, E_UNKNOWN);
				
				// --> Aplica a flag L_FFIRST
				SetFirst(vn, bFirst);

                // --> Desliga os descriptors do no
				vn->lnk.i = vn->lnk.o = NULL;
			}
		}
		// --> Fim da Modificacao para arestas internas

		// --> Modificacao para nos internos
		for (PLINE2 * pline = pa->NFP_nodes; pline != NULL; pline = pline->next)
		{
            // --> Reseta as flags da aresta interna
			pline->Flags = SETBITS(pline, PLINE2::RESERVED, P_UNKNOWN);
			
			VNODE2 * vn = pline->head;
			
			// --> Reseta as flags do no
			vn->Flags = SETBITS(vn, VNODE2::RESERVED, E_UNKNOWN);
				
			// --> Aplica a flag L_FFIRST
			SetFirst(vn, bFirst);
			
			// --> Desliga os descriptors do no
			vn->lnk.i = vn->lnk.o = NULL;
			
		}
		// --> Fim da Modificacao para nos internos

	} while ((pa = pa->f) != area);
} // InitArea

// ------------------------------------------------------------------------------------------
// --> Procedimento de insercao de no
static void InsertNode(void ** _list, VNODE2 * vn, void * parm)
{
	// --> Se o no ja possuir os seus descriptos, sai da funcao
	if (vn->lnk.i != NULL) // already tied
	{
		assert(vn->lnk.o != NULL);
		return;
	}

	// --> Faz a ligacao de todos os segmentos que possuem o mesmo ponto (X,Y)

	// --> Cria os descriptors para inserir na Connective List do cross-vertice do no vn
	VLINK ** list = (VLINK **)_list;
	LNK_HEAP * pHeap = (LNK_HEAP *)parm;
	VLINK * i = pHeap->Get(true);
	VLINK * o = pHeap->Get(true);
	i->SetIn(true);
	i->SetValid(false);
	o->SetIn(false);
	o->SetValid(false);
	i->vn = o->vn = vn;

	// --> Se nao existe nenhuma connective list, cria uma com o descriptor D- (link out)
	if (*list == NULL)
		*list = o->n = o->p = o;
	// --> Se existe nenhuma connective list, insere o descriptor D- na lista
	else
		(((o->p = (*list)->p)->n = o)->n = *list)->p = o;

	// --> Insere o descriptor D+ (link in) na connective list
	(((i->p = o->p)->n = i)->n = o)->p = i;

	// --> Atribui os descriptors ao no vn
	vn->lnk.i = i;
	vn->lnk.o = o;
} // InsertNode

// ------------------------------------------------------------------------------------------
// --> Recalcula o numero de vertices de cada contorno do poligono area
local void RecalcCount(PAREA * area)
{
	PAREA * pa = area;
	do {
		for (PLINE2 * pline = pa->cntr; pline != NULL; pline = pline->next)
		{
			VNODE2 * vn = pline->head;
			UINT nCount = 0;
			do {
				nCount++;
			} while ((vn = vn->next) != pline->head);
			pline->Count = nCount;
		}
	} while ((pa = pa->f) != area);
} // RecalcCount

// ------------------------------------------------------------------------------------------
// --> Realiza a operacao boolena nOpCode com os conjuntos de poligono a e b e armazena o 
// --> resultado em r, modificando a e b
int PAREA::Boolean0(PAREA * a, PAREA * b, PAREA ** r, PBOPCODE nOpCode)
{
	*r = NULL;

	assert(a->CheckDomain());
	assert(b->CheckDomain());

	SEGM2 * aSegms = NULL;
	int err = 0;
	try
	{
		// --> Inicializa conjunto de poligonos a e b
		InitArea(a, true);
		InitArea(b, false);

		POLYBOOL pb;
		{
			UINT	nSegms = VertCnt(a) + VertCnt(b);
			aSegms = (SEGM2 *)calloc(nSegms, sizeof(SEGM2));
			if (aSegms == NULL)
				error(err_no_memory);
			SEGM2 * pSegm = aSegms;

			// --> Obtem segmentos SEGM2 a partir do conjunto de poligonos a e b
			Area2Segms(a, &pSegm);
			Area2Segms(b, &pSegm);

			assert(pSegm == aSegms + nSegms);

			// --> Realiza a varredura para obter as interseccoes
			BOCTX bc(InsertNode, &pb.m_LinkHeap);
			bc.Sweep(aSegms, nSegms);
		}
		// --> Recalcula o numero de vertices dos contornos do conjunto de poligonos a e b
		RecalcCount(a);
		RecalcCount(b);	
		
		// DEBUG: Print polygons after Hobby's Sweep algorithm
		//POLYBOOLEAN::SaveParea("PolygonA_pass2.txt", a);
		//POLYBOOLEAN::SaveParea("PolygonB_pass2.txt", b);

		// --> Realiza a operacao booleana		
		DoBoolean(a, b, r, nOpCode);
	}
	catch (int e)
	{
		err = e;
	}
	catch (const STD::bad_alloc &)
	{
		err = err_no_memory;
	}
	free(aSegms);
	return err;
} // PAREA::Boolean0

// ------------------------------------------------------------------------------------------
// --> Realiza a operacao boolena nOpCode com os conjuntos de poligono a e b e armazena o 
// --> resultado em r, sem modificar a e b
int PAREA::Boolean(const PAREA * _a, const PAREA * _b, PAREA ** r, PBOPCODE nOpCode)
{
	*r = NULL;

	if (_a == NULL and _b == NULL)
		return 0;

	if		(_b == NULL)
	{
		int err = 0;
		if (_a != NULL and (nOpCode == SB or nOpCode == XR or nOpCode == UN))
		{
			try
			{
				*r = _a->Copy();
			}
			catch (int e)
			{
				err = e;
			}
		}
		return err;
	}
	else if (_a == NULL)
	{
		int err = 0;
		if (nOpCode == XR or nOpCode == UN)
		{
			try
			{
				*r = _b->Copy();
			}
			catch (int e)
			{
				err = e;
			}
		}
		return err;
	}

	PAREA * a = NULL, * b = NULL;
	if (_a != NULL and (a = _a->Copy()) == NULL)
		return err_no_memory;
	if (_b != NULL and (b = _b->Copy()) == NULL)
	{
		PAREA::Del(&a);
		return err_no_memory;
	}
	int err = Boolean0(a, b, r, nOpCode);

	PAREA::Del(&a);
	PAREA::Del(&b);
	return err;
} // PAREA::Boolean

} // namespace POLYBOOLEAN

