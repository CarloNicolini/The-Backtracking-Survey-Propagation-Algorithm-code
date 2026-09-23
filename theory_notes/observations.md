## L'osservazione algebrica centrale

Nel codice il bias è s_C = 1 − min(s_T, s_F) (Vertex.hpp, macro __H con #define CERT). Ma dato che s_T + s_F + s_I = 1:

```math
s_C = 1 − min(s_T, s_F) = max(s_T, s_F) + s_I
```

*La certainty è dominata da s_I*, cioè dalla frazione di cluster in cui la variabile è libera. Vicino ad α_s la frazione tipica di variabili non congelate è ≈ 0.85 (la tua Fig. 5a), quindi le variabili in cima al ranking sono quelle con s_I → 1, per cui s_T e s_F sono entrambi ~10⁻³ o meno.

Il risultato è che la scelta di *quale* variabile fissare è ottima nel senso in cui è stata progettata (conserva Σ: log b_i ≈ 0), ma la scelta del *valore* è sign(s_T − s_F) — la differenza fra due numeri minuscoli, su un punto fisso convergente solo a epsilon = 0.01. In pratica si fissano per prime proprio le variabili su cui SP non ha informazione, e su quelle il verso è quasi rumore numerico. Σ non se ne accorge (il costo istantaneo è nullo), ma l'accumulo di queste scommesse è ciò che poi il backtracking deve pagare.

## La lista, in ordine di priorità

*A. Selezione e assegnazione sono collassate in una sola quantità.* s_C risponde a "quanto costa in Σ fissare i", non a "so in che verso fissarla". Vanno separate: uno score di selezione (b_i, o una stima di ΔΣ dopo riconvergenza) e una regola di assegnazione robusta — per esempio fissare solo se |s_T − s_F| / (s_T + s_F) > θ, altrimenti saltare la variabile. Da testare subito nella forma più semplice: score = b_i · |s_T − s_F|^γ, con γ che interpola fra certainty pura (γ=0) e polarizzazione pura.

*B. Il greedy su Σ a un passo è miope.* Si massimizza il fattore istantaneo sul numero di cluster, non il tasso di perdita di Σ nei passi successivi. Una stima di risposta lineare (suscettibilità di cavità) di dΣ/d(fissaggio) è calcolabile dai messaggi senza rifare SP.

*C. Correlazioni dentro il batch.* frac = 0.00125 variabili vengono fissate insieme con marginali calcolati prima di qualunque fissaggio. Due variabili nella stessa clausola, entrambe con b alto, possono essere singolarmente innocue e congiuntamente fatali. Rimedio a costo quasi nullo: vietare la co-decimazione a distanza ≤ 2 sul grafo dei fattori.

*D. Il backtracking usa bias stantii.* surveys() chiama compute_s() solo da _size_init a _N, cioè solo sulle non fissate: una variabile fissata conserva la s_C del momento in cui fu fissata. In backtrack() si confrontano quindi grandezze calcolate su formule residue diverse — e poiché i bias crescono lungo la decimazione, si rilasciano sistematicamente le variabili fissate per prime. Può essere una feature (è lo spirito di Parisi), ma è un effetto collaterale, non un criterio.

*E. Il backtracking è schedulato, non innescato.* r = 0.9 costante. Il codice già traccia Σ, η, _last_certitude, _unit_prop — e non ne usa nessuno come segnale. Un backtracking event-driven (caduta anomala di Σ, crescita di η, valanga di unit propagation) è probabilmente più efficace di qualunque r fisso.

*F. La contraddizione è fatale.* exit(-1), nessuna analisi del conflitto. Rilasciare le variabili dell'implication graph del conflitto invece delle b globalmente più basse è l'ibrido SP↔️CDCL che nessuno ha davvero provato.

*G. Whitening come criterio online.* Il tuo stesso risultato dice che tutte le soluzioni trovate sono non congelate. Allora il criterio giusto è mantenere l'*assegnamento parziale* whitenabile: il whitening costa O(M) per sweep, una variabile fissata che diventa non-✻ è candidata al rilascio, un batch che fa crescere il core va rifiutato.

*H. Misura tiltata verso i cluster non congelati.* È la proposta finale della Discussion del tuo articolo. Implementazione concreta: riponderare ogni clausola in funzione del numero di letterali soddisfatti favorendo ≥ 2 (misure biased alla Budzynski–Ricci-Tersenghi–Semerjian). È una modifica locale dell'update clausola→variabile e concentra la misura sulle soluzioni whitenabili.

*I. Numerica.* ε = 0.01 su un ranking che discrimina |s_T − s_F| ~ 10⁻³ non è affidabile, e non c'è damping. Test da mezz'ora: quanto cambia l'ordinamento fra ε = 10⁻² e ε = 10⁻⁶?

*J. Determinismo.* Dato il punto fisso la scelta è deterministica; campionare il verso con probabilità w⁺/(w⁺+w⁻) dà diversità e si sposa con i restart (l'articolo nota che una seconda run risolve le istanze rimaste).

Tre note di codice, en passant: #define POL è scritto #define __H(a,b,c) = abs(a-b), con un = di troppo — non compila, quindi l'alternativa "polarizzazione" non è mai stata testata davvero. CERT e P_M sono la stessa identica funzione. E dopo un backtrack _m_t_m_1 = 0, quindi due backtrack consecutivi rilascerebbero zero variabili.

## L'esperimento da fare per primo

Prima di scegliere il criterio, serve una definizione operativa di errore. Io userei questa: *il primo batch di decimazione dopo il quale la formula residua diventa UNSAT.* È verificabile esattamente, non congetturalmente.

A N moderato (500 ÷ 5000), dopo ogni batch si scarica la residua (print_on_file_residual_formula() c'è già) e la si passa a CaDiCaL/Kissat con time limit. Individuato il batch fatale, si rifà il batch una variabile alla volta con check SAT e si isola la variabile colpevole. Poi si registrano s_T, s_F, s_I, b, |s_T − s_F|, grado, indice di passo, stato di whitening — con gruppo di controllo le variabili non colpevoli dello stesso batch. Girando anche a r = 0 (SID) gli errori emergono più puliti.

Una seconda misura, altrettanto importante: quante volte il backtracking ripara davvero un errore fatale (la residua torna SAT). Quella separa un problema nel criterio di fissaggio da un problema nel criterio di rilascio — e finora nessuno dei due è mai stato isolato.

Se gli errori si concentrano su s_I alto e |s_T − s_F| piccolo, l'ipotesi A è confermata e il nuovo criterio segue quasi da sé. Se invece si concentrano sulle variabili con b più basso del batch, il problema è C (correlazioni intra-batch) e la soluzione è tutt'altra.

Ho salvato l'analisi completa con i riferimenti nel progetto. La domanda su cui vorrei sentirti: *secondo la tua esperienza di run, quando BSP fallisce vicino ad α_a, fallisce più per contraddizione in unit propagation o per non convergenza di SP?* Le due cose puntano a colpevoli diversi — la prima verso C/F, la seconda verso A/H — e ti risparmierebbe metà dell'esperimento.

Sources: [Local equations describe unreasonably efficient stochastic algorithms in random K-SAT (arXiv:2504.06757 / PNAS 2025)](https://arxiv.org/abs/2504.06757) · [Biased landscapes for random CSPs (arXiv:1811.01680)](https://arxiv.org/abs/1811.01680) · [Analysing Survey Propagation Guided Decimation on Random Formulas (Hetterich, arXiv:1602.08519)](https://www.arxiv.org/abs/1602.08519) · [On belief propagation guided decimation for random k-SAT (Coja-Oghlan, arXiv:1007.1328)](https://ui.adsabs.harvard.edu/abs/2010arXiv1007.1328C/abstract) · [The backtracking survey propagation algorithm (Nat. Commun. 7:12996)](https://www.nature.com/articles/ncomms12996)