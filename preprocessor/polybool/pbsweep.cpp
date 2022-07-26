//	pbsweep.cpp - plane sweep on integer grid
//
//	This file is a part of PolyBoolean software library
//	(C) 1998-1999 Michael Leonov
//	Consult your license regarding permissions and restrictions
//

#include "pbarea.h"

namespace POLYBOOLEAN
{
//------------------------------------------------------------------------------------------
// --> Compara dois eventos durante insercao na fila com prioridades
pbINT BOCTX::EVENT::Compare(const EVENT &a, const EVENT &b)
{
	assert(INT20_MIN <= a.x and a.x <= INT20_MAX);
	assert(INT20_MIN <= a.y and a.y <= INT20_MAX);
	assert(INT20_MIN <= b.x and b.x <= INT20_MAX);
	assert(INT20_MIN <= b.y and b.y <= INT20_MAX);

	if (a.x != b.x)
		return a.x - b.x;

	// --> Determina o tipo de evento (START, INTERSECCAO, END)
	TYPE aType = a.GetType();
	TYPE bType = b.GetType();

	int asx, asy, bsx, bsy;

	// --> Se o tipo do evento a for INTERSECCAO
	if (aType == X)
		asx = a.GetSignX(), asy = a.GetSignY();
	else
		asx = asy = 0;

	// --> Se o tipo do evento b for INTERSECCAO
	if (bType == X)
		bsx = b.GetSignX(), bsy = b.GetSignY();
	else
		bsx = bsy = 0;

	// --> A ordenacao deve ocorrer inicialmente pelo sinal em x, coordenada vertical 
	// --> pelo sinal em y, pelo tipo de evento e pela diferenca no identificador

	// --> Se os sinais em x nao forem do mesmo tipo, retornar a diferenca
	if (asx != bsx)
		return asx - bsx;

	// --> Se os eventos nao estiverem na mesma horizontal, retornar diferenca
	if (a.y != b.y)
		return a.y - b.y;

	// --> Se os sinais em y nao forem do mesmo tipo, retornar a diferenca
	if (asy != bsy)
		return asy - bsy;

	// --> Se os eventos nao forem do mesmo tipo, retornar a diferenca dos tipos
	if (aType != bType)
		return aType - bType;

	// --> Retornar a diferenca entre os identificadores
	return a.id - b.id;
} // EVENT::Compare

//////////////// basic geometric primitives //////////////////

//------------------------------------------------------------------------------------------
// --> Verifica se o segmento s0 possui coeficiente angular maior que s1
local bool CheckSlope(const SEGM2 & s0, const SEGM2 & s1)
{
	pbDINT det =
		(pbDINT)(s0.r->g.y - s0.l->g.y) * (s1.r->g.x - s1.l->g.x) -
		(pbDINT)(s0.r->g.x - s0.l->g.x) * (s1.r->g.y - s1.l->g.y);
	return (det > 0);
} // CheckSlope

//------------------------------------------------------------------------------------------
// --> Verfifica se o segmento a esta acima de b na lista ativa principal, posicao da linha
// --> de varredura (sweep line) e a.l->g.x
local bool IsAbove(const SEGM2 & a, const SEGM2 & b)
{
	// --> Coordenada x do ponto mais a esquerda de b
	pbINT x0 = b.l->g.x;

	// --> A linha de varredura nao pode estar a esquerda de b
	assert(a.l->g.x >= x0);

	// --> O segmento b nao e vertical
	if (pbDINT yden = b.r->g.x - x0) {
		// --> O segmento b deve estar orientado da esquerda para direita
		assert(yden > 0);

		// --> Compara os coeficientes angulares dos segmentos (b.l, b.r) e (a.l, b.l)
		pbINT y0 = b.l->g.y;
		pbDINT ynom = yden * y0 + (pbDINT)(b.r->g.y - y0) * (a.l->g.x - x0);
		pbDINT sign = yden * a.l->g.y - ynom;

		// --> Se o coeficiente angular de (b.l, b.r) for maior
		if (sign > 0)
			return true;

		// --> Se o coeficiente angular de (a.l, b.l) for maior
		if (sign < 0)
			return false;
		
		// --> Os segmentos (b.l, b.r) e (a.l, b.l) sao colineares
		return CheckSlope(a, b);
	}
	
	// --> O segmento e vertical, entao verifique se b e mais alto
	return (a.l->g.y >= b.r->g.y);
} // IsAbove

//------------------------------------------------------------------------------------------
// --> Arredonda a divisao de a por b e armazena o resultado em nDiv
local void RoundTo(pbDINT a, pbDINT b, pbINT * nDiv, int * nSgn)
{
	pbDINT m = a % b;
	if (a >= 0)
	{
		if (2 * m >= b)
			*nDiv = (pbINT)(a / b) + 1, *nSgn = -1;
		else
			*nDiv = (pbINT)(a / b) + 0, *nSgn = (m == 0) ? 0 : +1;
	}
	else
	{
		if (-2 * m > b)
			*nDiv = (pbINT)(a / b) - 1, *nSgn = +1;
		else
			*nDiv = (pbINT)(a / b) + 0, *nSgn = (m == 0) ? 0 : -1;
	}
} // RoundTo

// -----------------------------------------------------------------------------------------
// --> Compara de forma exata nom / den com x. O valor retornado deve ser
// -->   0    se nom/den == x
// -->   < 0  se nom/den <  x
// -->   > 0  se nom/den >  x
local int SgnCmp(pbDINT nom, pbDINT den, pbINT x)
{
	// --> O denominador deve ser positivo
	assert(den > 0);

	pbDINT den_x = den * x;
	if (nom < den_x)
		return -1;
	if (nom > den_x)
		return +1;
	return 0;
} // SgnCmp

// -----------------------------------------------------------------------------------------
// --> Retorna se o ponto (xnom / xden, ynom / yden) e lexicograficamente menor que (x, y)
local bool LexLs(pbDINT xnom, pbDINT xden, pbDINT ynom, pbDINT yden,
			 pbINT x, pbINT y)
{
	int cmp = SgnCmp(xnom, xden, x);
	if (cmp > 0)
		return false;
	if (cmp < 0)
		return true;
	return (SgnCmp(ynom, yden, y) < 0);
} // LexLs

// -----------------------------------------------------------------------------------------
// --> Retorna se o ponto (xnom / xden, ynom / yden) e lexicograficamente maior que (x, y)
local
bool LexGt(pbDINT xnom, pbDINT xden, pbDINT ynom, pbDINT yden,
			 pbINT x, pbINT y)
{
	int cmp = SgnCmp(xnom, xden, x);
	if (cmp > 0)
		return true;
	if (cmp < 0)
		return false;
	return (SgnCmp(ynom, yden, y) > 0);
} // LexGt

#ifndef NDEBUG
// -----------------------------------------------------------------------------------------
// --> Rotina utilizada para verificar precodincoes
local bool IntLess(pbINT xa, pbINT ya, pbINT xb, pbINT yb)
{
	return (xa < xb or xa == xb and ya < yb);
} // IntLess
#endif // NDEBUG

// ------------------------------------------------------------------------------------------
// --> Determina o ponto de interseccao entre os segmentos (a, b) e (c, d), assumindo que
// --> eles nao sao paralelos. As coordenadas dos pontos de interseccao sao representadas
// --> por numeros racionais com denominadores positivos. Uma precondicao e que a < b e c < d
local bool SegmIsect(pbINT xa, pbINT ya, pbINT xb, pbINT yb,
			   pbINT xc, pbINT yc, pbINT xd, pbINT yd,
			   pbDINT * xnom, pbDINT * xden,
			   pbDINT * ynom, pbDINT * yden)
{
	assert(IntLess(xa, ya, xb, yb));
	assert(IntLess(xc, yc, xd, yd));

    pbDINT xcd = xc - xd;
    pbDINT ycd = yc - yd;

    pbDINT r = xcd * (yc - ya) - ycd * (xc - xa);

    pbINT xab = xa - xb;
    pbINT yab = ya - yb;
    pbDINT d = ycd * xab - xcd * yab;

	assert(d != 0);

	if (d > 0)
	{
		*xden = *yden = d;
		*xnom = d * xa - r * xab;
		*ynom = d * ya - r * yab;
	}
	else
	{
		*xden = *yden = d = -d;
		*xnom = d * xa + r * xab;
		*ynom = d * ya + r * yab;
	}
	return	LexLs(*xnom, *xden, *ynom, *yden, xb, yb) and
			LexLs(*xnom, *xden, *ynom, *yden, xd, yd) and
			LexGt(*xnom, *xden, *ynom, *yden, xa, ya) and
			LexGt(*xnom, *xden, *ynom, *yden, xc, yc);
} // SegmIsect

// ------------------------------------------------------------------------------------------
// --> Determina se dois segmentos interseccionam e retorna o resultado em (xDiv, yDiv)
local bool CalcIsect(	const SEGM2 & s0, const SEGM2 & s1,
				pbINT * xDiv, int * xSgn,
				pbINT * yDiv, int * ySgn)
{
	// --> Segmento s1 deve ser o proximo a s0 (imediatamente abaixo)
	assert(s0.n == &s1);
	if (not CheckSlope(s0, s1))
		return false;

	// --> Determina o ponto de interseccao (se existir) na forma racional
	pbDINT xnom, xden, ynom, yden;
	if (not SegmIsect(
		s0.l->g.x, s0.l->g.y, s0.r->g.x, s0.r->g.y,
		s1.l->g.x, s1.l->g.y, s1.r->g.x, s1.r->g.y,
		&xnom, &xden, &ynom, &yden))
		return false;

	// --> Determina a divisao arredondada (xnom / xden, ynom, yden)
	RoundTo(xnom, xden, xDiv, xSgn);
	RoundTo(ynom, yden, yDiv, ySgn);
	return true;
} // CalcIsect

// ------------------------------------------------------------------------------------------
// --> Calcula o coordenada y arredondada do ponto de interseccao do segmento b com a reta 
// --> vertical x = X2/2 e armazena em yDiv
local void Isect2X(const SEGM2 & b, pbDINT X2, pbINT * yDiv)
{
	// --> Obtem as coordenadas dos pontos iniciais e finais do segmento
	pbINT x0 = b.l->g.x;
	pbINT y0 = b.l->g.y;
	pbINT x1 = b.r->g.x;
	pbINT y1 = b.r->g.y;

	pbDINT yden = (pbDINT)(x1 - x0) * 2;
	// --> x1 deve ser maior que x0 pois x0 e a coordenada x do ponto mais a esquerda e x1 
	// --> a coordenada x do ponto mais a direita. 
	assert(yden > 0);

	pbDINT ynom = yden * y0 + (X2 - 2 * x0) * (y1 - y0);

	// --> Determina a divisao arredondada yDiv = (ynom / ydev)
	int ySgn;
	RoundTo(ynom, yden, yDiv, &ySgn);
} // IsectX

// ------------------------------------------------------------------------------------------
// --> Obtem as coordenadas y arredondadas do segmento que interceptam as arestas esquerda 
// --> (armazena em lyDiv) e direita (armazena em ryDiv) do quadrado de tolerancia. Para 
// --> segmentos que tenham seu ponto inicial em X, lyDiv recebe a coordenada j do grid. Para
// --> segmentos que tenham seu ponto final em X, ryDiv recebe a coordenada j do grid.
local void ObtainSegmY(const SEGM2 & s, pbINT X, pbINT * lyDiv, pbINT * ryDiv)
{
	// --> O ponto inicial do segmento deve estar a esquerda de X , ou no proprio X para que 
	// --> o segmento intercepte o quadrado de tolerancias
	assert(s.l->g.x <= X);
	// --> O ponto final do segmento deve estar a direita de X , ou no proprio X para que 
	// --> o segmento intercepte o quadrado de tolerancias
	assert(s.r->g.x >= X);

	if (s.l->g.x == X)
		// --> Para segmentos que possuam seu ponto inicial em X lyDiv recebe j do grid
		*lyDiv = s.l->g.y;
	else
		// --> Caso contrario calcular a coordena y do ponto em que o segmento intercepta a
		// --> aresta esquerda do quadrado de tolerancia (x = X - 1/2)
		Isect2X(s, (pbDINT)X * 2 - 1, lyDiv);

	if (s.r->g.x == X)
		// --> Para segmentos que possuam seu ponto final em X ryDiv recebe j do grid
		*ryDiv = s.r->g.y;
	else
		// --> Caso contrario calcular a coordena y do ponto em que o segmento intercepta a
		// --> aresta direita do quadrado de tolerancia (x = X + 1/2)
		Isect2X(s, (pbDINT)X * 2 + 1, ryDiv);
} // ObtainSegmY

//////////////////// class BOCTX implementation /////////////////////

// ------------------------------------------------------------------------------------------
// --> Construtor: inicializa o metodo de insercao e a fila de parametros
BOCTX::BOCTX(INS_PROC InsertProc, void * InsertParm)
{
	// --> inicializa o contador de ids
	m_nId = 0;

	// --> Inicializa o metodo de insercao
	m_InsertProc = InsertProc;
	
	// --> Inicializa a fila de parametros
	m_InsertParm = InsertParm;
} // BOCTX::BOCTX

// ------------------------------------------------------------------------------------------
void BOCTX::InsMainList(SEGM2 * s)
{
	// --> Enquanto nao encontrou um segmento acima do segmento atual (linha de varredura)
	SEGM_LIST::iterator segm = m_S.begin();
	while (segm != m_S.end() and IsAbove(*s, *segm))
		++segm;
	
	// --> Insere s abaixo de segm
	m_S.insert(segm, *s);
} // BOCTX::InsMainList

// ------------------------------------------------------------------------------------------
// --> Acrescenta o evento na lista de eventos
void BOCTX::AddEvent(EVENTLIST * list, const EVENT & e)
{
	list->push_back(e);
} // BOCTX::AddEvent

// ------------------------------------------------------------------------------------------
// --> Inicializa os eventos para START e END
void BOCTX::CollectEvents(SEGM2 * aSegms, UINT nSegms)
{
	// --> Para todos os segmentos de ambos os poligonos
	for (UINT i = 0; i < nSegms; i++)
	{
		SEGM2 * segm = aSegms + i;
		EVENT s, e;

		// --> O evento de inicio corresponde ao vertice esquerdo e o fim do vertice direito
		s.SetType(EVENT::S);
		e.SetType(EVENT::E);

		// --> Define as coordenadas dos eventos de inicio e fim do segmento
		s.x = segm->l->g.x, s.y = segm->l->g.y;
		e.x = segm->r->g.x, e.y = segm->r->g.y;

		// --> Atribui uma id unica para o evento
		s.id = m_nId++;
		e.id = m_nId++;

		// --> Cada evento e associado ao segmento de origem (ambos os campos do UNION)
		s.s.s = e.e.s = segm;

		// --> Insere os eventos na lista de eventos
		m_E.push(s);
		m_E.push(e);
	}
} /* CollectEvents */

// ------------------------------------------------------------------------------------------
// --> Verifica se dois segmentos se interseccionam
void BOCTX::CheckForCross(SEGM2 * sp, SEGM2 * sn)
{
	// --> Segmento sn deve ser o proximo em relacao ao segmento sp (imediatamente abaixo)
	assert(sp->n == sn);

	// --> Verifica se sn e sp nao sao iguais ou ultimo segmento na fila
	if (sn != sp and sn != &*m_S.end() and sp != &*m_S.end())
	{
		pbINT	xDiv, yDiv;
		int		xSgn, ySgn;

		// --> Verifica se os dois segmentos interseccionam
		if (CalcIsect(*sp, *sn, &xDiv, &xSgn, &yDiv, &ySgn))
		{
			EVENT e;

			// --> Acrescenta o evento interseccao
			e.SetType(EVENT::X);

			e.x = xDiv, e.SetSignX(xSgn);
			e.y = yDiv, e.SetSignY(ySgn);
			e.c.s0 = sp;
			e.c.s1 = sn;
			e.id = m_nId++;

			m_E.push(e);
		}
	}
} // BOCTX::CheckForCross

// ------------------------------------------------------------------------------------------
// --> Processa o evento de acordo com o seu tipo
void BOCTX::HandleEvent(EVENT & event)
{
	// Algoritmo Bentley-Ottmann
	switch (event.GetType()) {
		// --> Evento do tipo END (final de segmento)
		// --> Para evento END deve-se remover o evento da lista ativa principal e
		// --> verificar se os segmentos imediatamente acima e abaixo do evento se
		// --> interseccionam
		case EVENT::E:
			// --> E necessario o abre chaves devido as declaracoes
			{
			// --> Recupera o proximo segmento da lista de segmentos
			// --> segmento imediatamente abaixo do evento
			SEGM2 * sn = event.e.s->n;

			// --> Recupera o segmento anterior da lista de segmentos
			// --> segmento imediatamente acima do evento
			SEGM2 * sp = event.e.s->p;

			// --> Remove o segmento da lista de eventos
			m_S.erase(SEGM_LIST::iterator(event.e.s));

			// --> Verifica se os segmento imediatamente acima e abaixo se interseccionam
			// --> e adiciona o evento de interseccao na lista de eventos m_E
			CheckForCross(sp, sn);
			}	
			break;

		// --> Evento do tipo START (inicio de segmento)
		// --> Para o evento start deve-se adicionar o evento na lista ativa principal na
		// --> posicao correta (lista de segmentos ordenada de cima para baixo) e verifica
		// --> se os segmentos imediatamente acima e abaixo do evento se interseccionam
		case EVENT::S:
			// --> Insere o segmento na lista ativa principal
			InsMainList(event.s.s);

			// --> Verifica se o segmentos se intersecciona com o imeditamente abaixo
			CheckForCross(event.s.s, event.s.s->n);

			// --> Verifica se o segmentos se intersecciona com o imeditamente acima
			CheckForCross(event.s.s->p, event.s.s);
			break;

		// --> Evento tipo INSTERSECTION (interseccao de segmentos)
		// --> Para o evento intersection deve-se inverter a ordem dos segmentos que se
		// --> cruzam na lista ativa principal. Depois deve-se verificar se o segmento 
		// --> superior intersecciona com o imediatamente acima e se o segmento inferior 
		// --> intersecciona com o imediatamente abaixo.
		case EVENT::X:
			// --> E necessario o abre chaves devido as declaracoes
			{
			// --> Interseccoes com segmentos nao adjacentes devem ser ignoradas
			if	(event.c.s0->n != event.c.s1)
				return;

			// --> Cria iterators para os segmentos que criaram a interseccao
			SEGM_LIST::iterator s0(event.c.s0), s1(event.c.s1);

			// --> Os segmentos estao invertidos
			assert(s0->n == &*s1 and s1->p == &*s0);

			// --> Inverte a posicao dos segmentos s0 e s1 na lista ativa principal
			// --> Remove segmento s1 (solta segmento s0)
			m_S.erase(s1);
			m_S.insert(s0, *s1);

			// --> Verifica se o segmento superior intersecciona com o imediatamente acima
			CheckForCross(s1->p, &*s1);

			// --> Verifica se o segmento inferior intersecciona com o imediatamente abaixo
			CheckForCross(&*s0, s0->n);
		}	break;
	}
} // BOCTX::HandleEvent

// ------------------------------------------------------------------------------------------
// --> Determina as interseccoes entre os dois poligonos a partir da lista de segmentos
void BOCTX::Sweep(SEGM2 * aSegms, UINT nSegms)
{
	// --> Inicializa a lista de eventos START e END
	CollectEvents(aSegms, nSegms);
	// --> A lista de eventos m_E contem todos os eventos de START e END inicialmente.
	// --> A medida que forem detectadas interseccoes, os eventos de INTERSECCION tambem
	// --> serao armazenas em m_E.

	// --> Enquanto existir evento na lista
	while (not m_E.empty())
	{
		// --> Cria a lista para armazenamento do estado atual da lista ativa principal
		// --> A save_list armazena o estado inicial da lista ativa principal m_S(antes da 
		// --> realizacao do passo 1)
		SAVE_LIST save_list;

		// --> Reserva antecipadamente 32 posicoes
		save_list.reserve(32);
		
		// --> Transfere o estado atual o line sweep para a lista salva
		for (SEGM_LIST::iterator segm = m_S.begin(); segm != m_S.end(); ++segm)
			save_list.push_back(&*segm);

		// --> Executa o primeiro passo
		// --> O primeiro passo consiste na aplicacao do algoritmo de Bentley-Ottmann para
		// --> encontrar as interseccoes dos segmentos

		// --> Retira o primeiro evento da pilha e armazena
		EVENT e = m_E.top(); 
		m_E.pop();

		// --> Cria a lista de eventos
		// --> A elist armazena os eventos em que a coordenada x do pontos inicial, 
		// --> final e interseccao sao aproximados para i (coordenada x inteira do grid).
		// --> A elist e necessaria para a realizacao do passo 2 .
		EVENTLIST elist;

		// --> Reserva antecipadamente 8 posicoes
		elist.reserve(8);

		assert(INT20_MIN <= e.x and e.x <= INT20_MAX);

		// --> Acrescenta o evento na lista de eventos
		AddEvent(&elist, e);

		// --> Processa evento de acordo com o algoritmo de Bentley-Ottman
		HandleEvent(e);

		// --> Para todos os eventos remanescentes localizados na mesma vertical
		while (not m_E.empty() and m_E.top().x == e.x) {
			// --> Retira o primeiro evento da pilha e armazena
			e = m_E.top(); m_E.pop();

			// --> Acrescenta o evento na lista de eventos
			AddEvent(&elist, e);

			// --> Processa evento
			HandleEvent(e);
		}

		// --> Executa o segundo passo
		Pass2(&elist, &save_list);
	}
} // Sweep

// ------------------------------------------------------------------------------------------
// --> Insere um no no segmento dado a coordenada x e o quadrado de tolerancia. 
void BOCTX::InsNewNode(SEGM2 * s, TSEL * tsel, pbINT X)
{
	// --> A coordenada y do no a ser inserido deve a ser a mesma do quadrado de tolerancia 
	// --> que intersecciona o segmento.
	pbINT Y = tsel->y;
	VNODE2 * vn;
	// --> O no a ser inserido coincide com o vertice esquerdo do segmento.
	if		(s->l->g.x == X and s->l->g.y == Y)
		vn = s->l;
	// --> O no a ser inserido coincide com o vertice direito do segmento.
	else if	(s->r->g.x == X and s->r->g.y == Y)
		vn = s->r;
	// --> Caso contrario deve-se criar um novo no.
	else
	{
		// --> Obtem o no inicial do segmento de acordo com a orientacao do contorno
		VNODE2 * after = (s->m_bRight) ? s->r->prev : s->r;
		assert(after->g.x != X or after->g.y != Y);

		// --> Cria  um novo no com coordenadas (X,Y)
		vn = new VNODE2;
		if (vn == NULL)
			error(err_no_memory);
		vn->Flags = s->l->Flags;
		vn->g.x = X;
		vn->g.y = Y;
		vn->NFPshared = false;
		// TEST: Marcar no como intersecao
		vn->intersection = false;

		// --> Inclui o no vn no contorno apos o no after (entre o no inicial e final do 
		// --> segmento)
		vn->Incl(after);
	}
	// --> Chama o procedimento para criacao e montagem dos descritores e connective list
	// --> para o novo no
	m_InsertProc(&tsel->list, vn, m_InsertParm);
} // BOCTX::InsNewNode

// ------------------------------------------------------------------------------------------
// --> Encontra a interseccao do segmento com o quadrado de tolerancia e o insere
void BOCTX::Intercept(LIST_TSEL * tsel, SEGM2 * segm, pbINT cx)
{
	// --> Lista de quadrados de tolerancia nao esta vazia
	assert(not tsel->empty());
	pbINT ly, ry;

	// --> ly recebe a coordenada y que o segmento intercepta a aresta esquerda do quadrado
	// --> de tolerancia e ry recebe a coordenada y que o segmento intercepta a aresta 
	// --> direita do quadrado de tolerancia
	ObtainSegmY(*segm, cx, &ly, &ry);

	// --> Realiza o snap rounding para o segmento: para cada quadrado de tolerancia que
	// --> entre ry e ly insere um no no segmento.

	// --> Para segmentos com inclinacao negativa
	if		(ly > ry)
	{
		// --> Faz a busca no sentido inverso (do maior y para o menor)
		LIST_TSEL::reverse_iterator ts = tsel->rbegin();		
		// --> Busca o primeiro quadrado de tolerancia abaixo de ly
		for (;;)
		{
			// --> Encontrou o quadrado desejado
			if (ts->y <= ly)
				break;
			// --> Nao ha quadrados de tolerancia abaixo de ly
			if (++ts == tsel->rend())
				return;
		} 
		// --> Para cada interseccao com quadrado de tolerancia entre ly e ry insere um no
		while (ts->y >= ry and ts->y <= ly)
		{
			InsNewNode(segm, &*ts, cx);
			if (++ts == tsel->rend())
				return;
		}
	}

	// --> Para segmentos com inclinacao positiva
	else if (ly < ry)
	{
		// --> Faz a busca na sentido direto (do menor y para o maior)
		LIST_TSEL::iterator ts = tsel->begin();
		// --> Busca o primeiro quadrado de tolerancia acima de ly
		for (;;)
		{
			// --> Encontrou o quadrado desejado
			if (ts->y >= ly)
				break;
			// --> Nao ha quadrados de tolerancia acima de ly
			if (++ts == tsel->end())
				return;
		}
		// --> Para cada interseccao com quadrado de tolerancia entre ly e ry insere um no
		while (ts->y >= ly and ts->y <= ry)
		{
			InsNewNode(segm, &*ts, cx);
			if (++ts == tsel->end())
				return;
		}
	}
	// --> Para segmentos horizontais
	else
	{
		// --> Faz a busca na sentido direto (do menor y para o maior)
		LIST_TSEL::iterator ts = tsel->begin();
		// --> Busca o primeiro quadrado de tolerancia acima de ly ou em ly
		for (;;)
		{
			// --> Encontrou o quadrado desejado
			if (ts->y >= ly)
				break;
			// --> Nao ha quadrados de tolerancia acima de ly ou em ly
			if (++ts == tsel->end())
				return;
		}
		// --> Se o quadrado de tolerancia estiver em ly temos uma interseccao. Insere um no
		if (ts->y == ly)
			InsNewNode(segm, &*ts, cx);
	}
} // BOCTX::Intercept

// ------------------------------------------------------------------------------------------
// --> Executa o segundo passo
void BOCTX::Pass2(EVENTLIST * elist, SAVE_LIST * save_list)
{
	assert(not elist->empty());

	// --> Cria quadrados de tolerancia para cada evento
	LIST_TSEL tsel;
	for (EVENTLIST::const_iterator e = elist->begin(); e != elist->end(); ++e)
	{
		TSEL e0 = { e->y, NULL };
		tsel.push_back(e0);
	}
	
	// --> Ordena os quadrados verticalmente (em ordem y crescente) e remove os repetidos
	tsel.sort();
	tsel.unique();
	
	// --> Intersecciona os quadrados de tolerancia com cada segmento ativo do estado inicial
	// --> da lista principal (antes do passo 1). Ou seja, verfica a interseccao dos 
	// --> segmentos que cruzam, passam ou terminam em i (posicao x atual do grid da linha de
	// --> varredura)
	pbINT elistx = elist->front().x;
	for (SAVE_LIST::iterator segm = save_list->begin(); segm != save_list->end(); ++segm)
		Intercept(&tsel, *segm, elistx);

	// --> Restou apenas verificar as interseccoes dos segmentos que tem seu ponto inicial
	// --> em i.
	for (EVENTLIST::iterator e = elist->begin(); e != elist->end(); ++e)
	{
		if (e->GetType() == EVENT::S)
			Intercept(&tsel, e->s.s, elistx);
	}
} // BOCTX::Pass2

} // namespace POLYBOOLEAN