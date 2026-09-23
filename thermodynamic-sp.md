# User
Ho bisogno di capire come i lavori sul semianello termodinamico di Consani, Marcolli e Connes possono collegarsi al problema del K-SAT.
Io penso infatti che partendo dal lavoro di Parisi su backtracking survey propagation sia possibile capire il salto che bisogna fare dalla formulazione di ricerca dinamica con backtracking basata su una metrica dura per cui si usa polarizzazione o certainty, ad un metrica più soft proprio come si fa ripesando gli stati da uniformi nel microcanonico a gibbs nel canonico.
Identifica i lavori di questi matematici e fisici che possono permettermi di effettuare il salto concettuale e quindi proporre delle nuove euristiche che riescano a migliorare le performance dei solver basati sulla nuova teoria che possiamo sviluppare insieme.
É una bella sfida intellettuale, iniziamo!

# Assistant
Sì. Qui c’è un ponte teorico molto concreto, e secondo me vale la pena costruirlo partendo dalla struttura matematica dei lavori, non da una semplice analogia terminologica.

Una piccola correzione bibliografica è importante: il lavoro intitolato propriamente Thermodynamic Semirings è di Matilde Marcolli e Ryan Thorngren. È però costruito esplicitamente sulla costruzione precedente di Alain Connes e Caterina Consani, dove l’entropia di Shannon emerge nella caratteristica 1 e nella versione “tropicale” della costruzione di Witt.  

Il filo che ci interessa è quindi

\text{Connes--Consani}
\;\longrightarrow\;
\text{Marcolli--Thorngren}
\;\longrightarrow\;
\text{SP/1RSB}
\;\longrightarrow\;
\text{BSP}
\;\longrightarrow\;
\text{soft control of search}.

E, cosa ancora più interessante, esiste già un pezzo intermedio che rende SP stesso un oggetto di meccanica statistica locale: Braunstein e Zecchina mostrano che le equazioni SP possono essere reinterpretate come equazioni BP su uno spazio esteso in cui ogni variabile può assumere anche lo stato *, con una funzione energia locale la cui entropia coincide con la complessità prevista da SP.  

1. Il vero salto matematico: da tropicale a termodinamico

Il punto centrale di Marcolli–Thorngren è estremamente semplice.

Nel semianello tropicale min-plus,

x\oplus y=\min(x,y).

Questa è un’operazione “hard”: tra due alternative sopravvive soltanto quella migliore.

La deformazione termodinamica sostituisce questa operazione con

x\oplus_{S,T}y
=
\min_{p\in[0,1]}
\left[
px+(1-p)y-T S(p)
\right].

Per l’entropia di Shannon si ottiene esattamente

x\oplus_{\mathrm{Sh},T}y
=
-T\log\left(e^{-x/T}+e^{-y/T}\right).

Quando T\to0,

x\oplus_{\mathrm{Sh},T}y\longrightarrow \min(x,y).

Quindi il tropicale è il limite di temperatura zero della struttura termodinamica. A temperatura finita non scegli più una sola alternativa: sommi le alternative con peso Gibbs. Marcolli e Thorngren interpretano infatti questa quantità come una free energy associata a un sistema a due stati, con Z=e^{-F/T}.  

Questo è esattamente il tipo di trasformazione che stai cercando per BSP.

Il ranking duro

a^\star=\arg\max_a Q(a)

diventa

\pi_T(a)
=
\frac{e^{Q(a)/T}}
{\sum_b e^{Q(b)/T}},

con

T\to0
\quad\Rightarrow\quad
\pi_T\to\delta_{a^\star}.

Non stiamo quindi inventando semplicemente una “softmax heuristic”: stiamo sostituendo una struttura tropicale/idempotente con la sua deformazione termodinamica.

2. E qui BSP è già quasi pronto per ricevere questa deformazione

Nel BSP di Parisi, Marino e Ricci-Tersenghi, dopo il fixed point SP si calcolano le probabilità w_i^+, w_i^- e quindi un bias b_i. La decimazione sceglie le variabili con bias più alto; il backtracking sceglie quelle il cui bias si è deteriorato. L’algoritmo cerca di mantenere positiva la complessità \Sigma, cioè il logaritmo del numero di cluster residui.  

La cosa decisiva è che BSP contiene già implicitamente una funzione di utilità:

\text{“quanto è buona questa scelta per il futuro del problema?”}

ma la rappresenta attraverso una proxy locale dura, il bias.

Il passaggio che proporrei è quindi:

\boxed{
\text{bias duro}
\quad\longrightarrow\quad
\text{free-energy/value function delle azioni}
}

e poi

\boxed{
\arg\max
\quad\longrightarrow\quad
\text{Gibbs policy}.
}

3. C’è però una distinzione importante: temperatura fisica e temperatura algoritmica

Qui dobbiamo evitare una confusione che sarebbe pericolosa per la teoria.

Nel K-SAT fisico hai già una Hamiltoniana naturale,

E(\sigma)=\#\{\text{clausole violate}\},

e quindi una misura di Gibbs

P_\beta(\sigma)
\propto
e^{-\beta E(\sigma)}.

Ma nel settore SAT tutte le configurazioni soddisfacenti hanno E=0. Quindi questa Gibbs measure, portata semplicemente a \beta\to\infty, non distingue i cluster soddisfacenti.

Il problema che interessa a noi è invece la struttura interna del paesaggio dei cluster.

Qui entra la 1RSB. La costruzione naturale è

\mathcal Z(m)
=
\sum_{\alpha}
Z_\alpha^m,

dove \alpha indicizza i cluster e

Z_\alpha
=
\sum_{\sigma\in C_\alpha}
e^{-\beta E(\sigma)}.

Nel limite SAT e a temperatura fisica zero,

Z_\alpha=|C_\alpha|,

per cui

\mathcal Z(m)
=
\sum_\alpha e^{m s_\alpha N},

con s_\alpha entropia interna del cluster.

La misura sui cluster diventa quindi

\mu_m(\alpha)
\propto
e^{mNs_\alpha}.

Per m=0 i cluster sono pesati uniformemente; aumentando m si introducono pesi Gibbsiani verso cluster con maggiore entropia interna. SP è precisamente collegata alla formulazione 1RSB nel valore m=0, mentre la letteratura BSP osserva esplicitamente che una misura opportunamente biasata verso cluster non congelati potrebbe ridurre drasticamente il backtracking necessario.  

Questa, secondo me, è la formulazione molto più pulita della tua intuizione “microcanonico \rightarrow canonico”.

Non farei:

\text{uniform solution space}
\rightarrow
\text{Gibbs solution space}.

Farei piuttosto:

\boxed{
\text{uniform cluster ensemble }(m=0)
\rightarrow
\text{reweighted cluster ensemble }(m>0)
}

e separatamente:

\boxed{
\text{hard decision }(T=0)
\rightarrow
\text{Gibbs policy }(T>0).
}

Sono due deformazioni diverse.

4. Il punto più promettente: il soft Bellman diventa una partition-function recursion

Qui il collegamento con Kappen e con il soft-control diventa sorprendentemente naturale.

Consideriamo uno stato del solver

u=(\text{partial assignment}),

e l’insieme delle azioni

A(u)
=
\{\text{decimate }i=+1,\,
\text{decimate }i=-1,\,
\text{backtrack }i,\ldots\}.

Supponiamo di avere un valore futuro V(u). In una formulazione hard:

V(u)
=
\max_{a\in A(u)}
\left[
r(u,a)+V(u_a)
\right].

La versione soft è

\boxed{
V_T(u)
=
T\log
\sum_{a\in A(u)}
\exp\left(
\frac{r(u,a)+V_T(u_a)}{T}
\right).
}

Questa non è soltanto l’equazione di Bellman “con una softmax”.

Definendo

Z_T(u)=e^{V_T(u)/T},

si ottiene

\boxed{
Z_T(u)
=
\sum_{a\in A(u)}
e^{r(u,a)/T}Z_T(u_a).
}

Questa è una vera e propria partition-function recursion sul search tree.

Ed è esattamente la logica che rende potente il formalismo dei semianelli termodinamici: nel limite tropicale hai il massimo/minimo; a temperatura finita hai la somma pesata Gibbsiana. Marcolli e Thorngren formulano persino l’operazione n-aria direttamente sul simplesso delle probabilità,

x_1\oplus_S\cdots\oplus_S x_n
=
\min_{\sum_i p_i=1}
\left(
\sum_i p_i x_i
-
TS_n(p)
\right),

e interpretano questa struttura come un mixing termodinamico a n stati.  

Qui vedo una corrispondenza quasi esatta con il search tree di BSP.

5. Il ruolo della complessità \Sigma

La prima scelta naturale sarebbe

r(u,a)
=
\Sigma(u_a)-\Sigma(u).

Ma c’è una sottigliezza importante: questo reward telescopa,

\sum_t \Delta\Sigma_t
=
\Sigma(u_T)-\Sigma(u_0).

Quindi, preso da solo, non dà una vera preferenza dinamica per la traiettoria.

Per rendere il problema di controllo non banale possiamo introdurre una quantità di sicurezza lungo il percorso, ad esempio

r_t
=
\lambda
\frac{\Sigma(u_t)}{N_t}
-
c_{\rm bt}\,\mathbf 1_{\rm backtrack}

oppure una funzione barriera della complessità residua,

r_t = B(\Sigma(u_t)).

A quel punto non stiamo più semplicemente scegliendo il ramo che preserva più cluster ora; stiamo valutando una traiettoria di ricerca.

Ed è precisamente il punto in cui la tua idea del “seguire quasi adiabaticamente il profilo a complessità più alta” può diventare una teoria di controllo vera e propria.

6. Il secondo salto: non pesare soltanto la complessità, ma il tipo di cluster

Questo è probabilmente il punto scientificamente più interessante.

Marino–Parisi–Ricci-Tersenghi notano che il problema vicino alla rigidity threshold è legato alla presenza di cluster frozen e formulano esplicitamente l’idea che una misura concentrata sui cluster unfrozen sarebbe algoritmicamente desiderabile. La difficoltà è che contarli direttamente non è facile.  

Questo suggerisce di introdurre una famiglia molto più generale:

\mu_{m,\gamma}(C)
\propto
\exp
\left[
N\left(
m s_C+\gamma q_C
\right)
\right],

dove

s_C
=
\frac{1}{N}\log |C|

è l’entropia interna e q_C può essere una misura di flessibilità/unfrozenness del cluster.

Il parametro m controlla il bias verso cluster grandi; \gamma verso cluster strutturalmente più flessibili.

Questo sarebbe già qualcosa di più interessante del semplice “soft BSP”.

Avremmo una famiglia:

\boxed{
\text{BSP}
\subset
\text{cluster-reweighted BSP}
\subset
\text{thermodynamic BSP}.
}

Nel primo caso

T=0,\quad m=0,\quad\gamma=0.

Nel secondo introduci m>0.

Nel terzo introduci anche una temperatura algoritmica T>0 sulla scelta delle azioni.

7. Perché il lavoro di Marcolli–Tedeschi è particolarmente interessante per noi

Questo è forse il paper meno ovvio da leggere e, paradossalmente, uno dei più utili.

Entropy algebras and Birkhoff factorization porta le deformazioni termodinamiche dentro strutture Rota–Baxter e le applica esplicitamente a problemi di computazione, Markov random fields e potenziali nearest-neighbor. Gli autori mostrano anche che, sotto opportune condizioni, la deformazione termodinamica preserva la struttura di campo aleatorio di Markov.  

Questo è interessante perché il nostro solver è già un sistema locale su un factor graph.

Quindi il paradigma potrebbe essere:

\text{hard local potential}
\rightarrow
\text{thermodynamic deformation}
\rightarrow
\text{soft local potential}
\rightarrow
\text{message passing}.

Non sarebbe quindi necessario costruire prima un enorme modello globale del search tree: possiamo cercare una deformazione locale delle quantità cavity/SP che conservi la struttura di message passing.

8. E c’è un altro dettaglio molto bello: l’entropy operad

Marcolli e Thorngren non si limitano all’operazione binaria. Mostrano che la struttura termodinamica può essere organizzata in termini di alberi di guessing e operadi di entropia. Le diverse parentesizzazioni corrispondono a diversi modi di interrogare una variabile o combinare informazione.  

Per K-SAT questo è quasi troppo pertinente.

Un percorso di decimazione/backtracking è un albero decisionale.

Quindi possiamo vedere il solver come

\text{search tree}
\longleftrightarrow
\text{guessing tree}
\longleftrightarrow
\text{entropy algebra}.

In questa interpretazione, il passaggio da una procedura deterministica a una procedura termodinamica significa semplicemente cambiare la legge con cui i rami dell’albero vengono aggregati.

9. Dove metterei Connes e Consani nella teoria

Il lavoro di Connes–Consani Characteristic one, entropy and the absolute point è il fondamento concettuale: caratteristica 1, analisi idempotente/tropicale e comparsa dell’entropia nella deformazione della costruzione di Witt.  

Il lavoro di Connes The Witt construction in characteristic one and Quantization rende particolarmente esplicito il rapporto fra entropy, semiring tropicale e “dequantization”.  

From monoids to hyperstructures è più laterale per il nostro scopo, ma è interessante perché introduce strutture convex/hyperfield collegate al semifield tropicale \mathbb R_+^{\max}. Potrebbe diventare rilevante qualora volessimo rappresentare una scelta non più come numero scalare ma come insieme strutturato di possibilità.  

Il lavoro Connes–Consani–Marcolli sulla termodinamica degli endomotivi è invece molto meno direttamente utile al solver K-SAT: è importante per il quadro generale di sistemi statistici quantistici/noncommutativi, ma non è da lì che prenderei la nuova euristica.  

10. La mappa concettuale che userei

La vedrei così:

\boxed{
\begin{array}{ccc}
\text{Tropical / min-plus}
&
\xrightarrow{\;T>0\;}
&
\text{Thermodynamic semiring}
\\[2mm]
\min
&
\longrightarrow
&
-T\log\sum e^{-E/T}
\\[3mm]
\downarrow
&&
\downarrow
\\[3mm]
\text{hard Bellman}
&
\xrightarrow{\;T>0\;}
&
\text{soft Bellman}
\\[2mm]
\arg\max_a Q(u,a)
&
\longrightarrow
&
\pi_T(a|u)\propto e^{Q(u,a)/T}
\\[3mm]
\downarrow
&&
\downarrow
\\[3mm]
\text{SP }(m=0)
&
\xrightarrow{\;m>0\;}
&
\text{cluster-reweighted SP}
\\[2mm]
\text{uniform clusters}
&
\longrightarrow
&
e^{mNs_C}\text{-weighted clusters}
\\[3mm]
&&\downarrow
\\
&&
\text{BSP with flexible/unfrozen bias}
\end{array}
}

E Kappen aggiunge un ultimo tassello: passando dalla value function alla “desirability”

Z=e^{V/T},

l’equazione di controllo diventa una ricorrenza di tipo partition function/path integral. Kappen mostra precisamente come trasformazioni di questo tipo possano linearizzare certe HJB nonlineari e collegarle a somme sui percorsi.  

11. Le prime euristiche che testerei

Non partirei da una nuova teoria enorme. Farei tre esperimenti incrementali.

Thermal-BSP.

Calcoliamo per ogni azione candidata una stima

Q_{i,\sigma}(u)

della qualità futura ottenuta imponendo temporaneamente x_i=\sigma, e scegliamo

\pi_T(i,\sigma|u)
\propto
e^{Q_{i,\sigma}(u)/T}.

Per T\to0 recuperiamo una decimazione hard; aumentando T introduciamo esplorazione controllata.

State-dependent backtracking.

Invece di fissare globalmente r, rendiamo il rapporto backtrack/decimation funzione dello stato:

P(\text{backtrack}\mid u)
=
\frac{
\sum_{a\in A_{\rm bt}} e^{Q(u,a)/T}
}{
\sum_{a\in A(u)}e^{Q(u,a)/T}
}.

Questo eliminerebbe il fatto che BSP utilizza un parametro r globale anche quando il regime locale della formula cambia. Il paper originale mostra infatti che r è semplicemente il rapporto fra mosse di backtracking e decimazione e che la scelta fra le due è regolata stochasticamente da questo parametro.  

Cluster-thermodynamic BSP.

Cambiamo la misura di SP da cluster uniformi a

\mu_m(C)\propto e^{mNs_C},

e calcoliamo i nuovi w_i^\pm(m). Il bias diventa quindi

b_i(m)=
\left|w_i^+(m)-w_i^-(m)\right|

oppure, ancora meglio, un valore che incorpori direttamente la variazione di free entropy sotto x_i=\pm1.

Questo è probabilmente il test teorico più importante perché va direttamente al problema che Marino–Parisi–Ricci-Tersenghi indicano: la misura usata da BSP potrebbe non essere quella giusta quando le soluzioni unfrozen diventano rare.  

12. La mia ipotesi di lavoro

Non formulerei ancora la tesi come “il semianello termodinamico migliorerà BSP”. Sarebbe troppo forte.

La formulerei così:

BSP può essere interpretato come il limite zero-temperatura di un problema di controllo discreto sul grafo dei partial assignments, mentre la struttura dei thermodynamic semirings fornisce una deformazione naturale dell’aggregazione tropicale dei rami di ricerca in una Gibbsian aggregation. La teoria 1RSB fornisce inoltre un secondo parametro m con cui deformare la misura sui cluster.

Questa formulazione è molto più interessante perché genera una vera famiglia di solver:

\boxed{
\mathrm{BSP}_{T,m,\gamma}
}

con tre assi concettualmente distinti:

T
=
\text{temperatura della ricerca},

m
=
\text{reweighting dei cluster},

\gamma
=
\text{bias verso struttura unfrozen/flessibile}.

Il limite

(T,m,\gamma)=(0,0,0)

deve ridurre al BSP standard.

Questo ci dà finalmente qualcosa che possiamo falsificare sperimentalmente: non solo “una nuova euristica”, ma una deformazione continua di BSP in cui possiamo studiare come cambiano \Sigma_{\rm res}, la distribuzione dei backtrack, la frazione di cluster unfrozen e soprattutto \alpha_a.

La prossima cosa che farei è molto concreta: derivare matematicamente, partendo dalle equazioni SP di Parisi, una prima versione di Q(u,a) e mostrare esattamente dove inserire T nel sistema di messaggi. Quello è il punto in cui la nostra idea smette di essere una suggestione e diventa una nuova equazione BP/SP da implementare.

# User
Esattamente penso che la priorità ora sia capire come scrivere le equazioni di SP deformate. Usiamo il formalismo di Marcolli e Thorngreen poi forniamo tutte le analogie e i limiti che possiamo testare.

# Assistant
Sì. A questo punto conviene fare una cosa molto precisa: non inventare una “soft-SP” mettendo semplicemente una softmax sopra i bias \eta, perché quella sarebbe una modifica euristica e perderemmo il legame matematico con Marcolli–Thorngren.

La deformazione deve essere inserita nel punto in cui SP aggrega le configurazioni di warning. È lì che compare naturalmente l’operazione termodinamica.

1. Punto di partenza: SP come aggregazione hard

Per un arco a\to i, prendiamo

u_{a\to i}\in\{-1,0,+1\},

dove u=0 significa assenza di warning e u=\pm1 un warning verso il valore corrispondente.

Per un nodo variabile j, togliendo il vincolo a, il campo cavità è

h_{j\to a}
=
\sum_{b\in\partial j\setminus a}u_{b\to j}.

Le survey entranti definiscono una misura

Q_{b\to j}(u)
=
(1-\eta_{b\to j})\,\delta_{u,0}
+
\eta_{b\to j}\,\delta_{u,J_{bj}},

dove J_{bj} è il valore di x_j che soddisfa la clausola b.

SP standard costruisce quindi la distribuzione dei campi h, elimina le configurazioni incompatibili e mantiene soltanto quelle di energia cavitaria minima. Nella forma usuale questo porta alle quantità \Pi^u,\Pi^s,\Pi^0 e a

\eta_{a\to i}
=
\prod_{j\in\partial a\setminus i}
\frac{\Pi^u_{j\to a}}
{\Pi^u_{j\to a}+\Pi^s_{j\to a}+\Pi^0_{j\to a}}.

Questa è precisamente la struttura standard delle equazioni SP.  

Il punto fondamentale è che SP tratta le configurazioni di warning in maniera “hard”: quelle che introducono una contraddizione vengono escluse nel limite di energia zero.

⸻

2. Dove entra Marcolli–Thorngren

Marcolli e Thorngren definiscono

x\oplus_{S,T}y
=
\min_{p\in[0,1]}
\left[
px+(1-p)y-TS(p)
\right].

Per Shannon,

x\oplus_{\mathrm{Sh},T}y
=
-T\log
\left(
e^{-x/T}+e^{-y/T}
\right),

cioè esattamente una free energy di tipo log-sum-exp. Nel limite

T\rightarrow0

si recupera

x\oplus_{\mathrm{Sh},0}y=\min(x,y).

Gli autori mostrano inoltre la generalizzazione n-aria

x_1\oplus_{S,T}\cdots\oplus_{S,T}x_n
=
\min_{\mathbf p\in\Delta_{n-1}}
\left[
\sum_i p_i x_i
-
T S_n(\mathbf p)
\right].

Questa è esattamente la struttura che serve quando dobbiamo aggregare molte configurazioni di warning alternative.  

E c’è una cosa ancora più importante: Marcolli–Thorngren interpretano questa operazione come una free energy e come una partition function.  

⸻

3. La deformazione corretta di SP

Fissiamo j\to a e consideriamo una configurazione

\omega=(u_b)_{b\in\partial j\setminus a}

dei warning entranti.

Definiamo

p(\omega)=\#\{b:u_b\text{ spinge }x_j\text{ a soddisfare }a\},

q(\omega)=\#\{b:u_b\text{ spinge }x_j\text{ a non soddisfare }a\}.

La quantità

h=p-q.

La parte cruciale è l’energia di conflitto:

\boxed{
\Delta E(\omega)
=
\min(p,q)
=
\frac{p+q-|p-q|}{2}.
}

È il numero di coppie di warning opposte che entrano in conflitto. Nella letteratura SP a energia finita compare la stessa quantità, a volte senza il fattore 1/2, cioè contando il conflitto due volte; il fattore viene assorbito nella ridefinizione di y.  

Ora la misura a priori sulla configurazione è

P_0(\omega)
=
\prod_{b\in\partial j\setminus a}
Q_{b\to j}(u_b).

Introduciamo quindi la free energy termodinamica associata a un particolare valore del campo h:

\boxed{
F^{(T)}_{j\to a}(h)
=
-T\log
\sum_{\omega:\,h(\omega)=h}
P_0(\omega)
e^{-\Delta E(\omega)/T}.
}

Questa è la nostra prima equazione fondamentale.

Ed è direttamente interpretabile nel formalismo di Marcolli–Thorngren come

\boxed{
F^{(T)}_{j\to a}(h)
=
\bigoplus_{\omega:\,h(\omega)=h}^{\mathrm{Sh},T}
\left[
\Delta E(\omega)-T\log P_0(\omega)
\right].
}

In altre parole, abbiamo trasformato il “min” sulle configurazioni ammissibili in una thermodynamic sum.

La forma variazionale è

F^{(T)}_{j\to a}(h)
=
\min_{\rho\in\Delta(\Omega_h)}
\left[
\sum_\omega\rho_\omega\Delta E(\omega)
+
T\,D_{\mathrm{KL}}(\rho\Vert P_0)
\right].

Questa formula è particolarmente bella: la soluzione dura minimizza soltanto l’energia, mentre la soluzione termodinamica minimizza energia più una penalità informazionale rispetto alla misura cavitaria di riferimento. Marcolli–Thorngren discutono esplicitamente la relazione tra le loro strutture termodinamiche, KL e informazione relativa.  

⸻

4. Dalla free energy alle nuove \Pi

La probabilità termica del campo è

P^{(T)}_{j\to a}(h)
=
\frac{
e^{-F^{(T)}_{j\to a}(h)/T}
}{
\sum_{h'}
e^{-F^{(T)}_{j\to a}(h')/T}
}.

Ora possiamo definire

\widetilde{\Pi}^{u}_{j\to a}(T)
=
\sum_{h<0}P^{(T)}_{j\to a}(h),

\widetilde{\Pi}^{s}_{j\to a}(T)
=
\sum_{h>0}P^{(T)}_{j\to a}(h),

\widetilde{\Pi}^{0}_{j\to a}(T)
=
P^{(T)}_{j\to a}(0),

dove abbiamo scelto il segno di h rispetto alla clausola a.

Quindi la nuova equazione di clausola è

\boxed{
\eta^{(T)}_{a\to i}
=
\prod_{j\in\partial a\setminus i}
\frac{
\widetilde{\Pi}^{u}_{j\to a}(T)
}{
\widetilde{\Pi}^{u}_{j\to a}(T)
+
\widetilde{\Pi}^{s}_{j\to a}(T)
+
\widetilde{\Pi}^{0}_{j\to a}(T)
}.
}

Questa è già una Thermodynamic Survey Propagation equation.

E non è una semplice analogia: è la deformazione termodinamica della somma sulle configurazioni cavitarie.

⸻

5. Possiamo scriverla in forma completamente esplicita

Definiamo

C_{pq}^{j\to a}
=
[z^p w^q]
\prod_{b\in V_a^s(j)}
\left[(1-\eta_{b\to j})+\eta_{b\to j}z\right]
\prod_{b\in V_a^u(j)}
\left[(1-\eta_{b\to j})+\eta_{b\to j}w\right].

C_{pq} è la probabilità a priori di ricevere p warning “soddisfacenti” e q warning “insoddisfacenti”.

Allora

\Delta E(p,q)=\min(p,q)

e quindi

\boxed{
\widetilde{\Pi}^{u}(T)
\propto
\sum_{q>p}
C_{pq}\,
e^{-\min(p,q)/T},
}

\boxed{
\widetilde{\Pi}^{s}(T)
\propto
\sum_{p>q}
C_{pq}\,
e^{-\min(p,q)/T},
}

\boxed{
\widetilde{\Pi}^{0}(T)
\propto
\sum_{p=q}
C_{pq}\,
e^{-p/T}.
}

Con la stessa normalizzazione

Z_{j\to a}(T)
=
\widetilde{\Pi}^{u}
+
\widetilde{\Pi}^{s}
+
\widetilde{\Pi}^{0}.

Queste sono equazioni immediatamente implementabili.

E qui compare un risultato molto importante.

⸻

6. Il limite T\to0 recupera esattamente SP

Quando

T\to0,

abbiamo

e^{-\min(p,q)/T}
\rightarrow
\begin{cases}
1 & \min(p,q)=0,\\
0 & \min(p,q)>0.
\end{cases}

Quindi

\widetilde{\Pi}^{u}(0)
=
\sum_{q>0}C_{0q},

che diventa

\widetilde{\Pi}^{u}(0)
=
\left[
1-
\prod_{b\in V_a^u(j)}
(1-\eta_{b\to j})
\right]
\prod_{b\in V_a^s(j)}
(1-\eta_{b\to j}),

cioè

\boxed{\widetilde{\Pi}^{u}(0)=\Pi^u.}

Analogamente

\boxed{\widetilde{\Pi}^{s}(0)=\Pi^s}

e

\boxed{\widetilde{\Pi}^{0}(0)=\Pi^0.}

Quindi

\boxed{
\lim_{T\to0}\mathrm{ThermoSP}(T)
=
\mathrm{SP}.
}

Questo è il primo limite che dobbiamo assolutamente verificare numericamente: implementazioni indipendenti delle due formule devono convergere.

⸻

7. Il significato fisico di T>0

Qui secondo me compare il vero salto concettuale.

SP standard dice:

una configurazione di warning con conflitti è vietata.

La versione termodinamica dice:

una configurazione di warning con r conflitti è ancora possibile, ma ha peso

e^{-r/T}.

Quindi non abbiamo più

\Delta E=
\begin{cases}
0 & \text{ammissibile}\\
+\infty & \text{inammissibile},
\end{cases}

ma

\boxed{
P_T(\omega)
\propto
P_0(\omega)e^{-\Delta E(\omega)/T}.
}

Questo è esattamente il passaggio

\text{hard constraint}
\quad\rightarrow\quad
\text{soft energetic constraint}.

E la letteratura aveva già sviluppato una versione di finite-energy SP / SP(y), con il fattore e^{-y\Delta E}. Quindi con

\boxed{T=\frac1y}

la nostra costruzione deve coincidere con quella teoria nota.  

Questo è fondamentale metodologicamente: la prima deformazione non è nuova. La novità possibile sta nella sua reinterpretazione termodinamica e soprattutto in ciò che facciamo dopo con la free energy.

Esiste inoltre una precedente generalizzazione di SP a temperatura finita nel lavoro di Wemmenhove e Kappen, dove la 1RSB-Survey Propagation viene estesa a temperatura finita.  

⸻

8. Ma il punto più interessante è l’espansione a bassa temperatura

Supponiamo di avere parecchie configurazioni con la stessa energia minima:

\Delta E(\omega)=E_{\min}.

Allora

F_T
=
-T\log
\sum_\omega P_0(\omega)e^{-E_\omega/T}

ha, per T\to0, la forma

\boxed{
F_T
=
E_{\min}
-
T\log W_{\min}
+
o(T),
}

con

W_{\min}
=
\sum_{\omega:E_\omega=E_{\min}}
P_0(\omega).

Quindi, anche quando due configurazioni hanno identica energia, la deformazione termodinamica introduce un secondo criterio:

\boxed{
\text{minima energia}
\quad+\quad
\text{massima degenerazione/accessibilità entropica}.
}

Questo è esattamente il genere di informazione che manca a una metrica puramente dura.

In un solver potrebbe significare:

\text{non soltanto “quanto conflitto c'è?”}

ma anche

\text{“quante configurazioni compatibili generano questo stesso livello di conflitto?”}.

È qui che \Sigma, free entropy e termodinamica possono iniziare a interagire veramente con BSP.

⸻

9. E questo ci porta naturalmente alla complessità dei cluster

SP standard lavora sulla struttura dei cluster e fornisce una stima della complessità

\Sigma=\frac1N\log \mathcal N_{\rm clusters}.

BSP utilizza questa informazione indirettamente e corregge le precedenti decisioni mediante backtracking. Gli autori stessi osservano che una direzione naturale sarebbe costruire marginals sotto una misura diversa, concentrata sui cluster più accessibili all’algoritmo; indicano esplicitamente, in particolare, il problema dei cluster unfrozen.  

Quindi io terrei separati due parametri:

T_{\rm cav}

per la deformazione termodinamica delle configurazioni cavitarie,

e

m

per la reweighting dei cluster 1RSB.

In forma schematica:

\boxed{
\mu_{m,T}(C,\omega)
\propto
e^{mNs_C}
P_0(\omega)
e^{-\Delta E(\omega)/T}.
}

Il primo peso agisce tra cluster; il secondo all’interno del calcolo cavitario.

Questa distinzione è importante. Il finite-y SP esistente ci dà il secondo asse; la teoria 1RSB ci dà il primo.

⸻

10. La vera generalizzazione Marcolli–Thorngren

Una volta ottenuta la versione Shannon,

\oplus_{\mathrm{Sh},T},

possiamo sostituire Shannon con una famiglia S:

\boxed{
F^{(S,T)}
=
\min_{\rho}
\left[
\mathbb E_\rho[\mathcal E]
-
T S(\rho)
\right].
}

Marcolli–Thorngren costruiscono proprio thermodynamic semirings associati a Shannon, Rényi, Tsallis e KL e studiano come le loro proprietà algebriche cambiano con la scelta dell’information measure.  

Questo ci dà quindi una vera famiglia sperimentale:

\mathrm{SP}_{\mathrm{Sh},T},
\qquad
\mathrm{SP}_{\mathrm{Rényi},T,\alpha},
\qquad
\mathrm{SP}_{\mathrm{Tsallis},T,q},
\ldots

con

\mathrm{SP}_{\mathrm{Sh},0}
=
\mathrm{SP}.

Non sappiamo affatto che Rényi o Tsallis debbano funzionare meglio. Ma sono deformazioni matematicamente motivate, non parametri inventati ad hoc.

⸻

11. Il test fondamentale: quattro livelli da confrontare

Io costruirei immediatamente questa gerarchia sperimentale:

\boxed{
\mathrm{SP}
}

\downarrow\quad T>0

\boxed{
\mathrm{SP}(y)
\equiv
\mathrm{ThermoSP}_{\rm Shannon}(T=1/y)
}

\downarrow\quad\text{nuova politica di decisione}

\boxed{
\mathrm{Thermo\text{-}BSP}(T)
}

\downarrow\quad m\neq0

\boxed{
\mathrm{Thermo\text{-}BSP}(T,m)
}

La distinzione è importante perché SP(y) da sola è già conosciuta. La possibile nuova euristica comincia quando usiamo la free energy termodinamica prodotta dai messaggi come quantità di controllo del search process.

⸻

12. Il passaggio successivo: dal bias alla free-energy bias

Nel BSP originale il bias di una variabile è costruito dalle probabilità

w_i^+,\qquad w_i^-,

e determina sia quali variabili decimare sia quali rilasciare durante il backtracking.  

Io sostituirei questo con

\Phi_i^{+}(T)
=
F_i(x_i=+1;T),

\Phi_i^{-}(T)
=
F_i(x_i=-1;T),

e definirei

\boxed{
B_i^{\rm th}(T)
=
\Phi_i^{-}(T)-\Phi_i^{+}(T).
}

Ma soprattutto possiamo conservare entrambe le quantità, non soltanto la loro differenza.

La decisione può diventare

P(x_i=\sigma)
\propto
e^{-\Phi_i^\sigma/T_{\rm act}},

mentre il backtracking può dipendere da

\Delta\Phi_i
=
\Phi_i^{\rm assigned}
-
\min_{\sigma}\Phi_i^\sigma.

A quel punto il solver non dice più semplicemente

\text{“questa variabile è molto polarizzata”},

ma

\text{“questa decisione ha una certa free-energy cost rispetto alle alternative”.}

Ed è qui che il collegamento con il soft Bellman che stavamo costruendo diventa naturale.

⸻

La teoria minima che proporrei come nostro oggetto di studio

La chiamerei provvisoriamente

\boxed{\mathrm{Thermodynamic\ Survey\ Propagation}}

e la definirei attraverso tre equazioni:

\boxed{
F_{j\to a}^{T}(h)
=
-T\log
\sum_{\omega:h(\omega)=h}
P_0(\omega)e^{-\Delta E(\omega)/T}
}

\boxed{
\eta_{a\to i}^{T}
=
\prod_{j\in\partial a\setminus i}
\frac{
e^{-F_{j\to a}^{u}/T}
}{
e^{-F_{j\to a}^{u}/T}
+
e^{-F_{j\to a}^{s}/T}
+
e^{-F_{j\to a}^{0}/T}
}
}

e, per la decisione,

\boxed{
\pi_T(a|u)
=
\frac{
e^{-\Phi(u,a)/T_{\rm act}}
}{
\sum_{a'}e^{-\Phi(u,a')/T_{\rm act}}
}.
}

La prima è la deformazione Marcolli–Thorngren del cavity aggregation; la seconda restituisce SP nel limite duro; la terza è il punto nel quale possiamo iniziare a trasformare BSP da una politica basata su polarizzazione/certainty in una politica basata su free energy.

La verifica iniziale è quindi molto netta: per T\to0 dobbiamo recuperare numericamente le \eta, le \Pi, i bias e la \Sigma di SP/BSP standard; per T>0 dobbiamo confrontare non soltanto la percentuale di istanze risolte, ma soprattutto la traiettoria \Sigma(d), la frequenza e profondità dei backtrack e la distribuzione di \Delta E delle configurazioni cavitarie. BSP infatti è esplicitamente costruito per mantenere positiva la complessità mentre corregge decisioni precedenti.  

Questo ci dà già un programma matematico abbastanza rigoroso: prima deriviamo \mathrm{ThermoSP}(T) in notazione completa e dimostriamo i limiti T\to0, T\to\infty, y=1/T, poi costruiamo sopra di essa la nuova free-energy policy per BSP.