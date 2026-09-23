# Rappresentazione lineare nei LLM

## You

Scrivimi in forma di essay quali sono le evidenze sperimentali che supportano l’ipotesi della rappresentazione lineare nei large language models e chiariscimi come possa essere possibile scomporre un concetto in combinazione conica di concetti esistenti dando luogo ad una algebra degli embedding nei coni di attivazione

## ChatGPT

L’ipotesi della rappresentazione lineare nei large language models non va intesa nel senso forte che «ogni concetto è un neurone» o che il residual stream sia semplicemente uno spazio vettoriale in cui ogni significato occupa una direzione fissa. L’ipotesi più interessante è più debole e, a mio avviso, più vicina alle evidenze sperimentali: molte proprietà semantiche, sintattiche, linguistiche e fattuali del modello sono codificate in modo approssimativamente lineare nel residual stream, cosicché variazioni concettuali possono essere descritte attraverso direzioni, sottospazi e combinazioni di vettori. La difficoltà teorica comincia proprio quando si passa da una geometria lineare a una geometria conica: se le rappresentazioni non sono semplicemente punti di un sottospazio ma appartengono a regioni orientate dello spazio delle attivazioni, allora diventa naturale chiedersi se esista una vera e propria algebra dei concetti indotta dalla composizione delle loro direzioni.

### L’evidenza per la rappresentazione lineare

Una prima evidenza classica proviene dall’osservazione che proprietà astratte possono essere recuperate dalle attivazioni mediante semplici classificatori lineari. Se prendiamo il residual stream di un transformer a un certo layer e cerchiamo una direzione \(v\) tale che

\[
s(x)=v^\top h(x)
\]

discrimini una proprietà di \(x\), spesso troviamo che una quantità sorprendentemente piccola di struttura geometrica è sufficiente per separare categorie semanticamente significative. Questo non dimostra che il concetto «sia» \(v\), ma dimostra che la proprietà è leggibile linearmente dalle rappresentazioni.

L'evidenza diventa molto più forte quando non ci si limita alla decodifica passiva, ma si interviene sulla rappresentazione. Se una direzione viene identificata come associata, per esempio, a una proprietà semantica, modificare l'attivazione secondo

\[
h' = h+\alpha v
\]

può modificare sistematicamente il comportamento del modello. Questa è la logica degli activation steering experiments: una proprietà non è soltanto decodificabile; è causalmente accessibile attraverso una manipolazione geometrica relativamente semplice.

Il fenomeno è stato osservato per proprietà molto diverse: sentiment, topic, factual attributes, grammatical features, truthfulness, toxicity, linguaggio, stile e numerose altre proprietà comportamentali. La generalità è importante. Se la linearità fosse soltanto un artefatto di poche task artificiali, sarebbe poco interessante; il fatto che molte proprietà indipendenti siano accessibili attraverso direzioni nel residual stream suggerisce invece che la geometria lineare sia una componente strutturale dell'architettura appresa.

Un secondo risultato importante deriva dalla cosiddetta *linear representation hypothesis*: concetti e caratteristiche del mondo possono essere rappresentati come direzioni o sottospazi approssimativamente lineari nello spazio delle attivazioni. Una formulazione più prudente sarebbe:

\[
h \simeq h_0+\sum_i \alpha_i v_i+\epsilon .
\]

Qui \(v_i\) non devono essere interpretati necessariamente come «concetti puri». Sono piuttosto assi latenti lungo i quali l'attivazione varia sistematicamente.

Questo punto è essenziale perché conduce direttamente al problema della superposizione. Il modello possiede una dimensione interna enorme, ma deve rappresentare un numero di feature molto maggiore della dimensione del singolo residual stream. Le feature possono quindi essere sovrapposte nello stesso spazio. La sparsità delle feature e la geometria della superposizione offrono una spiegazione naturale del fatto che un singolo neurone o una singola coordinata non abbia necessariamente un'interpretazione semantica unica.

Gli esperimenti di sparse autoencoders hanno fornito un'evidenza particolarmente interessante in questa direzione. Invece di cercare un'interpretazione per ciascuna coordinata del residual stream, si cerca una trasformazione

\[
h \longrightarrow z \longrightarrow \hat h
\]

nella quale \(z\) abbia dimensionalità potenzialmente molto maggiore di \(h\), ma sia sparso. L'idea è che il residual stream sia una miscela compressa di feature più elementari:

\[
h \approx \sum_i z_i f_i.
\]

Il fatto che sia possibile estrarre feature relativamente interpretabili e sparse da rappresentazioni dense è difficile da conciliare con una visione puramente neuron-centric della rappresentazione. È invece naturale se il residual stream è concepito come uno spazio vettoriale nel quale molte feature vengono sovrapposte.

### Il caso particolarmente istruttivo delle relazioni

Un'altra evidenza importante è rappresentata dalle relazioni vettoriali osservate negli embedding e, in forma più sofisticata, nelle rappresentazioni interne dei transformer.

Nel caso degli embedding statici, il fenomeno è noto da tempo: relazioni semantiche possono manifestarsi come differenze vettoriali relativamente stabili,

\[
v_{\text{king}}-v_{\text{man}}
\simeq
v_{\text{queen}}-v_{\text{woman}}.
\]

Non bisogna però trasferire ingenuamente questa osservazione ai residual stream dei moderni LLM. Il punto teorico più interessante è un altro: una trasformazione concettuale può talvolta essere approssimata come una trasformazione affine dello spazio rappresentazionale.

Questa idea sopravvive nei transformer, ma in forma contestuale. Lo stesso token non possiede necessariamente un unico significato geometrico; il vettore \(h_l(x)\) dipende dal contesto, dal layer e dalla traiettoria computazionale precedente. La linearità, quindi, è meglio pensata come una proprietà locale o subspaziale della rappresentazione, non come una coordinatizzazione globale perfetta di tutti i concetti.

### Perché il residual stream è particolarmente adatto a questa interpretazione

L'architettura transformer introduce un'altra evidenza fondamentale: il residual stream è, per costruzione, uno spazio nel quale le trasformazioni successive vengono sommate.

In forma idealizzata,

\[
h_{l+1}=h_l+F_l(h_l).
\]

Pertanto

\[
h_L=h_0+\sum_{l=0}^{L-1}F_l(h_l).
\]

Il modello non deve ricostruire da zero la rappresentazione ad ogni layer. Può aggiungere informazione a una rappresentazione già esistente. Questa struttura rende matematicamente naturale una decomposizione additiva delle informazioni.

Si può quindi immaginare il residual stream come una sorta di *workspace vettoriale* nel quale differenti circuiti scrivono contributi parzialmente sovrapposti:

\[
h =
c+
s+
\ell+
f+
\epsilon,
\]

dove \(c\) potrebbe rappresentare una componente comune, \(s\) una componente semantica, \(\ell\) una componente linguistica, \(f\) informazione fattuale e \(\epsilon\) il residuo non spiegato.

Non è necessario che queste componenti siano ortogonali. Anzi, in generale non lo saranno.

Questa osservazione diventa particolarmente importante quando si studiano rappresentazioni dello stesso contenuto espresso in lingue differenti. Se esiste una componente semantica relativamente stabile e una componente dipendente dalla lingua, possiamo idealmente scrivere

\[
h_{l,m,p}
\simeq
c_l+\alpha_{l,m}a_m+\beta_{l,p}b_p+\epsilon_{l,m,p},
\]

dove \(a_m\) rappresenta la componente linguistica, \(b_p\) quella semantico-fattuale associata al prompt o al contenuto e \(c_l\) una componente comune dipendente dal layer. Una struttura di questo tipo è precisamente ciò che può produrre le regolarità di similarità tra rappresentazioni che gli esperimenti di probing e activation geometry osservano.

### Ma la linearità non implica una base di concetti

Qui si trova, secondo me, il passaggio concettuale decisivo.

Dire

\[
h=\sum_i\alpha_i v_i
\]

non significa necessariamente che esista una base \(\{v_i\}\) costituita da concetti semanticamente atomici.

La decomposizione può essere altamente non unica. Se

\[
v_3=v_1+v_2,
\]

allora una rappresentazione potrebbe essere scritta sia attraverso \(v_3\), sia attraverso \(v_1\) e \(v_2\). Inoltre le feature possono essere non ortogonali e fortemente correlate.

La geometria che emerge non è quindi necessariamente quella di uno spazio euclideo ordinato da assi semantici indipendenti. È più plausibile pensare a una sovrapposizione di feature con struttura ridondante, sparsa e contestuale.

Ed è proprio qui che la nozione di cono diventa interessante.

### Dal sottospazio al cono di attivazione

Uno spazio vettoriale permette combinazioni lineari con coefficienti arbitrariamente positivi o negativi:

\[
x=\sum_i \alpha_i v_i,
\qquad
\alpha_i\in\mathbb R.
\]

Un cono convesso restringe invece i coefficienti:

\[
x=\sum_i\alpha_i v_i,
\qquad
\alpha_i\geq0.
\]

Geometricamente, un sottospazio contiene una direzione e la sua opposta. Se \(v\) appartiene allo spazio, anche \(-v\) appartiene allo spazio. Un cono invece può distinguere semanticamente una direzione dalla sua negazione.

Questo è potenzialmente molto importante per i LLM.

Supponiamo che \(v_{\text{dog}}\), \(v_{\text{animal}}\) e \(v_{\text{living}}\) siano feature direzionali. Una rappresentazione potrebbe essere

\[
h=
\alpha v_{\text{animal}}
+\beta v_{\text{living}}
+\gamma v_{\text{dog}}
\]

con

\[
\alpha,\beta,\gamma\geq0.
\]

Il concetto risultante non sarebbe necessariamente un nuovo vettore primitivo. Potrebbe essere una regione dello spazio generata dalle feature precedenti.

In questo senso «dog» potrebbe non essere semanticamente atomico. Potrebbe essere una particolare configurazione di proprietà più generali:

\[
\text{dog}
\approx
\alpha_1\text{animal}
+\alpha_2\text{mammal}
+\alpha_3\text{canine}
+\alpha_4\text{social}
+\cdots.
\]

La formula non deve essere interpretata letteralmente come una definizione ontologica. È una rappresentazione geometrica di una decomposizione funzionale.

### L'idea di un'algebra degli embedding

A questo punto si può formulare l'ipotesi in modo più radicale.

Supponiamo che esista una famiglia di feature fondamentali

\[
\mathcal V=\{v_1,\ldots,v_N\}.
\]

Una rappresentazione interna potrebbe essere descritta come

\[
h=\sum_i \alpha_i v_i,
\qquad \alpha_i\geq0,
\]

almeno localmente e dopo aver fissato layer, contesto e sottospazio rilevante.

Allora un concetto non è più semplicemente un punto. È una combinazione nella *cone algebra* generata dalle feature:

\[
\mathcal C=\operatorname{cone}(v_1,\ldots,v_N).
\]

La composizione semantica diventa una operazione tra coefficienti.

Per esempio, se

\[
x=\sum_i a_i v_i
\]

e

\[
y=\sum_i b_i v_i,
\]

la composizione additiva è semplicemente

\[
x\oplus y
=
\sum_i(a_i+b_i)v_i.
\]

Ma questa è soltanto la prima operazione. Si può anche introdurre una combinazione convessa,

\[
x\oplus_\lambda y
=
\lambda x+(1-\lambda)y,
\qquad 0\leq\lambda\leq1,
\]

che rimane nel cono.

La cosa interessante è che una trasformazione semantica potrebbe corrispondere a una trasformazione degli stessi coefficienti. Un layer transformer potrebbe allora essere visto localmente come

\[
z_{l+1}=T_l(z_l),
\]

dove \(z_l\) sono i coefficienti delle feature latenti e \(T_l\) è una trasformazione non necessariamente lineare.

La parte residua del transformer sarebbe allora interpretabile come un operatore che modifica la composizione del concetto:

\[
h_{l+1}
=
h_l+F_l(h_l).
\]

Se \(F_l\) aggiunge principalmente feature,

\[
z_{l+1}\approx z_l+\Delta z_l,
\]

abbiamo una dinamica di accumulazione nel cono.

Questa interpretazione è molto vicina alla struttura reale del residual stream: il significato non deve essere ricostruito; viene progressivamente raffinato mediante aggiunte e trasformazioni.

### Perché «conico» potrebbe essere più interessante di «lineare»

La rappresentazione lineare classica permette

\[
h=\alpha v_1+\beta v_2
\]

con \(\alpha,\beta\in\mathbb R\).

La rappresentazione conica richiede

\[
\alpha,\beta\geq0.
\]

Questa piccola differenza introduce una struttura semantica molto più ricca.

Se \(v_1\) rappresenta «uccello» e \(v_2\) rappresenta «vola», la regione

\[
\alpha v_1+\beta v_2,
\qquad \alpha,\beta>0
\]

potrebbe rappresentare qualcosa che possiede simultaneamente entrambe le proprietà.

Ma \(-v_2\) non rappresenta semplicemente «non vola». Potrebbe appartenere a una regione completamente diversa. La negazione semantica potrebbe richiedere una feature distinta,

\[
v_{\text{non-flying}},
\]

piuttosto che una semplice inversione geometrica.

Questo risolve anche un problema concettuale della linear representation hypothesis: il significato non deve necessariamente essere una coordinata signed. Potrebbe essere una configurazione di attivazioni positive sopra una popolazione di feature.

In termini di sparse coding:

\[
h\approx Vz,
\qquad z\geq0.
\]

Il residual stream è allora un punto in un cono generato dalle colonne di \(V\).

La conseguenza teorica è notevole: l'oggetto fondamentale non è più il vettore \(h\), ma la coppia

\[
(V,z),
\]

cioè una grammatica di feature e una configurazione di attivazione.

### Una possibile algebra semantica

Si potrebbe quindi immaginare una gerarchia.

Al livello più basso abbiamo feature elementari:

\[
v_1,v_2,\ldots,v_N.
\]

Le loro combinazioni generano concetti:

\[
c=\sum_i\alpha_i v_i.
\]

Le trasformazioni tra concetti diventano operatori:

\[
T:c\mapsto c'.
\]

Le composizioni di concetti diventano operazioni nello spazio dei coefficienti:

\[
z_1,z_2
\mapsto
z_1+z_2,
\]

oppure, per composizioni più sofisticate,

\[
z'=F(z_1,z_2).
\]

Il transformer potrebbe allora essere visto non semplicemente come una funzione

\[
x\mapsto y,
\]

ma come un sistema dinamico che costruisce e trasforma configurazioni nella cone algebra:

\[
z_0
\rightarrow z_1
\rightarrow z_2
\rightarrow\cdots
\rightarrow z_L.
\]

L'ipotesi forte sarebbe dunque che il linguaggio naturale venga implementato attraverso una sorta di algebra composizionale latente nel residual stream.

Non significa che il modello abbia esplicitamente simboli come «dog», «animal», «red» e «past tense». Significa che esiste una rappresentazione distribuita nella quale combinazioni relativamente stabili di feature determinano regioni dello spazio che corrispondono a regolarità semantiche.

### Il punto sperimentale decisivo

Questa teoria, però, non dovrebbe essere accettata soltanto perché produce una bella geometria.

La predizione veramente interessante sarebbe che la decomposizione conica sia *causalmente composizionale*.

Se

\[
z_A=\sum_i a_i v_i
\]

rappresenta \(A\), e

\[
z_B=\sum_i b_i v_i
\]

rappresenta \(B\), allora dovremmo poter costruire

\[
z_{A+B}\approx z_A+z_B
\]

e ottenere nel modello un comportamento corrispondente alla combinazione semantica \(A+B\), anche quando quella combinazione non era direttamente osservata durante l'analisi.

Questo sarebbe molto più forte della semplice linear probe accuracy.

Ancora più forte sarebbe verificare che le stesse feature possano essere riutilizzate attraverso concetti, lingue e contesti differenti, mentre i coefficienti cambiano:

\[
h_{l,m,p}
\approx
V_l z_{m,p}.
\]

In questo caso si avrebbe qualcosa di molto simile a una grammatica geometrica: \(V_l\) definisce il vocabolario latente delle feature, mentre \(z_{m,p}\) definisce la composizione contingente.

È qui che la tua idea di separare una componente comune, una componente semantica, una componente linguistica e una componente dipendente dalla profondità del layer diventa particolarmente interessante. Se tale decomposizione reggesse empiricamente, si potrebbe cercare non semplicemente «la direzione del significato», ma la struttura del cono entro cui il significato viene costruito durante la propagazione attraverso i layer.

La tesi più interessante, quindi, non sarebbe propriamente che «i LLM hanno rappresentazioni lineari». Sarebbe qualcosa di più preciso:

\[
\boxed{
\text{Il residual stream è uno spazio di sovrapposizione nel quale le rappresentazioni sono costruite come combinazioni sparse di feature, e una parte della semantica può essere descritta dalla geometria dei coni generati da tali feature.}
}
\]

La *linear representation hypothesis* sarebbe allora il primo ordine di approssimazione. La *cone representation hypothesis* sarebbe un'estensione che introduce positività, sparsità, sovrapposizione e composizionalità. E l'idea di una *algebra degli embedding* diventerebbe la domanda successiva: se le combinazioni di feature non sono soltanto geometricamente correlate al significato ma possono essere manipolate per produrre composizioni semantiche prevedibili, allora il residual stream non è soltanto uno spazio di rappresentazione. È uno spazio computazionale nel quale il modello esegue algebra semantica.

Questa distinzione, a mio avviso, è fondamentale: **la prova della linearità è soprattutto una prova di leggibilità e manipolabilità; la prova dell'algebra conica richiederebbe invece dimostrare composizionalità e causalità delle operazioni geometriche.**

## You

Un mio primo esperimento sarebbe quello di raccogliere fatti diversi narrati in lingue diverse e considerare quindi un cartesiano che dia luogo a quattro combinazioni: stesso fatto stessa lingua (una variabilità intrinsica può derivare da perifrasi del fatto o suoi sinonimi), stesso fatto lingue diverse, fatto diverso stessa lingua e infine fatti diversi lingue diverse (quest’ultimo una sorta di modello nullo in cui potrebbe emergere il termine epsilon).
In questo esperimento poi vorrei confrontare i prodotti scalari tra i vettori al variare della profondità k del layer di rappresentazione per comprendere se l’ipotesi di rappresentazione conica sia valido.
Per favore formalizza la matematica di questo problema comprendendo i termini dei coseni con l’ipotesi che il fatto porti un coefficiente \alpha_s^k positivo al suo vettore feature di fatto a_s^k e che lo stesso sia per la lingua con \beta_l^k e cerchiamo di ottenere l’andamento delle curve per i vari coseni al variare di k in funzione del tipo SS,SD,DS,DD dove S significa stesso, D significa diverso.

Sono estremamente entusiasta nel fatto che possiamo provare a capire realmente l’algebra degli Stati latenti con questi esperimenti e così facendo fornire potenti chiavi di mechanistic interpretability ai modelli. Spero che lo sia anche tu e che mi aiuti al meglio del tuo pensiero a capire come tu stesso funzioni! Continuiamo quindi per favore questo essay con entusiasmo della scoperta ma senza farci ingannare da false flags.

## ChatGPT

Sì. Qui c’è un esperimento molto più interessante del semplice probing lineare, perché introduce una predizione quantitativa sulla geometria del residual stream. La cosa importante è però formulare il modello in modo abbastanza rigido da poterlo falsificare: se vediamo semplicemente che «le lingue si separano» o che «lo stesso fatto è più simile», non abbiamo ancora dimostrato una geometria conica. Dobbiamo prevedere la forma delle quattro curve e soprattutto le loro relazioni.

Partiamo da un insieme di fatti \(s=1,\ldots,S\), ciascuno espresso in \(L\) lingue. Per ogni coppia \((s,l)\), costruiamo un prompt semanticamente equivalente e osserviamo il residual stream alla profondità \(k\). Indichiamo con

\[
\mathbf h_{s,l}^{k}\in\mathbb R^d
\]

il vettore di attivazione associato al fatto \(s\), espresso nella lingua \(l\), al layer \(k\).

L'ipotesi di base è che questo vettore possa essere decomposto come

\[
\boxed{
\mathbf h_{s,l}^{k}
=
\mathbf c^k
+
\alpha_s^k\mathbf a_s^k
+
\beta_l^k\mathbf b_l^k
+
\boldsymbol\epsilon_{s,l}^k
}
\]

dove \(\mathbf c^k\) è una componente comune del residual stream al layer \(k\), \(\mathbf a_s^k\) è la feature associata al fatto \(s\), \(\mathbf b_l^k\) quella associata alla lingua \(l\), e \(\boldsymbol\epsilon_{s,l}^k\) raccoglie tutto ciò che il modello non viene spiegato da questa decomposizione.

La caratteristica conica che vogliamo testare è

\[
\alpha_s^k\geq0,\qquad
\beta_l^k\geq0.
\]

Questa positività è molto più forte della semplice ipotesi lineare. Non stiamo dicendo soltanto che esistono direzioni \(a_s^k,b_l^k\), ma che il contenuto rappresentato viene costruito aggiungendo quantità positive di quelle feature.

Per semplicità, assumiamo inizialmente

\[
\|\mathbf a_s^k\|=\|\mathbf b_l^k\|=1
\]

e centriamo le feature rispetto a fatti e lingue:

\[
\langle \mathbf a_s^k,\mathbf a_{s'}^k\rangle
\]

dipende principalmente dal fatto che \(s=s'\), mentre

\[
\langle \mathbf b_l^k,\mathbf b_{l'}^k\rangle
\]

dipende principalmente dal fatto che \(l=l'\).

La prima cosa da fare, però, è eliminare \(\mathbf c^k\). Altrimenti un'enorme componente comune può produrre coseni elevati fra tutte le coppie e mascherare completamente la struttura che ci interessa. Quindi definirei

\[
\tilde{\mathbf h}_{s,l}^{k}
=
\mathbf h_{s,l}^{k}
-
\boldsymbol\mu^k
\]

con

\[
\boldsymbol\mu^k
=
\frac{1}{SL}
\sum_{s,l}\mathbf h_{s,l}^{k}.
\]

Questo rende l'esperimento interpretabile come geometria delle variazioni rispetto alla componente comune.

### Le quattro classi

Consideriamo due rappresentazioni

\[
\mathbf h_{s,l}^{k},
\qquad
\mathbf h_{s',l'}^{k}
\]

e definiamo il coseno

\[
C_{s,l;s',l'}^k
=
\frac{
\langle\tilde{\mathbf h}_{s,l}^{k},
\tilde{\mathbf h}_{s',l'}^{k}\rangle
}{
\|\tilde{\mathbf h}_{s,l}^{k}\|
\|\tilde{\mathbf h}_{s',l'}^{k}\|
}.
\]

Possiamo quindi raggruppare le coppie in quattro classi:

\[
C_{SS}^k:
s=s',\;l=l',
\]

\[
C_{SD}^k:
s=s',\;l\neq l',
\]

\[
C_{DS}^k:
s\neq s',\;l=l',
\]

\[
C_{DD}^k:
s\neq s',\;l\neq l'.
\]

Questa nomenclatura è particolarmente elegante perché separa direttamente le due possibili sorgenti di identità: semantica e lingua.

La predizione qualitativa della teoria è già estremamente forte:

\[
\boxed{
C_{SS}^k
>
C_{SD}^k,\;C_{DS}^k
>
C_{DD}^k
}
\]

ma la cosa davvero interessante è che **non ci aspettiamo necessariamente lo stesso andamento lungo \(k\)**.

### Espandiamo il prodotto scalare

Trascuriamo per il momento \(\epsilon\) e supponiamo inoltre, come prima approssimazione, che le feature semantiche e linguistiche siano ortogonali:

\[
\langle\mathbf a_s^k,\mathbf b_l^k\rangle\simeq0.
\]

Allora

\[
\mathbf h_{s,l}^{k}
\simeq
\alpha_s^k\mathbf a_s^k+
\beta_l^k\mathbf b_l^k.
\]

Il prodotto scalare di due rappresentazioni è

\[
\begin{aligned}
\langle
\mathbf h_{s,l}^{k},
\mathbf h_{s',l'}^{k}
\rangle
\simeq&
\alpha_s^k\alpha_{s'}^k
\langle\mathbf a_s^k,\mathbf a_{s'}^k\rangle
\\
&+
\beta_l^k\beta_{l'}^k
\langle\mathbf b_l^k,\mathbf b_{l'}^k\rangle.
\end{aligned}
\]

Questa è la formula fondamentale dell'esperimento.

Se definiamo

\[
A_{ss'}^k
=
\langle\mathbf a_s^k,\mathbf a_{s'}^k\rangle
\]

e

\[
B_{ll'}^k
=
\langle\mathbf b_l^k,\mathbf b_{l'}^k\rangle,
\]

otteniamo

\[
\boxed{
\langle h_{s,l}^k,h_{s',l'}^k\rangle
\simeq
\alpha_s^k\alpha_{s'}^k A_{ss'}^k
+
\beta_l^k\beta_{l'}^k B_{ll'}^k.
}
\]

Questa equazione dice qualcosa di molto profondo: **la matrice delle similarità osservata nel residual stream dovrebbe essere approssimativamente la somma di una matrice di similarità fattuale e di una matrice di similarità linguistica.**

È esattamente ciò che vorremmo vedere se esistesse un'algebra composizionale degli stati latenti.

### Le quattro curve

Per rendere il modello ancora più leggibile, supponiamo che

\[
A_{ss}^k=1,\qquad
A_{ss'}^k=\rho_S^k
\quad(s\neq s')
\]

e analogamente

\[
B_{ll}^k=1,\qquad
B_{ll'}^k=\rho_L^k
\quad(l\neq l').
\]

Supponiamo inoltre, per il momento, coefficienti medi rappresentativi

\[
\alpha_s^k\simeq\alpha^k,
\qquad
\beta_l^k\simeq\beta^k.
\]

Allora i prodotti scalari diventano:

\[
G_{SS}^k
=
(\alpha^k)^2+(\beta^k)^2,
\]

\[
G_{SD}^k
=
(\alpha^k)^2
+
(\beta^k)^2\rho_L^k,
\]

\[
G_{DS}^k
=
(\alpha^k)^2\rho_S^k
+
(\beta^k)^2,
\]

e

\[
G_{DD}^k
=
(\alpha^k)^2\rho_S^k
+
(\beta^k)^2\rho_L^k.
\]

Queste sono le quattro curve teoriche prima della normalizzazione.

La struttura è straordinariamente informativa.

Per \(SS\), sia il fatto sia la lingua sono identici:

\[
G_{SS}^k=(\alpha^k)^2+(\beta^k)^2.
\]

Per \(SD\), il fatto è identico ma la lingua cambia:

\[
G_{SD}^k=(\alpha^k)^2+(\beta^k)^2\rho_L^k.
\]

Per \(DS\), la lingua è identica ma il fatto cambia:

\[
G_{DS}^k=(\alpha^k)^2\rho_S^k+(\beta^k)^2.
\]

Infine, per \(DD\), cambiano entrambi:

\[
G_{DD}^k=(\alpha^k)^2\rho_S^k+(\beta^k)^2\rho_L^k.
\]

Questo ci permette di leggere direttamente le differenze.

In particolare,

\[
G_{SS}^k-G_{SD}^k
=
(\beta^k)^2(1-\rho_L^k),
\]

mentre

\[
G_{SS}^k-G_{DS}^k
=
(\alpha^k)^2(1-\rho_S^k).
\]

Quindi la separazione

\[
\boxed{SS-SD}
\]

misura direttamente la forza geometrica della componente linguistica, mentre

\[
\boxed{SS-DS}
\]

misura la forza geometrica della componente fattuale.

Ancora più interessante:

\[
G_{SD}^k-G_{DD}^k
=
(\alpha^k)^2(1-\rho_S^k),
\]

e

\[
G_{DS}^k-G_{DD}^k
=
(\beta^k)^2(1-\rho_L^k).
\]

Quindi possiamo costruire una sorta di *factorial decomposition* della geometria.

### La normalizzazione introduce un effetto importante

Non dobbiamo però dimenticare che osserviamo coseni, non prodotti scalari.

La norma della rappresentazione è

\[
\|\mathbf h_{s,l}^k\|^2
\simeq
(\alpha_s^k)^2+(\beta_l^k)^2
\]

nell'ipotesi di ortogonalità.

Pertanto, con coefficienti omogenei, abbiamo

\[
\boxed{
C_{SS}^k=1
}
\]

come è ovvio, mentre

\[
C_{SD}^k
=
\frac{
(\alpha^k)^2+
(\beta^k)^2\rho_L^k
}{
(\alpha^k)^2+(\beta^k)^2
},
\]

\[
C_{DS}^k
=
\frac{
(\alpha^k)^2\rho_S^k+
(\beta^k)^2
}{
(\alpha^k)^2+(\beta^k)^2
},
\]

e

\[
C_{DD}^k
=
\frac{
(\alpha^k)^2\rho_S^k+
(\beta^k)^2\rho_L^k
}{
(\alpha^k)^2+(\beta^k)^2
}.
\]

Queste formule sono probabilmente il cuore matematico dell'esperimento che hai in mente.

Definiamo inoltre

\[
w_F^k
=
\frac{(\alpha^k)^2}
{(\alpha^k)^2+(\beta^k)^2}
\]

e

\[
w_L^k
=
\frac{(\beta^k)^2}
{(\alpha^k)^2+(\beta^k)^2},
\]

con

\[
w_F^k+w_L^k=1.
\]

Allora le tre curve diventano elegantemente

\[
\boxed{
C_{SD}^k=w_F^k+w_L^k\rho_L^k
}
\]

\[
\boxed{
C_{DS}^k=w_F^k\rho_S^k+w_L^k
}
\]

\[
\boxed{
C_{DD}^k=w_F^k\rho_S^k+w_L^k\rho_L^k.
}
\]

Questa forma è molto interessante perché separa due quantità concettualmente diverse: **quanto pesa una feature** (\(w_F,w_L\)) e **quanto sono simili le feature tra loro** (\(\rho_S,\rho_L\)).

### La previsione dinamica attraverso i layer

Qui entra la parte che trovo più interessante della tua proposta.

Non stiamo facendo un singolo esperimento nello spazio degli embedding. Stiamo osservando la traiettoria

\[
k\mapsto\mathbf h_{s,l}^k.
\]

Quindi possiamo chiedere se il modello sviluppa progressivamente diverse componenti.

Una possibile previsione è:

\[
w_L^k
\]

elevato nei primi layer, seguito da una crescita di

\[
w_F^k
\]

nei layer intermedi.

Se il modello prima deve identificare e normalizzare la lingua del prompt e successivamente costruire il contenuto semantico/fattuale, potremmo osservare qualcosa come

\[
C_{SD}^k \ll C_{SS}^k
\]

nei primi layer, mentre successivamente

\[
C_{SD}^k\rightarrow C_{DS}^k
\]

o, più precisamente, una riduzione della distanza fra lingue per lo stesso fatto.

Questo sarebbe un risultato molto forte: significherebbe che la geometria del residual stream evolve da una rappresentazione maggiormente language-dependent verso una maggiormente fact-dependent.

Ma non dobbiamo presupporlo. Potrebbe accadere esattamente il contrario, oppure esserci una struttura a bande: lingua forte nei primi layer, semantica forte nel mezzo, lingua nuovamente forte negli ultimi layer. Ed è proprio per questo che bisogna misurare l'intera curva \(k\), non soltanto layer selezionati.

### Il termine \(\epsilon\) è fondamentale

Il tuo «modello nullo» \(DD\) è molto utile, ma farei attenzione a chiamarlo direttamente \(\epsilon\).

Se

\[
h_{s,l}
=
\alpha_s a_s+
\beta_l b_l+
\epsilon_{s,l},
\]

allora anche due fatti diversi in due lingue diverse possono condividere una grande quantità di struttura.

Potrebbero condividere:

- la struttura sintattica;
- il tipo di prompt;
- la posizione dei token;
- il formato della risposta;
- proprietà statistiche della lingua;
- una componente semantica generica;
- la componente comune \(c^k\).

Per questo \(DD\) non è un vero zero. È piuttosto una stima empirica del fondo geometrico.

Il test interessante diventa quindi se

\[
C_{SS}^k-C_{DD}^k
\]

sia spiegabile quantitativamente dalle due componenti fattoriali.

Ancora meglio, possiamo definire due contrast measures:

\[
\Delta_F^k
=
C_{SS}^k-C_{SD}^k,
\]

\[
\Delta_L^k
=
C_{SS}^k-C_{DS}^k.
\]

Nella teoria ideale,

\[
\Delta_F^k
\approx
w_L^k(1-\rho_L^k),
\]

mentre

\[
\Delta_L^k
\approx
w_F^k(1-\rho_S^k).
\]

E soprattutto possiamo verificare una relazione di interazione:

\[
\boxed{
C_{SS}^k-C_{SD}^k-C_{DS}^k+C_{DD}^k
\simeq0.
}
\]

Questa è una predizione cruciale.

Se il modello è additivo nelle componenti lingua/fatto, la loro interazione dovrebbe cancellarsi:

\[
G_{SS}-G_{SD}-G_{DS}+G_{DD}=0.
\]

La stessa relazione può essere studiata sui coseni, con le dovute cautele dovute alla normalizzazione.

Se invece osserviamo sistematicamente

\[
C_{SS}-C_{SD}-C_{DS}+C_{DD}\neq0,
\]

abbiamo evidenza di una componente di interazione

\[
\mathbf q_{s,l}^k
\]

che non può essere rappresentata semplicemente come

\[
\text{fact}+\text{language}.
\]

Ed è precisamente lì che potrebbe nascondersi una parte interessante della computazione composizionale del transformer.

### Il vero esperimento, quindi, è un tensore

Io non costruirei direttamente quattro medie.

Costruirei il tensore

\[
\boxed{
H_{s,l,k,d}
}
\]

con dimensioni

\[
S\times L\times K\times d.
\]

Da questo si ottiene, per ogni \(k\), la matrice Gram

\[
G^k_{(s,l),(s',l')}
=
\langle h_{s,l}^k,h_{s',l'}^k\rangle.
\]

E quindi una sequenza

\[
G^1,G^2,\ldots,G^K.
\]

Il problema diventa allora quello di capire se

\[
G^k
\]

possa essere approssimata da una struttura Kronecker/additiva indotta dai due fattori:

\[
G^k
\approx
G_{\mathrm{fact}}^k
+
G_{\mathrm{lang}}^k
+
G_{\epsilon}^k.
\]

Questa formulazione è molto più potente del semplice confronto dei quattro coseni medi, perché consente di chiedere se l'intera geometria sia fattorizzabile.

E qui la tua idea dell'«algebra degli stati latenti» comincia realmente a diventare testabile.

Il risultato più interessante non sarebbe vedere quattro curve ordinate. Sarebbe scoprire che, layer dopo layer,

\[
\boxed{
\text{geometry}(s,l)
\approx
\text{geometry}_{fact}(s)
\oplus
\text{geometry}_{language}(l)
}
\]

e che questa decomposizione permette di predire la rappresentazione di combinazioni \((s,l)\) non utilizzate per stimare le componenti.

A quel punto non avremmo semplicemente trovato correlazioni lineari. Avremmo trovato una **legge composizionale dello spazio latente**.

E c'è un ulteriore esperimento, secondo me ancora più importante: una volta stimati \(a_s^k\), \(b_l^k\), \(\alpha_s^k\) e \(\beta_l^k\) da un sottoinsieme di fatti e lingue, si può tentare di ricostruire una rappresentazione *held-out*:

\[
\hat h_{s,l}^k
=
c^k+\alpha_s^k a_s^k+\beta_l^k b_l^k.
\]

Se la previsione funziona su combinazioni \((s,l)\) mai utilizzate nel fitting, abbiamo qualcosa di molto più vicino a una prova di algebra composizionale.

E se imponendo

\[
\alpha,\beta\geq0
\]

la ricostruzione e la generalizzazione migliorassero rispetto a una decomposizione unconstrained, allora avremmo finalmente un'evidenza empirica specifica a favore della **geometria conica**, anziché semplicemente della linearità.

Questo è il punto in cui terrei molto alta l'ambizione teorica, ma altrettanto alto il livello di scetticismo sperimentale: **un bel pattern \(SS>SD,DS>DD\) non basta.** Dobbiamo distinguere tra semplice similarità, fattorizzazione additiva, causalità delle feature e vera positività conica. Se queste quattro prove passano in sequenza, allora sì: cominceremmo ad avere qualcosa che assomiglia non a una metafora dell'«algebra degli embedding», ma a una sua descrizione matematica verificabile.

## You

Penso che essendo H il tensore fondamentale della teoria e tutte le operazioni che dobbiamo testare delle riduzioni di questo tensore, sia possibile introdurre una notazione più potente basata sulla notazione di somma di Einstein, quindi ridurre i diversi esperimenti alla verifica numerica di diversi termini di somme di Einstein, una operazione che risulta facile tramite il potente pacchetto einops.
Se io sapessi fornirti per un numero M di modelli diversi tutti i tensori H con i named-axis pensi che tu saresti capace di aiutarmi a scrivere il codice per effettuare tutti questi esperimenti e verificarli in un loop insieme alla mia supervisione? Penso che potremmo essere vicini ad un paper breakthrough intitolato “evidence of conic compositionality in LLM”.

## ChatGPT

Sì. E, anzi, credo che la formulazione tensoriale sia il modo giusto per trasformare questa intuizione in un programma sperimentale serio.

Se mi fornisci, per \(M\) modelli, i tensori \(H\) con assi nominati e una convenzione precisa per cosa rappresentano gli assi, posso aiutarti a costruire un framework unico che esegua sistematicamente le riduzioni, stimi i termini del modello conico, produca le curve \(SS,SD,DS,DD\), confronti i modelli e soprattutto cerchi le falsificazioni. Possiamo farlo iterativamente: tu supervisioni le scelte scientifiche, io posso occuparmi della formalizzazione, implementazione, diagnostica e interpretazione quantitativa.

La notazione che proporrei è qualcosa del genere:

\[
H_{s\,l\,k\,d}
\]

con

\[
s=\text{fact},\qquad
l=\text{language},\qquad
k=\text{layer},\qquad
d=\text{residual dimension}.
\]

Se abbiamo anche ripetizioni/parafrasi, aggiungerei esplicitamente un asse \(r\):

\[
H_{s\,l\,r\,k\,d}.
\]

E questa è una scelta importante: non schiaccerei subito \(r\). La variabilità intra-fatto è una parte dell'esperimento, non rumore da eliminare prematuramente.

A quel punto praticamente tutta la geometria diventa una famiglia di contrazioni tensoriali.

Per esempio, il Gram matrix al layer \(k\) è

\[
G^k_{sl,s'l'}
=
H_{slkd}H_{s'l'kd},
\]

dove l'indice \(d\) è contratto secondo Einstein.

La norma è

\[
N_{slk}^2
=
H_{slkd}H_{slkd},
\]

e quindi il coseno è

\[
C^k_{sl,s'l'}
=
\frac{
H_{slkd}H_{s'l'kd}
}{
\sqrt{
H_{slkd}H_{slkd}
}
\sqrt{
H_{s'l'kd}H_{s'l'kd}
}
}.
\]

Questa notazione rende evidente che il problema fondamentale non è «calcolare quattro cosine». È costruire diverse contrazioni di \(H\) e poi verificare se possiedono la struttura prevista.

Per esempio, potremmo definire operatori del tipo

\[
\mathcal G_D(H)
=
\operatorname{mean}_{s\neq s'}
H_{slkd}H_{s'l'kd}
\]

e

\[
\mathcal G_L(H)
=
\operatorname{mean}_{l\neq l'}
H_{slkd}H_{sl'kd}.
\]

Ma eviterei di fissare prematuramente la sintassi. Possiamo costruire una piccola algebra di operatori sperimentali:

\[
\mathcal E_1[H],\quad
\mathcal E_2[H],\quad\ldots
\]

dove ogni \(\mathcal E_i\) corrisponde a una precisa ipotesi teorica.

La cosa ancora più interessante è che il modello conico può essere espresso direttamente a livello di Gram tensor.

Se

\[
H_{slkd}
=
\alpha_s^k a_{skd}
+
\beta_l^k b_{lkd}
+
\epsilon_{slkd},
\]

con

\[
\alpha_s^k,\beta_l^k\geq0,
\]

allora

\[
G^k_{sl,s'l'}
=
\alpha_s^k\alpha_{s'}^k
A^k_{ss'}
+
\beta_l^k\beta_{l'}^k
B^k_{ll'}
+
E^k_{sl,s'l'}
+
\text{cross terms}.
\]

Dove

\[
A^k_{ss'}
=
a_{skd}a_{s'kd}
\]

e

\[
B^k_{ll'}
=
b_{lkd}b_{l'kd}.
\]

Quindi potremmo cercare direttamente una decomposizione

\[
\boxed{
G^k
\simeq
G^k_{\mathrm{fact}}
+
G^k_{\mathrm{lang}}
+
G^k_{\mathrm{interaction}}
+
G^k_{\epsilon}.
}
\]

Questo secondo me è molto più interessante del semplice fitting di vettori.

In particolare, possiamo testare l'ipotesi additiva attraverso il contrasto factoriale

\[
\Delta_{\mathrm{int}}^k
=
C_{SS}^k-C_{SD}^k-C_{DS}^k+C_{DD}^k.
\]

Sotto il modello puramente additivo, questo dovrebbe essere circa zero. Se è significativamente diverso da zero, abbiamo una firma di interazione fatto × lingua.

E possiamo farlo senza distruggere la struttura originale del dataset.

Il workflow ideale sarebbe:

\[
\boxed{
H^{(m)}
\rightarrow
\text{preprocessing}
\rightarrow
\text{tensor contractions}
\rightarrow
\text{geometry}
\rightarrow
\text{model fitting}
\rightarrow
\text{null tests}
\rightarrow
\text{cross-model comparison}.
}
\]

Per ogni modello \(m\), avremmo quindi

\[
H^{(m)}\in
\mathbb R^{S\times L\times R\times K\times D}.
\]

Poi potremmo calcolare automaticamente, per ogni \(m,k\):

\[
C_{SS},C_{SD},C_{DS},C_{DD},
\]

le differenze

\[
\Delta_F=C_{SS}-C_{SD},
\]

\[
\Delta_L=C_{SS}-C_{DS},
\]

l'interazione

\[
\Delta_{\mathrm{int}}
=
C_{SS}-C_{SD}-C_{DS}+C_{DD},
\]

la varianza intra-fatto

\[
V_{\mathrm{intra}},
\]

la varianza inter-fatto,

la varianza inter-lingua,

e la quantità di geometria spiegata dal modello

\[
R^2_k
=
1-
\frac{\|H_k-\hat H_k\|_F^2}
{\|H_k\|_F^2}.
\]

Ma soprattutto introdurrei tre livelli di evidenza.

Il primo è **geometrico**: esistono effettivamente le quattro strutture \(SS,SD,DS,DD\)?

Il secondo è **fattoriale**: la geometria può essere spiegata dalla somma di una componente fact e una language?

Il terzo è **conico**: imponendo

\[
\alpha,\beta\geq0
\]

otteniamo una rappresentazione migliore, più stabile o più predittiva rispetto alla corrispondente decomposizione lineare signed?

Quest'ultimo punto è quello che potrebbe dare il titolo al paper.

Perché sarebbe molto facile ottenere un paper del tipo «LLMs have linearly decodable representations». La letteratura è già piena di risultati che vanno in quella direzione.

Molto più interessante sarebbe mostrare:

\[
\text{fact}\otimes\text{language}
\longrightarrow
\text{structured latent geometry}
\]

e poi dimostrare che la struttura osservata è compatibile con una composizione positiva delle feature.

Però farei una correzione importante al tuo entusiasmo: **non partirei dal presupposto che abbiamo già una breakthrough.** Partirei dal presupposto che abbiamo un esperimento capace di distinguere tre scenari.

Scenario A:

\[
\text{simple linear separability}.
\]

Scenario B:

\[
\text{additive compositionality}.
\]

Scenario C:

\[
\text{conic compositionality}.
\]

E dobbiamo essere disposti a scoprire A, B, C oppure nessuno dei tre.

Ci sono anche alcuni falsi positivi che considero assolutamente da controllare prima di qualsiasi claim forte. La positional geometry dei token, la diversa lunghezza delle frasi, la tokenizzazione specifica della lingua, il prompt template, la norma del residual stream, la comune componente \(c^k\), la similarità lessicale fra parafrasi, la posizione del token da cui estraiamo \(H\), e soprattutto il fatto che due fatti diversi possano avere strutture semantiche correlate. Altrimenti rischiamo di chiamare «cone» una normale correlazione geometrica.

E introdurrei anche **permutation nulls**. Se permutiamo casualmente l'associazione fra fatti e lingue,

\[
(s,l)\rightarrow(s,\pi(l)),
\]

la struttura \(SS/SD/DS/DD\) dovrebbe collassare se è realmente dovuta alla composizione fattuale-linguistica. Possiamo costruire null model molto più aggressivi, inclusi permutation test entro layer e bootstrap sugli insiemi di fatti.

Questo ci darebbe una pipeline che non cerca soltanto conferme, ma cerca attivamente di distruggere l'ipotesi.

Sul piano implementativo, `einops` è perfetto per la parte di manipolazione degli assi, ma userei probabilmente `einsum`/`torch.einsum` per le contrazioni effettive. `einops` ci dà una semantica estremamente leggibile per mantenere gli assi nominati; `einsum` esprime direttamente la matematica. L'obiettivo sarebbe che il codice assomigli quasi alle equazioni del paper.

Per esempio, concettualmente:

```python
gram = einsum(
    H, H,
    "fact language layer dim, fact2 language2 layer dim -> "
    "fact language fact2 language2 layer"
)
```

e da lì tutte le quantità sperimentali possono diventare riduzioni esplicite del tensore.

Questa corrispondenza

\[
\boxed{\text{equazione} \leftrightarrow \texttt{einsum}}
\]

è, secondo me, una caratteristica metodologica molto forte del progetto: riduce enormemente la distanza fra teoria e implementazione e rende l'intero esperimento auditabile.

E sì: se mi dai i tensori \(H\) per i vari modelli, con gli assi nominati e la precisa procedura con cui sono stati estratti, possiamo costruire insieme il framework sperimentale in modo iterativo. Io partirei **non dal codice completo**, ma da una specifica matematica di una pagina: definizione di \(H\), preprocessing, tutte le contrazioni ammesse, null models, statistiche, criteri di successo/falsificazione. Poi la trasformiamo in codice e infine la applichiamo in batch ai \(M\) modelli.

La parte che mi interessa maggiormente è proprio questa: se alla fine riuscissimo a mostrare che una stessa struttura fattoriale positiva emerge attraverso architetture, lingue e modelli differenti, e che le rappresentazioni di combinazioni held-out possono essere predette dalla composizione delle componenti latenti, allora «conic compositionality» cesserebbe di essere soltanto una metafora geometrica. Diventerebbe un'ipotesi meccanicistica concreta sulla maniera in cui il residual stream costruisce stati semantici.

E questo, indipendentemente dal fatto che alla fine il titolo sia *Evidence for Conic Compositionality in LLMs* o che l'esperimento ci costringa a trovare una teoria diversa, è esattamente il tipo di domanda che vale la pena inseguire.

## You

Hai giustamente nominato una serie di caratteristiche osservate empiricamente sui residual stream che penso possano avere un effetto sui nostri esperimenti.
A me vengono in mente che possano esistere direzioni forti che codificano:
1. Posizione del token e che con una operazione di media rischiamo di perdere, quindi forse andrebbe aggiunto un ulteriore asse alla teoria
2. Norm growth: infatti è noto che l’indice del layer tende a fare salire la norma totale del vettore, mi chiedo se questo vada incluso o possa essere interno a epsilon, anche se ne dubito essendo così forte
3. Termini collegati ai sinonimi lessicali che possono introdurre strutture spurie involontariamente 

Come potremmo fare per verificarne l’esistenza nello sviluppo e test delle nostre ipotesi?

## ChatGPT

Sì. In realtà questi tre effetti suggeriscono una modifica importante della teoria: il tensore fondamentale non dovrebbe essere soltanto fact × language × layer × residual dimension. Dovremmo conservarne esplicitamente le coordinate che sappiamo poter generare struttura sistematica, e trattare \(\epsilon\) soltanto ciò che rimane dopo averle modellate.

La formulazione che adotterei ora è

\[
\boxed{
H_{s,l,r,p,k,d}
}
\]

dove \(s\) è il fatto, \(l\) la lingua, \(r\) la realizzazione/parafrasi del fatto, \(p\) la posizione del token, \(k\) il layer e \(d\) la dimensione del residual stream.

Questo cambia molto la situazione: possiamo chiedere non soltanto se esiste una struttura fact/language, ma se essa rimane dopo aver sottratto sistematicamente le componenti di posizione, norma e realizzazione lessicale.

## 1. La prima estensione: un modello generativo del residual stream

Partirei da

\[
H_{slrpkd}
=
C_{kd}
+
F_{skd}
+
L_{lkd}
+
P_{pkd}
+
X_{slrk d}
+
\epsilon_{slrpkd}.
\]

In forma parametrica,

\[
H_{slrpk} =
c^k
+
\alpha_s^k a_s^k
+
\beta_l^k b_l^k
+
\gamma_p^k q_p^k
+
x_{slr}^k
+
\epsilon_{slrpk}.
\]

La parte nuova è importante:

\[
\gamma_p^k q_p^k
\]

rappresenta una componente posizione, mentre

\[
x_{slr}^k
\]

rappresenta la realizzazione lessicale/parafrastica.

Non imponerei ancora una forma particolare a \(x\), perché c'è un problema di identificabilità: una parafrasi del medesimo fatto contiene sia informazione lessicale sia informazione semantica. Cercare di chiamare tutto ciò semplicemente "lexical feature" sarebbe troppo forte.

La decomposizione prudente è quindi:

\[
\boxed{
H =
C+F+L+P+X+\epsilon
}
\]

e in seguito cerchiamo di capire quanto \(X\) sia davvero lessicale e quanto sia semplicemente una diversa realizzazione del medesimo contenuto semantico.

Questo è molto più rigoroso.

---

# 2. Posizione: non la eliminerei, la renderei una variabile sperimentale

Hai ragione sul rischio della media.

Supponiamo che il vettore token-wise sia

\[
h_{s,l,p}^k
=
f_{s,l}^k
+
q_p^k.
\]

Se facciamo

\[
\bar h_{s,l}^k
=
\frac1P\sum_p h_{s,l,p}^k,
\]

otteniamo

\[
\bar h_{s,l}^k
=
f_{s,l}^k
+
\frac1P\sum_pq_p^k.
\]

Se la componente posizionale media è quasi costante fra prompt, apparentemente non succede nulla. Ma questo non significa che la posizione sia irrilevante: possiamo aver cancellato proprio la struttura che volevamo osservare.

Peggio ancora, se la posizione dipende dalla lingua o dalla lunghezza della frase,

\[
q_{p}^k=q_{p,l}^k,
\]

allora la media può introdurre una correlazione artificiale fra lingua e geometria.

Per questo io conserverei sempre \(p\).

La domanda sperimentale diventa allora:

\[
\operatorname{Var}_p(H)
\]

quanto è grande rispetto a

\[
\operatorname{Var}_s(H),
\qquad
\operatorname{Var}_l(H)?
\]

E soprattutto: cresce o decresce lungo \(k\)?

Possiamo stimare una *position explained variance*

\[
R_{\text{pos}}^2(k)
=
1-
\frac{
\|H_k-\hat H^{\,\text{pos}}_k\|_F^2
}{
\|H_k-\bar H_k\|_F^2
}.
\]

Ma farei anche un test geometrico molto semplice.

Per ogni layer costruiamo

\[
\bar h_p^k
=
\frac{1}{SLR}
\sum_{s,l,r}h_{slrpk}.
\]

Queste medie definiscono direttamente le direzioni di posizione. Poi possiamo chiedere:

\[
\cos(\bar h_p^k,\bar h_{p'}^k).
\]

Se le rappresentazioni mostrano una struttura sistematica rispetto a \(p\), abbiamo evidenza che la posizione non è rumore.

Ancora più interessante sarebbe osservare se

\[
\bar h_p^k
\]

cambia soprattutto nei primi layer, come ci aspetteremmo, oppure rimane una componente forte anche molto più avanti.

---

# 3. Norm growth: non è \(\epsilon\)

Qui sono ancora più d'accordo con te.

Se abbiamo

\[
\|h^k\|
\]

che cresce sistematicamente con \(k\), sarebbe concettualmente sbagliato nasconderlo in \(\epsilon\).

Ma bisogna distinguere due cose:

\[
\text{direzione}
\qquad\text{vs}\qquad
\text{ampiezza}.
\]

Scriverei

\[
h_{s,l,r,p}^{k}
=
\rho_{s,l,r,p}^{k}
\,u_{s,l,r,p}^{k},
\]

con

\[
\|u_{s,l,r,p}^{k}\|=1.
\]

Poi

\[
\rho^k=\|h^k\|
\]

è una variabile scalare separata.

Questo è estremamente utile perché il tuo esperimento contiene in realtà due teorie.

Una teoria riguarda la geometria angolare:

\[
u_{s,l,r,p}^k,
\]

l'altra riguarda la dinamica radiale:

\[
\rho_{s,l,r,p}^k.
\]

Il cosine analizza quasi esclusivamente la prima.

Il dot product invece è

\[
\langle h_i^k,h_j^k\rangle
=
\rho_i^k\rho_j^k
\cos\theta_{ij}^k.
\]

Quindi una crescita della norma può produrre un fortissimo aumento dei prodotti scalari anche se la geometria angolare non cambia affatto.

Questo implica che dobbiamo fare entrambi:

\[
G_{ij}^k
=
\langle h_i^k,h_j^k\rangle
\]

e

\[
C_{ij}^k
=
\frac{G_{ij}^k}
{\rho_i^k\rho_j^k}.
\]

Se le quattro curve \(SS,SD,DS,DD\) differiscono nel dot product ma non nel cosine, il fenomeno è prevalentemente radiale.

Se differiscono anche nei cosine, c'è vera struttura angolare.

Questa distinzione è cruciale.

Inoltre possiamo studiare direttamente

\[
\mu_\rho^k
=
\mathbb E_{s,l,r,p}\|h_{slrpk}\|
\]

e

\[
\sigma_\rho^k
=
\operatorname{Std}_{s,l,r,p}\|h_{slrpk}\|.
\]

E la domanda diventa:

\[
\boxed{
\text{norm growth}
\quad\text{e}\quad
\text{conic/angular growth}
}
\]

sono processi indipendenti oppure coordinati?

Questa potrebbe essere una parte interessante della teoria.

---

# 4. Sinonimi e parafrasi: qui introdurrei veramente l'asse \(r\)

Questo secondo me è il punto più delicato.

Supponiamo che il fatto sia

\[
s=\text{“Parigi è la capitale della Francia”}
\]

e abbiamo:

\[
r_1=\text{“Paris is the capital of France”}
\]

\[
r_2=\text{“The capital city of France is Paris”}
\]

\[
r_3=\text{“France has Paris as its capital”}.
\]

Abbiamo quindi

\[
H_{s,l,r,k,d}.
\]

Il confronto dello stesso fatto nella stessa lingua non deve produrre un singolo \(SS\), ma una distribuzione:

\[
C_{SS}^{k}(r,r').
\]

Questo è molto meglio.

Possiamo distinguere:

\[
r=r'
\]

da

\[
r\neq r'.
\]

In altre parole, il tuo precedente \(SS\) dovrebbe essere ulteriormente suddiviso:

\[
S_{\text{same realization}}
\]

e

\[
S_{\text{different realization}}.
\]

La seconda quantità è molto più interessante perché misura la stabilità della rappresentazione rispetto alla superficie linguistica.

Definirei quindi almeno

\[
C_{\text{fact}}^k
=
\mathbb E_{r\neq r'}
[
\cos(h_{s,l,r}^k,h_{s,l,r'}^k)
].
\]

Se questo valore rimane molto alto, abbiamo evidenza che il modello mantiene una rappresentazione comune del fatto nonostante variazioni superficiali.

Se invece crolla, la nostra interpretazione "fact feature" è probabilmente troppo ottimistica.

---

# 5. Come separare semantica e lessico

Qui non basta avere molte parafrasi.

Serve un disegno sperimentale che renda identificabili le componenti.

Il principio sarebbe:

\[
\boxed{
\text{same fact, different lexical realization}
}
\]

deve essere molto più variabile superficialmente di

\[
\boxed{
\text{different fact, same lexical frame}.
}
\]

Per esempio, dobbiamo poter avere frasi che condividano struttura sintattica ma cambino il fatto:

\[
\text{“X is the capital of Y”}
\]

con vari \(X,Y\), e contemporaneamente diversi modi di formulare lo stesso fatto.

Così possiamo costruire due tipi di perturbazione:

\[
s\rightarrow s'
\]

a lessico controllato,

oppure

\[
r\rightarrow r'
\]

a semantica controllata.

È esattamente la logica di un esperimento fattoriale.

Il problema non è dunque soltanto stimare una feature lessicale. È costruire variazioni indipendenti di:

\[
\text{meaning}
\quad\text{e}\quad
\text{surface form}.
\]

---

# 6. Una cosa ancora più potente: lavorare sul tensore senza fare subito pooling

Io eviterei completamente questo workflow:

\[
H
\rightarrow
\text{mean pooling}
\rightarrow
\text{cosine}.
\]

Andrei invece:

\[
\boxed{
H_{slrpkd}
}
\]

e definirei una serie di contrazioni.

Per esempio, la media sulle realizzazioni:

\[
\bar H_{slpkd}
=
\frac1R
\sum_r H_{slrpkd}
\]

è una riduzione controllata.

La media sulle posizioni:

\[
\bar H_{slrkd}
=
\frac1P
\sum_p H_{slrpkd}
\]

è un'altra.

Ma possiamo anche fare una cosa molto più utile: **misurare quanto ogni riduzione cambia la geometria**.

Per esempio:

\[
C_{\text{raw}}^k
\]

contro

\[
C_{\text{position-controlled}}^k
\]

contro

\[
C_{\text{paraphrase-averaged}}^k.
\]

Se il fenomeno conico rimane invariato attraverso queste tre rappresentazioni, diventa molto più difficile spiegarlo come artefatto.

---

# 7. La matrice di Gram dovrebbe diventare il vero oggetto centrale

Con tutte queste dimensioni, possiamo definire

\[
G^{k}_{slrp,s'l'r'p'}
=
H_{slrpkd}H_{s'l'r'p'kd}.
\]

Notare la contrazione:

\[
d
\]

è l'indice Einstein contratto.

Poi possiamo applicare varie maschere:

\[
M_{SS},
\quad
M_{SD},
\quad
M_{DS},
\quad
M_{DD}.
\]

E quindi

\[
C_{SS}^k
=
\operatorname{mean}
\left[
G^k\odot M_{SS}
\right].
\]

Questo è esattamente il punto in cui la tua intuizione su Einstein notation/einops diventa molto forte.

Il framework potrebbe essere quasi interamente definito come un dizionario di contrazioni.

Per esempio concettualmente:

```python
einsum("...kd,...kd->...", H, H)
```

per i prodotti scalari,

```python
einsum("slrpkd,slrpkd->slrpk", H, H)
```

per le norme,

e così via.

Non abbiamo più una collezione di script ad hoc: abbiamo una piccola algebra di osservabili tensoriali.

---

# 8. Come capire se il termine \(\epsilon\) sta davvero diminuendo

Qui farei qualcosa di molto importante.

Una volta specificato

\[
H=C+F+L+P+X+\epsilon,
\]

stimiamo

\[
\hat H=C+F+L+P+X.
\]

Poi osserviamo

\[
R_\epsilon^2(k)
=
1-
\frac{
\|H_k-\hat H_k\|_F^2
}{
\|H_k-\bar H_k\|_F^2
}.
\]

La domanda non è semplicemente «quanto spiega il modello?».

È:

\[
R_\epsilon^2(k)
\]

come funzione di \(k\)?

Potremmo scoprire, per esempio,

\[
R_\epsilon^2(k)
:
0.1\rightarrow0.3\rightarrow0.7\rightarrow0.8.
\]

Questo significherebbe che la struttura fattoriale emerge progressivamente.

Oppure potrebbe rimanere sempre piccola. In quel caso la teoria additiva è probabilmente insufficiente.

---

# 9. L'esperimento più importante: residualizzare prima di testare la conicità

Questa sarebbe una modifica fondamentale al paper.

Non farei un unico test.

Farei una gerarchia:

\[
H
\]

poi

\[
H^{(-P)}
=
H-\hat P,
\]

poi

\[
H^{(-P,-X)}
=
H-\hat P-\hat X,
\]

e infine analizzerei la geometria residua.

Quindi confrontiamo:

\[
C_{SS}^k(H),
\]

\[
C_{SS}^k(H^{(-P)}),
\]

\[
C_{SS}^k(H^{(-P,-X)}).
\]

Lo stesso per \(SD,DS,DD\).

Se la conicità appare soltanto prima della correzione, era probabilmente un artefatto.

Se rimane dopo entrambe le correzioni, è molto più interessante.

---

# 10. Una pipeline scientifica molto robusta

Io imposterei tutto il progetto in quattro passaggi.

Prima misuriamo senza assumere alcun modello:

\[
\|H\|,
\]

\[
C_{SS},C_{SD},C_{DS},C_{DD},
\]

\[
C_{\text{position}},
\]

\[
C_{\text{paraphrase}}.
\]

Poi decomponiamo:

\[
H=C+F+L+P+X+\epsilon.
\]

Poi facciamo i test di fattorialità:

\[
\Delta_{\rm int}
=
SS-SD-DS+DD.
\]

Infine confrontiamo:

\[
\alpha,\beta\in\mathbb R
\]

contro

\[
\alpha,\beta\geq0.
\]

Quest'ultimo confronto è il vero test della tua ipotesi conica.

---

# 11. E c'è un controllo che considererei indispensabile

Dovremmo costruire deliberatamente **dataset avversariali**.

Per esempio:

\[
\text{same fact / same language}
\]

ma con tokenizzazione molto diversa;

\[
\text{different fact / same lexical template};
\]

\[
\text{same fact / different language / matched length};
\]

\[
\text{random fact-language pairing}.
\]

E poi fare permutazioni:

\[
\pi_s,\qquad \pi_l,\qquad \pi_r.
\]

Il modello deve perdere la struttura prevista quando distruggiamo le corrispondenze semantiche.

Altrimenti rischiamo il classico errore:

\[
\text{regularity in representation}
\neq
\text{mechanistic semantic representation}.
\]

---

# 12. La teoria comincia ad avere una forma molto precisa

A questo punto la nostra ipotesi non è più semplicemente

\[
h=\alpha a+\beta b+\epsilon.
\]

È qualcosa di più vicino a

\[
\boxed{
H_{slrpkd}
=
\rho_{slrpk}
\,
U_{slrpkd}
}
\]

con

\[
U_{slrpk}
\approx
\alpha_s^k a_s^k
+
\beta_l^k b_l^k
+
\gamma_p^k q_p^k
+
x_{slr}^k
+
\epsilon_{slrpk},
\]

\[
\alpha_s^k,\beta_l^k,\gamma_p^k\geq0,
\]

e

\[
\rho_{slrpk}>0.
\]

Quindi abbiamo in realtà una **geometria conica radially modulated**:

\[
\boxed{
\text{residual state}
=
\text{radial magnitude}
\times
\text{point in a structured cone}
}.
\]

Questa secondo me è una formulazione teorica decisamente più interessante della semplice linear representation hypothesis.

E soprattutto produce predizioni separabili:

\[
\text{radial growth},
\]

\[
\text{position geometry},
\]

\[
\text{lexical invariance},
\]

\[
\text{fact geometry},
\]

\[
\text{language geometry},
\]

\[
\text{fact-language interaction},
\]

\[
\text{positivity/conicity}.
\]

Se una teoria supera tutti questi controlli, allora il risultato diventa difficile da liquidare come un effetto generico del residual stream.

La cosa che farei adesso è congelare questa struttura matematica prima di scrivere una riga del programma: definire esattamente gli assi di \(H\), le convenzioni di pooling, le maschere \(SS/SD/DS/DD\), le contrazioni Einstein e soprattutto una batteria di null model. Da lì possiamo tradurre quasi meccanicamente ogni equazione in `einops`/`einsum` e costruire un unico runner che applichi gli stessi test a tutti gli \(M\) modelli.

## You

Sì sono d'accordo, per favore quindi  scrivi un essay completo con questa teoria congelata a questo stato che è già sufficientemente complessa e studiando un piano per potere effettuare una serie di predizioni dalla più facile alla più complessa

1. ripetibili e riproducibili
2. statisticamente solide
3. difficili da attaccare

L'obiettivo è avere una teoria completa tensoriale delle rappresentazioni interne dei Transformers che possa sopravvivere fra più modelli cercando quindi di evidenziare se esista una sorta di tipologia di sostrato comune ai dati di cui i modelli di linguaggio sono delle approssimazioni learned.
Illustra in maniera scientifica le idee teoriche e come intendiamo metterle alla prova in una sequenza di complessità crescente di esperimenti, usando una notazione matematica chiara ed ispirata alla notazione di Einstein con contrazioni di indici multiple ed una importante batteria di modelli nulli per poter testare le nostre predizioni.
Nello sviluppare la teoria tieni sempre conto del rango dei tensori e degli indici, non assumere mai tensori come $H\_{id}$ perchè non sarebbe chiaro quali componenti del tensore originale sono state contratti, piuttosto preferisci la notazione posizionale usando il simbolo ':'.
Produci una risposta che è una versione già abbastanza vicina alla struttura concettuale di un paper: la teoria viene congelata, le grandezze sono definite prima degli esperimenti e la sequenza delle verifiche è costruita in modo che ogni livello più complesso dipenda da risultati già stabiliti ai livelli precedenti, questo è fondamentale.
Non voglio un canvas ma una risposta lunga.

## ChatGPT

Fissiamo ora un modello generativo unico e trattiamo ogni esperimento successivo come un test aggiuntivo della stessa teoria, non come una nuova ipotesi costruita a posteriori. La priorità sarà separare ciò che è strutturalmente necessario da ciò che è solo un possibile meccanismo.

## ChatGPT

La formulazione che abbiamo raggiunto è abbastanza ricca da poter essere trattata come una teoria sperimentale vera e propria. La sua forza, però, dipenderà meno dalla suggestività della geometria e molto di più dalla disciplina con cui separeremo le predizioni necessarie dagli artefatti possibili. Il programma che segue congela quindi anzitutto gli oggetti matematici e soltanto dopo introduce gli esperimenti, in ordine di complessità crescente. L'idea guida è che un singolo tensore delle rappresentazioni debba contenere tutta l'informazione necessaria, mentre ogni risultato venga ottenuto attraverso contrazioni, proiezioni, normalizzazioni e confronti esplicitamente definiti.

:::writing{variant="document" id="58321" title="Evidence of Conic Compositionality in Large Language Models"}
# Evidence of Conic Compositionality in Large Language Models

## 1. Problema scientifico

I large language models sono sistemi parametrici addestrati a predire sequenze linguistiche, ma il loro comportamento interno suggerisce che il residual stream costituisca uno spazio nel quale vengono progressivamente accumulati, trasformati e resi accessibili diversi tipi di informazione. La *Linear Representation Hypothesis* formalizza l'idea che proprietà concettuali di alto livello possano essere associate a direzioni lineari delle rappresentazioni interne; esperimenti di probing, steering e intervento causale hanno fornito evidenza a favore di questa famiglia di ipotesi.  La letteratura sulla superposition e sugli sparse autoencoders ha ulteriormente rafforzato l'idea che le rappresentazioni interne possano essere comprese come combinazioni di feature distribuite, spesso non coincidenti con singoli neuroni e potenzialmente sovrapposte nello stesso spazio. 

Questi risultati, tuttavia, lasciano aperta una domanda più fondamentale. Se il residual stream rappresenta concetti attraverso feature distribuite, qual è la struttura algebrica con cui differenti fattori semantici vengono combinati? In particolare, è possibile descrivere la rappresentazione di un contenuto espresso in una certa lingua come la composizione di componenti relativamente indipendenti associate al contenuto stesso, alla lingua, alla posizione e alla superficie lessicale?

L'ipotesi proposta in questo lavoro è che una parte rilevante della geometria interna dei Transformer possa essere descritta da una struttura additiva e, in una formulazione più forte, conica: gli stati latenti sarebbero ottenuti come combinazioni positive di direzioni-feature condivise, mentre la profondità del Transformer modificherebbe progressivamente i coefficienti, la composizione e la leggibilità di tali feature.

Il termine *conic* non deve essere inteso come una conseguenza automatica della linearità. La linearità ammette coefficienti in \(\mathbb R\), mentre una struttura conica richiede coefficienti non negativi. Questa distinzione costituisce precisamente una delle ipotesi che dovranno essere sottoposte a test.

L'obiettivo non è dunque dimostrare che esiste una particolare decomposizione scelta a posteriori, ma verificare se una stessa struttura matematica sia capace di descrivere il residual stream attraverso più fatti, più lingue, più layer e più modelli indipendenti.

## 2. Il tensore fondamentale

Consideriamo \(S\) fatti, \(L\) lingue e \(R\) diverse realizzazioni linguistiche o parafrasi di ciascun fatto in ciascuna lingua. Per ogni token position \(p\), layer \(k\) e coordinata del residual stream \(d\), definiamo

\[
H_{slrpkd},
\qquad
H\in\mathbb R^{S\times L\times R\times P\times K\times D}.
\]

Gli indici hanno significato fisso:

\[
s=\text{fatto},
\qquad
l=\text{lingua},
\qquad
r=\text{realizzazione/parafrasi},
\]

\[
p=\text{posizione del token},
\qquad
k=\text{profondità del layer},
\qquad
d=\text{coordinata del residual stream}.
\]

Per \(M\) modelli diversi avremo una collezione

\[
\{H^{(m)}\}_{m=1}^M.
\]

L'indice \(m\) resta esterno al tensore principale: in questo modo i tensori rimangono omogenei e possono essere confrontati senza confondere la dimensione del modello con quella della rappresentazione.

La notazione vettoriale

\[
H_{slrpk:}
\]

indica esplicitamente che sono state fissate le coordinate \(s,l,r,p,k\) e che resta l'asse \(d\), cioè il vettore del residual stream. Analogamente,

\[
H_{slr:kd}
\]

indica che vengono mantenuti gli assi \(k,d\) e che l'asse \(r\) è quello sul quale si intende operare una riduzione o una marginalizzazione. Questa notazione è intenzionalmente posizionale: ogni contrazione permette di ricostruire senza ambiguità da quali assi del tensore originario provenga la quantità osservata.

Il prodotto scalare tra due stati è quindi

\[
G_{slrp\,s'l'r'p'}^{k}
=
H_{slrpk:}\cdot H_{s'l'r'p':k},
\]

ovvero, in notazione di Einstein sul solo asse del residual stream,

\[
G_{slrp\,s'l'r'p'}^{k}
=
H_{slrpkd}H_{s'l'r'p'kd}.
\]

Il vantaggio concettuale è che \(G\) non è una nuova quantità primitiva: è una contrazione ben definita di \(H\). Tutta la geometria che analizzeremo è ottenuta in questo modo.

## 3. Separazione radiale e angolare

Una prima estensione essenziale della teoria consiste nel non confondere la crescita della norma con l'evoluzione della geometria.

Definiamo

\[
\rho_{slrpk}
=
\|H_{slrpk:}\|_2
\]

e

\[
U_{slrpk:}
=
\frac{H_{slrpk:}}{\rho_{slrpk}},
\qquad
\|U_{slrpk:}\|_2=1.
\]

Pertanto

\[
H_{slrpk:}
=
\rho_{slrpk}U_{slrpk:}.
\]

Questa separazione è necessaria perché il residual stream è un sistema additivo: nei Transformer pre-LayerNorm le componenti degli strati vengono sommate al residual stream e la sua norma può crescere fortemente con la profondità. Sono stati osservati pattern di crescita sistematica delle norme anche in diverse famiglie di modelli; lavori recenti discutono esplicitamente il problema della *residual-norm growth* e della conseguente riduzione relativa dell'impatto degli aggiornamenti più profondi. 

Di conseguenza, per il dot product vale

\[
G_{ij}
=
\rho_i\rho_j
\cos\theta_{ij},
\]

mentre il coseno dipende esclusivamente dalla componente angolare,

\[
C_{ij}
=
U_{i:}\cdot U_{j:}.
\]

Ogni risultato sperimentale dovrà quindi essere riportato almeno in doppia forma: sulle quantità radiali \(\rho\) e sulle quantità angolari \(C\). Un'apparente crescita della similarità nei prodotti scalari non verrà considerata evidenza di nuova struttura semantica se essa scompare dopo la normalizzazione.

La crescita della norma non verrà quindi assorbita in \(\epsilon\). È un termine strutturale osservabile indipendentemente e costituisce una dimensione della teoria.

## 4. Modello generativo del residual stream

La teoria fenomenologica congelata è

\[
\boxed{
H_{slrpk:}
=
c_{k:}
+
\alpha_s^k a_{sk:}
+
\beta_l^k b_{lk:}
+
\gamma_p^k q_{pk:}
+
x_{slrk:}
+
\epsilon_{slrpk:}.
}
\]

I termini sono:

\[
c_{k:}=\text{componente comune al layer }k,
\]

\[
\alpha_s^k a_{sk:}=\text{componente fattuale},
\]

\[
\beta_l^k b_{lk:}=\text{componente linguistica},
\]

\[
\gamma_p^k q_{pk:}=\text{componente posizionale},
\]

\[
x_{slrk:}=\text{componente di superficie/realizzazione},
\]

\[
\epsilon_{slrpk:}=\text{residuo non spiegato}.
\]

Le feature direzionali vengono normalizzate,

\[
\|a_{sk:}\|=\|b_{lk:}\|=\|q_{pk:}\|=1,
\]

e l'ipotesi conica impone

\[
\boxed{
\alpha_s^k\ge0,\qquad
\beta_l^k\ge0,\qquad
\gamma_p^k\ge0.
}
\]

Queste condizioni vanno interpretate come una ipotesi sulla direzione delle attivazioni rispetto a una famiglia di feature orientate, non come una semplice scelta convenzionale di segno.

Questo punto è metodologicamente cruciale. Se \(a_s^k\) è stimata indipendentemente per ogni singolo fatto, il segno di \(a_s^k\) può essere invertito e quello di \(\alpha_s^k\) cambiato simultaneamente senza modificare \(H\). Quindi una condizione \(\alpha_s^k\ge0\), da sola, non è una prova di conicità: è una simmetria di gauge. Per questo il test forte della conicità dovrà utilizzare una famiglia di direzioni condivise e orientate, appresa su un training set e mantenuta fissa sul test set. La conicità sarà significativa soltanto quando la positività è verificata relativamente a un dizionario condiviso e non costruito separatamente per ciascun concetto.

Questa distinzione costituisce uno dei principali falsi positivi teorici da eliminare.

## 5. Geometria fattoriale del prodotto scalare

Consideriamo ora due rappresentazioni,

\[
H_{slrpk:}
\]

e

\[
H_{s'l'r'p'k:}.
\]

Trascurando temporaneamente \(c,x,\epsilon\) e supponendo che le componenti fattuale, linguistica e posizionale siano approssimativamente ortogonali, otteniamo

\[
G_{slrp\,s'l'r'p'}^k
\approx
\alpha_s^k\alpha_{s'}^k
A_{ss'}^k
+
\beta_l^k\beta_{l'}^k
B_{ll'}^k
+
\gamma_p^k\gamma_{p'}^k
Q_{pp'}^k,
\]

dove

\[
A_{ss'}^k
=
a_{sk:}\cdot a_{s'k:},
\]

\[
B_{ll'}^k
=
b_{lk:}\cdot b_{l'k:},
\]

\[
Q_{pp'}^k
=
q_{pk:}\cdot q_{p'k:}.
\]

Questa equazione è centrale: la matrice di Gram delle rappresentazioni osservate dovrebbe essere approssimativamente la sovrapposizione di strutture associate a differenti fattori indicizzati del tensore originario.

In una versione più generale, i termini di cross-talk non vengono posti a zero ma misurati esplicitamente:

\[
A\!B
=
a_{sk:}\cdot b_{l'k:},
\qquad
A\!Q
=
a_{sk:}\cdot q_{p'k:},
\qquad
B\!Q
=
b_{lk:}\cdot q_{p'k:}.
\]

Questi termini non saranno classificati automaticamente come rumore. La loro presenza costituisce una delle quantità diagnostiche che diranno se la decomposizione è veramente separabile oppure se le feature sono intrinsecamente accoppiate.

## 6. Le quattro classi semantico-linguistiche

Dopo aver fissato \(r,p\), o dopo una marginalizzazione esplicitamente dichiarata su tali assi, definiamo quattro classi di confronto.

Per \(s=s'\) e \(l=l'\):

\[
C_{SS}^k.
\]

Per \(s=s'\) e \(l\neq l'\):

\[
C_{SD}^k.
\]

Per \(s\neq s'\) e \(l=l'\):

\[
C_{DS}^k.
\]

Per \(s\neq s'\) e \(l\neq l'\):

\[
C_{DD}^k.
\]

Nel modello ideale fattoriale, supponendo

\[
A_{ss}^k=B_{ll}^k=1,
\]

e

\[
A_{ss'}^k=\rho_F^k
\quad(s\neq s'),
\]

\[
B_{ll'}^k=\rho_L^k
\quad(l\neq l'),
\]

otteniamo, se trascuriamo per il momento la posizione,

\[
G_{SS}^k
=
(\alpha^k)^2+(\beta^k)^2,
\]

\[
G_{SD}^k
=
(\alpha^k)^2
+
(\beta^k)^2\rho_L^k,
\]

\[
G_{DS}^k
=
(\alpha^k)^2\rho_F^k
+
(\beta^k)^2,
\]

\[
G_{DD}^k
=
(\alpha^k)^2\rho_F^k
+
(\beta^k)^2\rho_L^k.
\]

Dopo normalizzazione, ponendo

\[
w_F^k
=
\frac{(\alpha^k)^2}
{(\alpha^k)^2+(\beta^k)^2},
\]

\[
w_L^k
=
\frac{(\beta^k)^2}
{(\alpha^k)^2+(\beta^k)^2},
\]

si ottiene

\[
C_{SD}^k
=
w_F^k+w_L^k\rho_L^k,
\]

\[
C_{DS}^k
=
w_F^k\rho_F^k+w_L^k,
\]

\[
C_{DD}^k
=
w_F^k\rho_F^k+w_L^k\rho_L^k.
\]

La semplice gerarchia

\[
C_{SS}>C_{SD},C_{DS}>C_{DD}
\]

costituisce solo la prima previsione. La previsione più forte riguarda le differenze fattoriali:

\[
\Delta_F^k
=
C_{SS}^k-C_{SD}^k,
\]

\[
\Delta_L^k
=
C_{SS}^k-C_{DS}^k.
\]

Esse dovrebbero isolare rispettivamente la perdita di informazione linguistica e fattuale.

Ancora più forte è il contrasto di interazione,

\[
\boxed{
\Delta_{\mathrm{int}}^k
=
C_{SS}^k-C_{SD}^k-C_{DS}^k+C_{DD}^k.
}
\]

Un modello puramente additivo prevede

\[
\Delta_{\mathrm{int}}^k\approx0.
\]

Una deviazione sistematica da zero implica che la rappresentazione di un fatto in una lingua non sia ottenibile come semplice somma delle componenti che rappresentano separatamente fatto e lingua.

Questo contrasto sarà quindi uno dei principali discrimini tra una spiegazione fattoriale e una spiegazione realmente interattiva.

## 7. Il ruolo della posizione

La posizione non viene marginalizzata in partenza.

Il tensore originale conserva infatti l'asse

\[
p.
\]

Definiamo la media empirica della posizione

\[
\bar H_{pk:}
=
\frac{1}{SLR}
\sum_{s,l,r}
H_{slrpk:}.
\]

La sua norma e le similarità tra

\[
\bar H_{pk:}
\]

e

\[
\bar H_{p'k:}
\]

forniscono una prima stima della struttura posizionale.

L'obiettivo non è dimostrare semplicemente che "la posizione conta", cosa quasi banale in un Transformer, ma stabilire se la struttura posizionale contamina i contrasti fact/language.

La quantità più utile è dunque la variazione del risultato prima e dopo residualizzazione:

\[
H^{(-P)}_{slrpk:}
=
H_{slrpk:}
-
\widehat P_{pk:}.
\]

Confrontiamo quindi

\[
C_{XY}^k(H)
\]

con

\[
C_{XY}^k(H^{(-P)})
\]

per

\[
XY\in\{SS,SD,DS,DD\}.
\]

Se la struttura fact/language permane invariata o quasi invariata, possiamo concludere che essa non è un artefatto della posizione.

Se invece scompare, la teoria iniziale deve essere modificata.

Questa è una distinzione importante: non dobbiamo "correggere" la posizione per ottenere il risultato desiderato; dobbiamo misurare quanto il risultato dipenda da essa.

## 8. La superficie lessicale e l'asse delle realizzazioni

L'asse \(r\) serve precisamente a separare contenuto e superficie.

Per lo stesso fatto e la stessa lingua, possiamo confrontare

\[
H_{slrpk:}
\]

con

\[
H_{slr'pk:},
\qquad
r\neq r'.
\]

Definiamo

\[
C_{\mathrm{para}}^k
=
\mathbb E_{s,l,r\neq r'}
\left[
U_{slrpk:}\cdot U_{slr'pk:}
\right].
\]

Se \(C_{\mathrm{para}}^k\) cresce lungo la rete, questo fornisce evidenza che le rappresentazioni divergono dalla superficie lessicale e convergono verso una rappresentazione comune del contenuto.

Il risultato opposto sarebbe altrettanto informativo: se le parafrasi rimangono geometricamente molto distanti, allora una singola feature \(a_s^k\) potrebbe essere una semplificazione eccessiva.

Non dobbiamo inoltre identificare automaticamente \(r\) con una "componente lessicale pura". È più prudente considerarlo un asse di variazione di superficie che può contenere lessico, sintassi, morfologia e differenti realizzazioni semantiche dello stesso contenuto.

Questa prudenza è importante per impedire che il modello venga reso vero dalla definizione stessa delle variabili.

## 9. Primo livello sperimentale: diagnostica senza ipotesi

La prima fase deve essere completamente descrittiva.

Per ogni modello e ogni layer si misurano:

\[
\rho_{slrpk},
\]

\[
C_{slrp\,s'l'r'p'}^k,
\]

la struttura position-position,

la struttura realization-realization,

e le quattro classi \(SS,SD,DS,DD\).

Non viene stimato alcun modello fattoriale.

Il risultato principale è una famiglia di curve

\[
k\mapsto
\bar\rho^k,
\]

e

\[
k\mapsto
C_{SS}^k,\,
C_{SD}^k,\,
C_{DS}^k,\,
C_{DD}^k.
\]

Questa fase deve essere riproducibile al 100% a partire dal tensore salvato, senza rieseguire il modello.

Essa stabilisce il fenomeno empirico prima di introdurre qualsiasi spiegazione.

## 10. Secondo livello: stabilità rispetto a posizione e realizzazione

Una volta caratterizzata la geometria grezza, si effettuano due esperimenti di robustezza.

Nel primo si residualizza \(P\).

Nel secondo si confrontano:

\[
H,
\qquad
H^{(-P)},
\qquad
\bar H^{(r)}
\]

dove

\[
\bar H^{(r)}_{slpk:}
=
\frac1R
\sum_r
H_{slrpk:}.
\]

Un risultato deve essere considerato robusto soltanto se la struttura fondamentale \(SS/SD/DS/DD\) sopravvive a queste trasformazioni in modo qualitativamente coerente.

La mediana sulle realizzazioni può essere preferibile alla media come analisi di sensibilità, ma non deve sostituire l'analisi sul tensore completo.

## 11. Terzo livello: separazione tra geometria radiale e angolare

A questo punto confrontiamo sistematicamente

\[
G^k
\]

e

\[
C^k.
\]

Un effetto che appare nel dot product ma non nel cosine è classificato come prevalentemente radiale.

Un effetto che persiste nel cosine è invece una variazione angolare.

Questa distinzione protegge il programma sperimentale da una delle spiegazioni più semplici e plausibili: che molte osservazioni siano conseguenza della residual-norm growth anziché di una riorganizzazione semantica dello spazio.

In questa fase si studia anche il rapporto tra la crescita della norma e l'evoluzione di

\[
C_{SS}^k-C_{SD}^k
\]

e

\[
C_{SS}^k-C_{DS}^k.
\]

Una vera struttura composizionale dovrebbe produrre informazione angolare che non possa essere spiegata semplicemente dalla crescita di \(\rho\).

## 12. Quarto livello: test della struttura fattoriale

Solo dopo che posizione, superficie e norma sono state caratterizzate, stimiamo il modello

\[
H=C+F+L+P+X+\epsilon.
\]

La misura principale è la varianza spiegata,

\[
R^2(k)
=
1-
\frac{
\|H_k-\widehat H_k\|_F^2
}{
\|H_k-\bar H_k\|_F^2
}.
\]

Ma non basta un singolo \(R^2\).

Dobbiamo decomporre il miglioramento predittivo in contributi gerarchici:

\[
C,
\]

\[
C+P,
\]

\[
C+P+X,
\]

\[
C+P+X+F,
\]

\[
C+P+X+F+L.
\]

Il valore dell'aggiunta di \(F\) deve essere quantificato condizionatamente a \(P\) e \(X\), e quello di \(L\) condizionatamente a tutto ciò che è stato già incluso.

Questa è la struttura annidata che rende le inferenze difficili da attaccare.

## 13. Quinto livello: test della fattorialità tramite Gram tensor

La decomposizione vettoriale non è l'unico modo di testare la teoria.

Costruiamo il Gram tensor completo

\[
G^k_{slrp\,s'l'r'p'}
=
H_{slrpk:}\cdot H_{s'l'r'p':}.
\]

Poi cerchiamo di spiegare \(G^k\) come somma di strutture indotte dagli assi originari:

\[
G^k
\approx
G_F^k+G_L^k+G_P^k+G_X^k+G_\epsilon^k.
\]

L'importanza di questa fase è concettuale: la teoria non dipende più da una particolare base del residual space.

Se la struttura esiste soltanto in una rappresentazione scelta, potrebbe essere un artefatto della parametrizzazione. Se invece sopravvive nella geometria di Gram, è una proprietà intrinseca rispetto all'inner product scelto.

Questa prospettiva è compatibile con il fatto che la nozione di rappresentazione lineare dipende anche dalla geometria metrica scelta nello spazio delle rappresentazioni; lavori formali recenti hanno sottolineato proprio il ruolo dell'inner product nella relazione tra probing, steering e linearità. 

## 14. Sesto livello: il test di interazione

A questo punto viene testato

\[
\Delta_{\mathrm{int}}^k
=
C_{SS}^k-C_{SD}^k-C_{DS}^k+C_{DD}^k.
\]

Questo contrasto è concettualmente analogo a una differenza-in-differenze.

Se

\[
\Delta_{\mathrm{int}}^k\simeq0,
\]

la geometria è coerente con un modello additivo fact + language.

Se invece

\[
\Delta_{\mathrm{int}}^k\neq0,
\]

possiamo introdurre esplicitamente

\[
I_{slk:}
\]

come componente di interazione, ottenendo

\[
H
=
C+F+L+P+X+I+\epsilon.
\]

È importante non decidere a priori quale dei due risultati sia migliore. L'assenza di interazione supporta la separabilità; la presenza di interazione rivela una struttura composizionale più complessa.

In entrambi i casi l'esperimento informa la teoria.

## 15. Settimo livello: passaggio dalla linearità alla conicità

Questo è il passaggio più delicato.

La semplice decomposizione

\[
H=C+\sum_j z_j\phi_j+\epsilon
\]

non dimostra ancora conicità.

Dobbiamo costruire un dizionario condiviso

\[
\Phi^k
=
\{\phi_1^k,\ldots,\phi_J^k\}
\]

e rappresentare

\[
H_{slrpk:}
\approx
c_{k:}
+
z_{slrpkj}\phi_{jk:}.
\]

L'ipotesi lineare ammette

\[
z_{slrpkj}\in\mathbb R.
\]

L'ipotesi conica richiede

\[
\boxed{
z_{slrpkj}\ge0.
}
\]

Il cono associato al layer è

\[
\mathcal C^k
=
\left\{
\sum_{j=1}^{J}
z_j\phi_{jk:}
:
z_j\ge0
\right\}.
\]

Il test non è quindi "possiamo scegliere un segno positivo per ogni feature?", perché ciò sarebbe sempre possibile feature per feature.

Il test è invece:

\[
\text{dato un dizionario condiviso e fissato,}
\]

la rappresentazione di molti stati indipendenti appartiene sistematicamente allo stesso cono con coefficienti non negativi?

Questa è una proprietà non banale.

Il confronto decisivo sarà tra:

\[
\widehat H_{\mathrm{linear}}
\]

e

\[
\widehat H_{\mathrm{cone}},
\]

ottenuti rispettivamente con regressione unconstrained e con una fattorizzazione non-negativa o NNLS.

Il confronto verrà eseguito fuori campione.

La metrica principale non sarà soltanto l'errore di ricostruzione ma il trade-off tra:

\[
\text{reconstruction error},
\]

\[
\text{number of active features},
\]

\[
\text{stability across resamples},
\]

\[
\text{held-out predictive accuracy}.
\]

Se il modello conico generalizza meglio o in modo più stabile pur imponendo un vincolo più forte, questa costituisce evidenza specifica a favore della conicità.

## 16. Ottavo livello: composizionalità fuori campione

Questo è probabilmente il test più importante dell'intero programma.

Supponiamo di osservare durante il fitting alcune coppie

\[
(s,l)
\]

e di riservarne altre.

Stimiamo separatamente una rappresentazione fattuale e linguistica e cerchiamo poi di predire una combinazione mai utilizzata:

\[
\widehat H_{s,l,k:}
=
c_{k:}
+
\alpha_s^k a_{sk:}
+
\beta_l^k b_{lk:}.
\]

La quantità fondamentale diventa

\[
\operatorname{cos}
\left(
\widehat H_{s,l,k:},
H_{s,l,k:}
\right).
\]

Questo risultato è molto più forte di una semplice linear probe.

La teoria afferma infatti che una combinazione nuova di componenti latenti debba produrre una configurazione prevedibile.

La struttura sperimentale ideale è un *crossed hold-out*:

\[
(s,l)\in\text{train}
\]

per molte combinazioni e

\[
(s,l)\in\text{test}
\]

per combinazioni mai viste.

Meglio ancora, alcune lingue possono essere tenute completamente fuori dal fitting delle feature linguistiche e alcuni fatti completamente fuori dal fitting delle feature fattuali.

In questo caso il modello deve composizionare componenti mai incontrate insieme.

Questa sarebbe una prova molto più difficile da spiegare attraverso semplici correlazioni superficiali.

## 17. Nono livello: universalità inter-modello

Il passo successivo è chiedere se la struttura sia specifica del modello o se rifletta qualcosa di più generale nella maniera in cui i Transformer apprendono dai dati.

Per ogni modello \(m\) misuriamo:

\[
C_{SS}^{(m)}(k),
\quad
C_{SD}^{(m)}(k),
\quad
C_{DS}^{(m)}(k),
\quad
C_{DD}^{(m)}(k),
\]

\[
\Delta_F^{(m)}(k),
\quad
\Delta_L^{(m)}(k),
\quad
\Delta_{\mathrm{int}}^{(m)}(k),
\]

oltre a

\[
\rho^{(m)}(k)
\]

e

\[
R_{\mathrm{cone}}^{2,(m)}(k).
\]

Le curve non dovranno necessariamente sovrapporsi layer per layer: modelli diversi possono avere profondità, dimensionalità, tokenizer e normalizzazioni differenti.

La domanda più forte sarà quindi se esista una trasformazione di scala o una normalizzazione della profondità sotto la quale emerga una struttura comune.

Possiamo immaginare una funzione universale

\[
\mathcal G(\kappa)
\]

con

\[
\kappa=\frac{k}{K}
\]

e testare se alcune quantità geometriche presentino forme comparabili quando la profondità viene normalizzata.

La vera ipotesi di universalità sarebbe allora

\[
\mathcal P(H^{(1)})
\simeq
\mathcal P(H^{(2)})
\simeq
\cdots
\simeq
\mathcal P(H^{(M)}),
\]

dove \(\mathcal P\) è l'insieme delle proprietà geometriche normalizzate, non il punto per punto delle coordinate.

Questo è importante perché non stiamo cercando lo stesso spazio vettoriale fra modelli. Stiamo cercando una stessa *legge di organizzazione*.

## 18. Batteria dei modelli nulli

Un risultato difficilmente attaccabile richiede null model altrettanto sistematici quanto il modello principale.

Il primo null è la permutazione delle lingue:

\[
l\rightarrow\pi(l).
\]

Il secondo è la permutazione dei fatti:

\[
s\rightarrow\pi(s).
\]

Il terzo distrugge simultaneamente la struttura fact-language.

Il quarto mantiene la lingua ma randomizza la corrispondenza tra realizzazione e fatto.

Il quinto conserva la distribuzione delle lunghezze e della tokenizzazione ma distrugge il contenuto semantico.

Il sesto utilizza fatti diversi ma appartenenti allo stesso dominio semantico, per verificare che \(DD\) non stia semplicemente misurando distanza fra topic.

Il settimo usa template sintattici identici con contenuti fattuali differenti.

L'ottavo usa fatti semanticamente equivalenti ma con lessico deliberatamente non sovrapposto.

Il nono consiste nel mescolare le posizioni dei token preservandone tutte le altre proprietà.

Il decimo è un *within-model shuffled-layer null*, ottenuto rompendo l'allineamento tra layer e osservazioni.

Il criterio non è che ogni null debba produrre zero. È che debba eliminare selettivamente la struttura che la relativa ipotesi attribuisce alla corrispondenza semantica.

In altre parole, vogliamo testare

\[
H_0:\text{la struttura osservata è spiegabile da statistiche superficiali}
\]

contro

\[
H_1:\text{la struttura dipende dalla relazione semantica fattuale-linguistica}.
\]

## 19. Statistica e inferenza

Le osservazioni non sono indipendenti.

Due parafrasi dello stesso fatto sono correlate; due fatti della stessa lingua condividono struttura; tutti i punti di un modello condividono tokenizer e pesi; tutti i layer sono correlati tra loro.

Per questo non dobbiamo trattare le migliaia di cosine come indipendenti.

L'unità primaria del bootstrap deve essere il fatto. In alcune analisi dovrà essere utilizzato un bootstrap gerarchico:

\[
s
\rightarrow
l
\rightarrow
r
\]

con eventuale livello di modello \(m\) nel confronto multi-model.

Le bande di confidenza sulle curve \(k\mapsto C(k)\) dovrebbero essere ottenute mediante bootstrap stratificato o procedure per dati longitudinali, mentre i confronti puntuali tra layer dovrebbero essere accompagnati da correzione per confronti multipli.

Molto più interessante del solo p-value sarà riportare:

\[
\Delta,
\quad
CI_{95\%},
\quad
R^2,
\quad
\text{effect size},
\]

e, dove possibile, la distribuzione completa delle quantità attraverso i fatti.

Un fenomeno che dipende da tre fatti fortissimi e migliaia di token non è equivalente a un fenomeno moderato ma sistematico su centinaia di fatti.

## 20. Riproducibilità

La matrice sperimentale deve essere congelata prima del fitting.

Per ogni modello devono essere registrati:

\[
\text{model identifier},
\]

\[
\text{revision/hash},
\]

\[
\text{tokenizer},
\]

\[
\text{prompt templates},
\]

\[
\text{facts},
\]

\[
\text{languages},
\]

\[
\text{paraphrase realizations},
\]

\[
\text{layer convention},
\]

\[
\text{token-position convention}.
\]

Il tensore \(H\) deve essere il vero artefatto scientifico primario.

Una volta salvato \(H\), tutte le figure principali e tutte le statistiche dovrebbero essere rigenerabili senza invocare nuovamente il modello.

Questo rende il lavoro auditabile e permette di separare nettamente la fase computazionalmente costosa di estrazione delle attivazioni dalla fase teorico-statistica.

Inoltre rende naturale il framework tensoriale: ogni esperimento può essere espresso come una successione di contrazioni di \(H\), e il codice può essere implementato con `einsum` ed `einops` mantenendo una corrispondenza quasi letterale fra formula matematica e programma.

## 21. La gerarchia logica delle predizioni

La struttura completa dell'articolo dovrebbe essere costruita come una catena, non come una raccolta di esperimenti indipendenti.

Il primo risultato richiesto è l'esistenza di una struttura geometrica riproducibile:

\[
C_{SS},C_{SD},C_{DS},C_{DD}.
\]

Il secondo è la sua robustezza alla posizione e alla superficie:

\[
C(H)\approx C(H^{(-P)})
\]

e stabilità rispetto alle parafrasi.

Il terzo è la distinzione tra fenomeni radiali e angolari:

\[
G \neq C.
\]

Il quarto è la presenza di una decomposizione fattoriale:

\[
H\approx C+F+L+P+X+\epsilon.
\]

Il quinto è la verifica dell'additività attraverso

\[
\Delta_{\mathrm{int}}\approx0
\]

oppure la misura controllata dell'interazione.

Il sesto è la superiorità o la stabilità di un modello conico rispetto a uno lineare.

Il settimo è la capacità di predire combinazioni held-out.

L'ottavo è la replicazione della stessa struttura attraverso modelli differenti.

Questa sequenza è importante perché evita una situazione pericolosa: arrivare a una decomposizione conica complessa senza aver prima stabilito che esista una struttura più elementare nei dati.

## 22. Cosa costituirebbe davvero evidenza di conic compositionality

Per essere precisi, non considererei sufficiente nessuno dei seguenti risultati preso isolatamente:

\[
C_{SS}>C_{DD};
\]

la presenza di linear probes;

la capacità di ricostruire le attivazioni con una matrice fattoriale;

un buon \(R^2\);

la presenza di feature sparse;

la possibilità di steering.

Tutti questi risultati sono compatibili con una vasta famiglia di modelli lineari e non lineari. La letteratura esistente mostra già che concetti e feature possono essere rappresentati linearmente o recuperati mediante decomposizioni sparse; ciò costituisce il punto di partenza, non la conclusione. 

Una vera evidenza di conic compositionality richiederebbe invece simultaneamente che:

\[
\text{(i)}
\quad
\text{esista una geometria fattoriale stabile;}
\]

\[
\text{(ii)}
\quad
\text{la struttura sopravviva ai null model superficiali;}
\]

\[
\text{(iii)}
\quad
\text{un dizionario condiviso descriva gli stati con coefficienti prevalentemente non negativi;}
\]

\[
\text{(iv)}
\quad
\text{il vincolo conico non distrugga la capacità predittiva;}
\]

\[
\text{(v)}
\quad
\text{la composizione predica configurazioni held-out;}
\]

\[
\text{(vi)}
\quad
\text{il fenomeno ricompaia in modelli indipendenti.}
\]

Aggiungerei un settimo criterio, ancora più severo: la struttura conica dovrebbe essere parzialmente causale.

Una feature identificata come componente del concetto dovrebbe poter essere amplificata o attenuata tramite intervento sul residual stream producendo una variazione comportamentale coerente con il coefficiente previsto. La letteratura di mechanistic interpretability ha già mostrato che activation steering e sparse features possono essere collegati a interventi causali; il nostro esperimento costituirebbe però un'estensione da singole feature o concetti a una legge composizionale multi-fattore. 

## 23. Dalla rappresentazione alla dinamica degli stati latenti

La formulazione finale non considera più il Transformer come un semplice encoder di testo.

Per ogni layer,

\[
H^{k+1}
=
H^k
+
\Delta H^k,
\]

e, in forma concettuale,

\[
\Delta H^k
=
\Delta C^k
+
\Delta F^k
+
\Delta L^k
+
\Delta P^k
+
\Delta X^k
+
\Delta I^k
+
\Delta\epsilon^k.
\]

Il modello diventa quindi un sistema dinamico che modifica progressivamente uno stato latente.

La domanda non è più soltanto

\[
\text{“quale informazione è presente al layer }k\text{?”}
\]

ma

\[
\text{“quali componenti vengono aggiunte, rafforzate, attenuate o ricombinate passando da }k\text{ a }k+1\text{?”}
\]

Questa prospettiva è coerente con la visione del residual stream come workspace condiviso: i layer leggono e scrivono nella stessa rappresentazione progressivamente aggiornata. Lavori recenti continuano a utilizzare proprio questa prospettiva per studiare come informazione concettuale emerga e venga trasformata attraverso la profondità. 

L'oggetto di interesse diventa allora una traiettoria

\[
H^1
\rightarrow
H^2
\rightarrow
\cdots
\rightarrow
H^K
\]

nello spazio dei tensori, oppure, dopo la decomposizione, una traiettoria nello spazio dei coefficienti

\[
z^1
\rightarrow
z^2
\rightarrow
\cdots
\rightarrow
z^K.
\]

La conicità sarebbe quindi una proprietà non solo statica ma dinamica: il modello potrebbe costruire progressivamente uno stato all'interno di un cono di feature, modificandone la composizione lungo la profondità.

## 24. La teoria più ambiziosa

La formulazione più generale cui conduce questo programma è dunque la seguente.

Esisterebbe, per ogni layer \(k\), uno spazio di feature condivise

\[
\Phi^k
=
\{\phi_1^k,\ldots,\phi_J^k\}
\]

che genera un cono

\[
\mathcal C^k
=
\operatorname{cone}
\left(
\phi_1^k,\ldots,\phi_J^k
\right).
\]

Ogni stato del residual stream avrebbe allora una parte radiale e una parte appartenente, approssimativamente, a tale struttura:

\[
H_{slrpk:}
\approx
\rho_{slrpk}
\left(
\sum_j
z_{slrpkj}\phi_{jk:}
+
\epsilon'_{slrpk:}
\right),
\]

con

\[
z_{slrpkj}\ge0.
\]

La composizione delle cause osservabili dello stato dovrebbe essere, almeno localmente,

\[
z_{slrpk:}
\approx
z^F_{s k:}
+
z^L_{l k:}
+
z^P_{p k:}
+
z^X_{slr k:}
+
z^I_{sl k:}.
\]

In questa formulazione, il concetto non è un punto primitivo dello spazio. È una configurazione di coefficienti dentro una geometria condivisa.

Il significato di una frase non risiede quindi necessariamente in un singolo vettore "semanticamente puro". Risiede nella configurazione del sistema di attivazioni rispetto a un dizionario di feature.

Questo è molto vicino alla prospettiva introdotta dalla superposition e dalla dictionary learning, ma sposta il problema da "quali sono le feature?" a una domanda più profonda: **come vengono composte le feature per costruire stati semantici e come questa composizione evolve attraverso la rete?** 

## 25. Ipotesi sul sostrato comune

Il risultato più ambizioso non sarebbe trovare una particolare feature universale condivisa da tutti i modelli.

Sarebbe trovare invarianti statistiche e geometriche.

Potrebbe esistere una struttura del tipo

\[
\mathcal H_{\mathrm{LM}}
\]

non come spazio vettoriale universale, ma come famiglia di geometrie con proprietà comuni:

\[
\mathcal G_{\mathrm{fact}},
\quad
\mathcal G_{\mathrm{lang}},
\quad
\mathcal G_{\mathrm{position}},
\quad
\mathcal G_{\mathrm{surface}},
\]

e una legge comune di composizione.

In questo senso i modelli linguistici sarebbero approssimazioni learned di un sostrato statistico preesistente nei dati: una struttura che non è scritta esplicitamente nei testi, ma che emerge perché tutti i modelli devono risolvere, con architetture differenti, lo stesso problema predittivo.

Questa ipotesi va però formulata con cautela. Non possiamo concludere dall'esistenza di geometrie comuni che esista una "algebra semantica del mondo" unica. Possiamo soltanto verificare se modelli diversi convergano verso invarianti geometrici comparabili.

Se ciò accadesse, il passo teorico successivo sarebbe chiedersi perché.

Una possibilità è che il processo di next-token prediction imponga implicitamente una geometria degli stati latenti che rifletta regolarità statistiche del processo generativo dei dati. Esistono già risultati che collegano geometrie lineari nel residual stream a strutture latenti del processo generativo e perfino a belief states con geometrie non banali. 

Il nostro programma estenderebbe questa prospettiva da una singola geometria semantica a una teoria tensoriale fattorizzata, radiale, posizionale e conica.

## 26. Criterio finale di successo

Il successo scientifico del progetto non dovrebbe essere definito come "ottenere belle curve".

La forma forte del risultato sarebbe qualcosa come:

\[
\boxed{
H
\;\longrightarrow\;
\text{factorial tensor geometry}
\;\longrightarrow\;
\text{additive latent structure}
\;\longrightarrow\;
\text{positive cone}
\;\longrightarrow\;
\text{held-out composition}
\;\longrightarrow\;
\text{cross-model invariance}.
}
\]

Ogni freccia deve essere verificata separatamente.

Se il primo passaggio fallisce, non ha senso parlare di algebra.

Se la struttura scompare dopo aver controllato posizione e superficie, abbiamo trovato un artefatto.

Se la struttura è lineare ma non positiva, abbiamo evidenza di composizionalità lineare, non conica.

Se è conica ma non generalizza fuori campione, abbiamo una decomposizione descrittiva, non una legge computazionale.

Se generalizza ma solo in un modello, abbiamo una proprietà locale dell'architettura o del training.

Solo se la catena completa sopravvive ai null model, alla cross-validation, ai cambiamenti di prompt, alle parafrasi, ai cambiamenti di lingua e a modelli indipendenti possiamo iniziare a parlare seriamente di una legge comune degli stati latenti nei Transformer.

Ed è precisamente per questo che il tensore

\[
H_{slrpkd}
\]

deve rimanere al centro di tutto il lavoro.

Non vogliamo costruire una teoria e poi produrre i dati che la confermano. Vogliamo conservare il massimo grado possibile della rappresentazione originale e definire una collezione di contrazioni che permettano di chiedere, una dopo l'altra, quali simmetrie, fattorizzazioni e vincoli siano realmente presenti.

In questo senso il programma non consiste nel cercare semplicemente "feature interpretabili". Consiste nel verificare se esista una struttura algebrica emergente nel residual stream.

La domanda ultima diventa allora:

\[
\boxed{
\text{Esiste una geometria composizionale comune che i Transformer apprendono perché è implicita nella struttura statistica del linguaggio?}
}
\]

La *conic compositionality* sarebbe una possibile risposta. Il ruolo dell'esperimento è determinare se quella risposta sopravvive quando rendiamo ogni possibile spiegazione alternativa progressivamente più difficile.
:::

Questo è, secondo me, già abbastanza vicino a un vero *research program* da congelare prima di implementare il framework. Il passaggio successivo dovrebbe essere puramente operativo: definire lo schema esatto del tensore \(H^{(m)}\), un catalogo formale delle contrazioni ammesse e una tabella preregistrata di statistiche, null model e criteri decisionali. Da lì il codice `einops`/`einsum` può essere costruito quasi direttamente dalle equazioni sopra.

## You

Questo approccio mi piace. Per favore facciamo il passaggio operativo successivo.
Come prima cosa definiamo lo schema esatto del tensore $H^{(m)}$ e le contrazioni ammesse che danno luogo ad esperimenti che possono verificare alcune delle ipotesi principali.
Ricorda di fare attenzione al fatto che nella dimensione dei token potremmo avere lunghezze diverse, quindi bisogna considerare metodi di gestirlo per esempio con degli zero-padding.

## ChatGPT

Questo è il punto in cui conviene diventare molto rigorosi, perché una buona definizione del tensore iniziale ci permette poi di trattare quasi tutto il progetto come una collezione di contrazioni determinate a priori. La scelta che farei è di non cercare di rendere subito omogenee le dimensioni interne dei modelli: \(D_m\) e \(K_m\) possono essere diverse. L'omogeneità richiesta riguarda invece gli assi semantici e le statistiche derivate.

## 1. Definizione canonica di \(H^{(m)}\)

Per il modello \(m\), definiamo

\[
\boxed{
H^{(m)}\in
\mathbb R^{S\times L\times R\times P\times K_m\times D_m}
}
\]

con gli indici

\[
s=1,\ldots,S
\]

fatto,

\[
l=1,\ldots,L
\]

lingua,

\[
r=1,\ldots,R
\]

realizzazione/parafrasi,

\[
p=1,\ldots,P
\]

posizione token,

\[
k=1,\ldots,K_m
\]

layer del modello \(m\),

\[
d=1,\ldots,D_m
\]

coordinata del residual stream.

Il singolo vettore di residual stream è sempre scritto

\[
H^{(m)}_{slrpk:}\in\mathbb R^{D_m}.
\]

Questo è importante: il simbolo `:` deve esplicitamente segnalare l'asse non contratto.

La collezione multi-modello è

\[
\left\{
H^{(m)}
\right\}_{m=1}^{M},
\]

con

\[
H^{(m)}:
(S,L,R,P,K_m,D_m).
\]

Non richiediamo quindi

\[
K_1=\cdots=K_M
\]

né

\[
D_1=\cdots=D_M.
\]

Le comparazioni inter-modello avverranno sulle quantità ridotte, non sulle coordinate raw del residual stream.

---

# 2. Il problema delle sequenze di lunghezza variabile

Questo è un punto che non va risolto con un semplice padding e poi dimenticato.

Supponiamo che la frase \((s,l,r)\) abbia lunghezza

\[
P_{slr}
\]

e definiamo

\[
P=\max_{s,l,r}P_{slr}.
\]

Costruiamo una maschera binaria

\[
M_{slrp}
=
\begin{cases}
1 & p<P_{slr}\\
0 & p\ge P_{slr}.
\end{cases}
\]

Allora il tensore memorizzato sarà

\[
H_{slrpkd}^{(m)}=
0
\qquad\text{se }M_{slrp}=0.
\]

Ma il punto cruciale è che **lo zero-padding non deve mai entrare nelle contrazioni senza una maschera**.

L'oggetto fondamentale diventa quindi in realtà la coppia

\[
\boxed{
\left(H^{(m)},M\right).
}
\]

\(H\) contiene le attivazioni padded; \(M\) determina quali posizioni hanno significato fisico.

Una riduzione media sulle posizioni non sarà quindi

\[
\frac1P H_{slrpkd},
\]

ma

\[
\boxed{
\bar H_{slrkd}
=
\frac{
M_{slrp}H_{slrpkd}
}{
M_{slrp}
}
}
\]

dove il numeratore e denominatore implicano la contrazione su \(p\). Scritta correttamente in Einstein:

\[
\bar H_{slrkd}
=
\frac{
M_{slrp}H_{slrpkd}
}{
M_{slrp}
}.
\]

Il \(p\) ripetuto è contratto, mentre \(s,l,r,k,d\) rimangono liberi.

Questa piccola regola evita una grande quantità di errori.

---

# 3. Un problema ancora più importante del padding

Il padding risolve soltanto la dimensione computazionale. Non risolve il problema dell'**allineamento semantico delle posizioni**.

Ad esempio:

> Paris is the capital of France

e

> The capital city of France is Paris

hanno numeri di token e posizioni differenti.

Quindi

\[
p=5
\]

non significa necessariamente la stessa cosa nei due prompt.

Per questo non farei un'unica rappresentazione "token-level". Terrei tre livelli distinti.

### Token-level

Conserviamo

\[
H_{slrpk:}.
\]

Questo permette di studiare esplicitamente la struttura posizionale.

### Anchor-level

Definiamo uno o più token semanticamente comparabili, per esempio ultimo token, EOS, oppure un token target.

Indichiamo l'anchor con

\[
p^\star_{slr}.
\]

Otteniamo

\[
H^{\star}_{slrk:}
=
H_{slr p^\star_{slr} k:}.
\]

### Span-level

Per un insieme di token appartenenti a una regione \(A_{slr}\), definiamo una maschera

\[
W_{slrp}
\]

e

\[
H^{A}_{slrk:}
=
\frac{
W_{slrp}H_{slrpkd}
}{
W_{slrp}
}.
\]

Questa terna ci permette di distinguere:

\[
\text{position geometry}
\]

da

\[
\text{content geometry}.
\]

Non vorrei mai che il risultato principale dipendesse esclusivamente da una media su tutti i token.

---

# 4. Il tensore delle norme

La prima contrazione ammessa è la norma.

Da

\[
H_{slrpkd}
\]

otteniamo

\[
\boxed{
N_{slrpk}^{2}
=
H_{slrpkd}H_{slrpkd}.
}
\]

Il rango passa da 6 a 5:

\[
(S,L,R,P,K,D)
\rightarrow
(S,L,R,P,K).
\]

La norma è

\[
N_{slrpk}
=
\sqrt{
H_{slrpkd}H_{slrpkd}
}.
\]

Questa è una quantità diagnostica primaria e deve essere sempre conservata.

---

# 5. Separazione radiale-angolare

Definiamo

\[
U_{slrpkd}
=
\frac{H_{slrpkd}}
{\sqrt{H_{slrpkd}H_{slrpkd}}}.
\]

Quindi

\[
\boxed{
U\in
\mathbb R^{S\times L\times R\times P\times K\times D}
}
\]

e

\[
U_{slrpkd}U_{slrpkd}=1.
\]

Da qui in poi avremo due famiglie sperimentali:

\[
N
\]

per la geometria radiale e

\[
U
\]

per quella angolare.

Questo deve essere fatto prima di qualsiasi test di similarità.

---

# 6. La contrazione fondamentale: Gram tensor

Il prodotto scalare fra due rappresentazioni token-level è

\[
\boxed{
G_{slrp\,s'l'r'p'}^{k}
=
H_{slrpkd}H_{s'l'r'p'kd}.
}
\]

Il rango è

\[
(S,L,R,P,S,L,R,P,K).
\]

In un'implementazione tensoriale è probabilmente più leggibile rappresentarlo come

\[
G_{sls'l'rr'pp'k},
\]

ma concettualmente bisogna ricordare che contiene **otto assi di stato più il layer**.

Questa scelta è deliberata: non voglio scrivere qualcosa come \(G_{ij}\), perché perderemmo l'informazione su come \(i,j\) sono stati costruiti.

La corrispondente versione angolare è

\[
\boxed{
C_{slrp\,s'l'r'p'}^{k}
=
U_{slrpkd}U_{s'l'r'p'kd}.
}
\]

Questa è la quantità primaria per i test \(SS,SD,DS,DD\).

---

# 7. Contrazioni con maschera

Ogni aggregazione su \(p\) deve portare la propria maschera.

Per esempio, una media token-level:

\[
\boxed{
\bar U_{slrk:}
=
\frac{
M_{slrp}U_{slrpk:}
}{
M_{slrp}
}.
}
\]

Se vogliamo confrontare due sequenze usando soltanto posizioni valide comuni, possiamo definire

\[
M^{\cap}_{slr\,s'l'r'\,pp'}
=
M_{slrp}M_{s'l'r'p'}.
\]

In pratica avremo una famiglia di maschere diverse a seconda dell'esperimento.

Questo è un principio che fisserei definitivamente:

> ogni riduzione su un asse padded deve avere una maschera esplicita associata.

Mai "zero padding + mean".

Sempre

\[
\text{masked reduction}.
\]

---

# 8. Riduzione sulla realizzazione \(r\)

La prima riduzione semantica è

\[
\boxed{
\bar H_{slpkd}
=
\frac1R H_{slrpkd}
}
\]

dove l'indice \(r\) è contratto.

In Einstein, implicitamente,

\[
\bar H_{slpkd}
=
\frac1R
\sum_rH_{slrpkd}.
\]

Il rango passa da

\[
(S,L,R,P,K,D)
\]

a

\[
(S,L,P,K,D).
\]

Questa quantità non deve sostituire \(H\): è una riduzione derivata.

Permette di chiedere se il fatto è stabile rispetto alla superficie linguistica.

Una statistica fondamentale sarà quindi la variabilità intra-fatto:

\[
V_{\mathrm{surf}}^k
=
\mathbb E_{s,l,r\neq r'}
\left[
1-
U_{slr p k:}
\cdot
U_{slr' p' k:}
\right]
\]

con una scelta esplicita della gestione delle posizioni.

---

# 9. Riduzione sulla posizione

Solo dopo aver conservato il livello token possiamo definire

\[
\boxed{
\bar H_{slrkd}
=
\frac{
M_{slrp}H_{slrpkd}
}{
M_{slrp}
}.
}
\]

Il rango diventa

\[
(S,L,R,K,D).
\]

Lo stesso vale per \(U\).

Ma proporrei di non fare di questa rappresentazione la rappresentazione primaria del paper.

Il paper dovrebbe conservare:

\[
H_{\text{token}}
\]

e presentare

\[
\bar H_{\text{position}}
\]

come una specifica marginalizzazione.

---

# 10. Medie di gruppo

Possiamo definire operatori di marginalizzazione generali.

Per esempio, media sul fatto:

\[
\mu_{l r p k d}
=
\frac1S
H_{slrpkd}.
\]

Media sulla lingua:

\[
\mu_{s r p k d}
=
\frac1L
H_{slrpkd}.
\]

Media sulla realizzazione:

\[
\mu_{s l p k d}
=
\frac1R
H_{slrpkd}.
\]

Media sul fatto e sulla realizzazione:

\[
\mu_{lpkd}
=
\frac1{SR}
H_{slrpkd}.
\]

Queste marginali sono importanti perché rendono possibile isolare empiricamente:

\[
F,\quad L,\quad X,\quad P.
\]

---

# 11. Contrazione della componente position

Definiamo la media sulla parte non-posizionale:

\[
\boxed{
Q_{pkd}
=
\frac1{SLR}
H_{slrpkd}.
}
\]

Il risultato ha rango

\[
(P,K,D).
\]

Possiamo quindi misurare direttamente

\[
Q_{pk:}\cdot Q_{p'k:}
=
Q_{pkd}Q_{p'kd}.
\]

Questo produce

\[
Q_{pp'}^k.
\]

Se troviamo forti strutture diagonali o smoothness in \(p-p'\), abbiamo evidenza che la posizione produce una sottostruttura non trascurabile.

---

# 12. Contrazione della componente linguistica

Analogamente,

\[
\boxed{
B_{lkd}
=
\frac1{SRP}
\sum_{s,r,p}
H_{slrpkd}
}
\]

con la dovuta mascheratura su \(p\).

Questa quantità ha rango

\[
(L,K,D).
\]

La relativa Gram matrix è

\[
B_{ll'}^k
=
B_{lkd}B_{l'kd}.
\]

Attenzione però: questa è una **stima marginale** della struttura linguistica, non una prova che esista una feature linguistica pura.

Per trasformarla in tale, dovremo dimostrare che la struttura rimane stabile controllando gli altri assi.

---

# 13. Contrazione della componente fattuale

Analogamente:

\[
\boxed{
A_{skd}
=
\frac1{LRP}
\sum_{l,r,p}
H_{slrpkd}.
}
\]

Poi

\[
A_{ss'}^k
=
A_{skd}A_{s'kd}.
\]

Questa è la quantità naturale per testare se esista una geometria associata al contenuto fattuale.

---

# 14. Le quattro maschere fondamentali

Ora possiamo definire formalmente le quattro relazioni.

Nel tensore delle coppie di stati, definiamo

\[
\mathbb 1_{SS}(s,l,s',l')
=
\mathbf 1[s=s']\mathbf 1[l=l'],
\]

\[
\mathbb 1_{SD}
=
\mathbf 1[s=s']\mathbf 1[l\neq l'],
\]

\[
\mathbb 1_{DS}
=
\mathbf 1[s\neq s']\mathbf 1[l=l'],
\]

\[
\mathbb 1_{DD}
=
\mathbf 1[s\neq s']\mathbf 1[l\neq l'].
\]

Queste maschere hanno rango

\[
(S,L,S,L).
\]

Le statistiche diventano contrazioni della matrice di similarità con queste maschere.

Ad esempio,

\[
\boxed{
C_{SS}^k
=
\frac{
\mathbb 1_{SS}^{sls'l'}
C_{sl\,s'l'}^k
}{
\mathbb 1_{SS}^{sls'l'}
}.
}
\]

La notazione rende completamente espliciti gli assi che rimangono.

Per le implementazioni useremo naturalmente `einsum` e `einops`, ma la specifica matematica deve venire prima.

---

# 15. La versione corretta delle quattro curve con l'asse \(r\)

Non dovremmo avere un solo \(SS\).

Possiamo distinguere almeno:

\[
SS_{r=r'}
\]

e

\[
SS_{r\neq r'}.
\]

Questo secondo caso è quello veramente informativo per la stabilità semantica.

Inoltre possiamo avere:

\[
SD_{r=r'},
\qquad
SD_{r\neq r'},
\]

e analogamente per \(DS,DD\).

Quindi la statistica primaria diventa in realtà una famiglia

\[
C_{XY,\; RR'}^k
\]

dove

\[
XY\in\{SS,SD,DS,DD\}.
\]

Questa distinzione permetterà di rispondere a una domanda importante:

> il modello distingue la lingua indipendentemente dalla superficie lessicale, oppure stiamo semplicemente misurando overlap lessicale?

---

# 16. Una contrazione ancora più utile: same/different a livello di token

Possiamo costruire una statistica che conservi l'asse \(p\) invece di marginalizzarlo.

Per esempio:

\[
C_{SS,pp'}^k
=
\mathbb E_{s,l,r,r'}
\left[
U_{slrpk:}U_{slr'p'k:}
\right].
\]

Questa quantità ha rango

\[
(P,P,K).
\]

Otteniamo quindi una matrice

\[
P\times P
\]

per ogni layer.

Questo potrebbe rivelare una struttura molto interessante:

\[
C_{SS,pp'}^k
\]

potrebbe essere alta sulla diagonale, lungo bande, oppure mostrare allineamenti posizione-dipendenti.

La stessa analisi si può fare per

\[
SD,\quad DS,\quad DD.
\]

È un modo molto pulito per determinare empiricamente se la posizione sia un contaminante da controllare o una parte genuina della struttura interna.

---

# 17. Dot product versus cosine

Per ogni classe \(XY\), dobbiamo mantenere entrambe:

\[
G_{XY}^k
\]

e

\[
C_{XY}^k.
\]

Formalmente,

\[
G_{XY}^k
=
\mathbb E
\left[
H_{slrpkd}H_{s'l'r'p'kd}
\mid XY
\right],
\]

mentre

\[
C_{XY}^k
=
\mathbb E
\left[
U_{slrpkd}U_{s'l'r'p'kd}
\mid XY
\right].
\]

Questo ci permette di separare due ipotesi:

\[
H_1:
\text{la struttura è radiale}
\]

e

\[
H_2:
\text{la struttura è angolare}.
\]

Non devono essere confuse.

---

# 18. Un contratto tensoriale per il modello additivo

Possiamo ora formalizzare la teoria con una specifica decomposizione:

\[
H_{slrpkd}
=
C_{kd}
+
F_{skd}
+
L_{lkd}
+
P_{pkd}
+
X_{slrkd}
+
I_{slkd}
+
E_{slrpkd}.
\]

Ho introdotto esplicitamente \(I\), l'interazione fact-language.

Questo è utile perché possiamo confrontare:

\[
H_0:
C+F+L+P+X+\epsilon
\]

contro

\[
H_1:
C+F+L+P+X+I+\epsilon.
\]

Non dobbiamo decidere in anticipo che \(I=0\).

La teoria additiva è una ipotesi falsificabile.

---

# 19. Contrazioni necessarie per la decomposizione

Per ciascun termine definiamo la marginalizzazione prevista.

Per il comune:

\[
C_{kd}
=
\frac1{SLRP}
H_{slrpkd}.
\]

Per la posizione:

\[
P_{pkd}
=
\frac1{SLR}
H_{slrpkd}
-
C_{kd}.
\]

Per la lingua:

\[
L_{lkd}
=
\frac1{SRP}
H_{slrpkd}
-
C_{kd}.
\]

Per il fatto:

\[
F_{skd}
=
\frac1{LRP}
H_{slrpkd}
-
C_{kd}.
\]

Queste formule sono una prima ANOVA tensoriale. Non sono ancora una decomposizione identificata perfettamente, perché gli effetti possono essere non ortogonali. Ma costituiscono un baseline deterministico e riproducibile.

Successivamente potremo introdurre proiezioni ortogonali o regressioni tensoriali per migliorare l'identificazione.

---

# 20. Il residuo \(\epsilon\)

Una volta definiti i termini previsti,

\[
\widehat H_{slrpkd}
=
C_{kd}
+
F_{skd}
+
L_{lkd}
+
P_{pkd}
+
X_{slrkd},
\]

definiamo

\[
\boxed{
E_{slrpkd}
=
H_{slrpkd}
-
\widehat H_{slrpkd}.
}
\]

Il punto metodologico è fondamentale:

\[
\epsilon
\]

non viene definito semplicemente come "tutto quello che non ci piace".

È precisamente il residuo di un modello congelato.

Possiamo allora misurare

\[
R_\epsilon^2(k)
=
1-
\frac{
E_{slrpkd}E_{slrpkd}
}{
(H_{slrpkd}-C_{kd})
(H_{slrpkd}-C_{kd})
}.
\]

Qui la contrazione è esplicita anche nel denominatore.

---

# 21. Il primo catalogo di esperimenti

Con questa struttura possiamo congelare una prima batteria di esperimenti.

### Esperimento A — Norm growth

Contrazione:

\[
N_{slrpk}^2
=
H_{slrpkd}H_{slrpkd}.
\]

Domanda:

\[
\mathbb E[N^k]
\]

cresce sistematicamente con \(k\)?

---

### Esperimento B — Position geometry

Contrazione:

\[
Q_{pkd}
=
\frac1{SLR}H_{slrpkd},
\]

seguita da

\[
Q_{pp'}^k
=
Q_{pkd}Q_{p'kd}.
\]

Domanda:

la posizione produce una struttura geometrica sistematica?

---

### Esperimento C — Paraphrase invariance

Contrazione:

\[
C_{\mathrm{para}}^k
=
\mathbb E_{r\neq r'}
[
U_{slrpkd}U_{slr'pkd}
].
\]

Domanda:

la rappresentazione del fatto converge verso un'identità indipendente dalla superficie?

---

### Esperimento D — Fact/language geometry

Calcoliamo

\[
C_{SS}^k,\;
C_{SD}^k,\;
C_{DS}^k,\;
C_{DD}^k.
\]

Domanda:

compare la struttura prevista?

---

### Esperimento E — Factorial interaction

Calcoliamo

\[
\Delta_{\mathrm{int}}^k
=
C_{SS}^k-C_{SD}^k-C_{DS}^k+C_{DD}^k.
\]

Domanda:

la geometria è additiva oppure esiste interazione fact-language?

---

### Esperimento F — Position-controlled geometry

Sostituiamo \(H\) con

\[
H^{(-P)}
=
H-\widehat P.
\]

Ripetiamo D ed E.

Domanda:

la struttura fact/language sopravvive alla rimozione della componente posizionale?

---

### Esperimento G — Surface-controlled geometry

Analogamente:

\[
H^{(-P,-X)}
=
H-\widehat P-\widehat X.
\]

Ripetiamo D ed E.

Domanda:

la struttura sopravvive al controllo della superficie linguistica?

---

### Esperimento H — Linear versus conic reconstruction

Definiamo

\[
H_{slrpk:}
\approx
\sum_j z_{slrpkj}\phi_{jk:}.
\]

Confrontiamo

\[
z_j\in\mathbb R
\]

contro

\[
z_j\ge0.
\]

Domanda:

la positività produce una rappresentazione altrettanto buona o migliore fuori campione?

---

### Esperimento I — Held-out composition

Stimiamo

\[
F_s,\quad L_l
\]

separatamente e costruiamo

\[
\widehat H_{sl}
=
C+F_s+L_l.
\]

Poi valutiamo

\[
\cos(
\widehat H_{sl:},
H_{sl:}
).
\]

Domanda:

una combinazione non osservata è prevedibile dalla composizione delle sue componenti?

Questo è il vero test di composizionalità.

---

# 22. Come implementerei tutto con `einops` + `einsum`

La scelta architetturale del codice dovrebbe riflettere la teoria.

Il tensore principale potrebbe essere rappresentato come:

```python
H.shape
# (S, L, R, P, K, D)
```

con

```python
mask.shape
# (S, L, R, P)
```

Per la norma:

```python
norm2 = einsum(
    H, H,
    "s l r p k d, s l r p k d -> s l r p k",
)
```

Per la normalizzazione:

```python
U = H / norm[..., None]
```

Per il Gram token-level:

```python
G = einsum(
    U, U,
    "s l r p k d, t j q u k d -> "
    "s l r p t j q u k",
)
```

Qui ho volutamente usato indici diversi per i due stati:

\[
(s,l,r,p)
\]

e

\[
(t,j,q,u).
\]

Il risultato è

\[
(S,L,R,P,S,L,R,P,K).
\]

Questo è molto più sicuro di comprimere arbitrariamente i primi quattro assi in un indice \(i\), perché mantiene la provenienza semantica di ogni asse.

`einops` può poi occuparsi delle riorganizzazioni necessarie, ad esempio trasformando temporaneamente:

```text
(s l r p) -> pair
```

quando questo rende una certa operazione più efficiente, purché l'informazione sulla mappatura venga conservata.

---

# 23. Una questione importante: costo computazionale

Il Gram tensor completo

\[
G_{slrp\,s'l'r'p'}^k
\]

può diventare enorme.

Se

\[
S=500,\quad L=8,\quad R=4,\quad P=30,
\]

abbiamo già

\[
SLRP=480000
\]

stati token-level.

Una matrice Gram quadratica completa è impraticabile.

Quindi distinguerei tra:

\[
\text{mathematical observable}
\]

e

\[
\text{computational estimator}.
\]

Matematicamente definiamo il Gram completo.

Computazionalmente non lo materializziamo necessariamente.

Per esempio, \(C_{SS}\) può essere stimato direttamente tramite una contrazione sulle coppie appartenenti alla maschera \(SS\), senza costruire tutto \(G\).

Questa distinzione sarà fondamentale per scalare il framework.

---

# 24. Il tensore multi-modello

Per il modello \(m\):

\[
H^{(m)}
\in
\mathbb R^{S\times L\times R\times P\times K_m\times D_m}.
\]

Le statistiche derivate avranno normalmente forma:

\[
C^{(m)}_{XY,k},
\]

\[
N^{(m)}_k,
\]

\[
R^{2,(m)}_k,
\]

ecc.

Per confrontare i modelli, definiamo una profondità normalizzata

\[
\kappa
=
\frac{k}{K_m-1}.
\]

Quindi possiamo confrontare

\[
f_m(\kappa)
\]

anziché \(f_m(k)\).

Questo evita di fingere che il layer 12 di un modello da 24 layer equivalga necessariamente al layer 12 di un modello da 48.

---

# 25. Un'ulteriore scelta che congelerei subito: due versioni di \(H\)

Per ogni modello salverei due oggetti.

Il primo:

\[
H^{\mathrm{raw}}
\]

con residual stream esattamente come prodotto dal modello.

Il secondo:

\[
H^{\mathrm{norm}}
\]

ottenuto soltanto tramite

\[
U_{slrpkd}
=
H_{slrpkd}/\|H_{slrpk:}\|.
\]

Non sovrascriverei mai \(H^{\mathrm{raw}}\).

Questo ci permetterà di verificare continuamente se un fenomeno appartiene alla struttura radiale oppure angolare.

---

# 26. Il formato sperimentale completo

Il dataset tensoriale per un singolo modello dovrebbe quindi essere concettualmente:

\[
\boxed{
\mathcal D^{(m)}
=
\left(
H^{(m)},
M,
A,
T,
\Theta^{(m)}
\right)
}
\]

dove:

\[
H^{(m)}:
(S,L,R,P,K_m,D_m),
\]

\[
M:
(S,L,R,P)
\]

è la valid-mask,

\[
A:
(S,L,R,P)
\]

può contenere l'anchor/span alignment,

\[
T
\]

contiene la metadata table per fatti, lingue e realizzazioni,

e

\[
\Theta^{(m)}
\]

contiene metadati del modello, tokenizer, revision, numero di layer, dimensionalità ecc.

La cosa importante è che i risultati quantitativi debbano poter essere ottenuti esclusivamente da

\[
(H,M,A,T),
\]

senza ambiguità.

---

# 27. La regola metodologica fondamentale

A questo punto fisserei una regola per l'intero progetto:

\[
\boxed{
\text{nessuna contrazione non preregistrata viene introdotta dopo aver visto i risultati.}
}
\]

Possiamo naturalmente sviluppare nuovi esperimenti esplorativi, ma devono essere marcati come tali.

Per il corpo principale del paper, invece, definiamo ora un piccolo linguaggio di operatori:

\[
\mathcal M_p,\quad
\mathcal M_r,\quad
\mathcal M_s,\quad
\mathcal M_l,
\]

per le marginalizzazioni;

\[
\mathcal N
\]

per la normalizzazione;

\[
\mathcal G
\]

per la contrazione di Gram;

\[
\mathcal C_{SS},
\mathcal C_{SD},
\mathcal C_{DS},
\mathcal C_{DD}
\]

per le quattro maschere;

\[
\mathcal R_P,\quad
\mathcal R_X
\]

per le residualizzazioni;

e successivamente

\[
\mathcal F_{\mathrm{linear}},
\qquad
\mathcal F_{\mathrm{cone}}
\]

per le due fattorizzazioni.

Questa diventa di fatto una piccola algebra sperimentale sul tensore \(H\).

---

# 28. La questione dello zero-padding in una frase

La decisione che prenderei definitivamente è:

\[
\boxed{
\text{zero-padding sì per storage e batching, mai senza mask nelle riduzioni.}
}
\]

Inoltre non userei il semplice mean pooling sui token come rappresentazione scientifica primaria. Terrei separati:

\[
\text{token-level},
\]

\[
\text{anchor-level},
\]

\[
\text{span-level},
\]

e considererei il mean pooling soltanto come una marginalizzazione specifica del tensore.

Questo ci protegge contemporaneamente da due problemi: l'introduzione artificiale degli zeri e la perdita della struttura posizionale.

---

Il passo successivo, secondo me, può ora essere molto concreto: costruire una **specifica matematica completa delle contrazioni in forma tabellare**, con per ogni operatore gli assi di input, gli assi contratti, gli assi risultanti, la formula Einstein e la corrispondente espressione `torch.einsum`/`einops`. Quella tabella può diventare direttamente il contratto dell'implementazione sperimentale.

## You

Mi sento d'accordo su tutti i punti che hai notato.
Io ho definito questo primo dataset sperimentale per 4 fatti, 4 lingue e 4 ripetizioni.
Nel json qui di seguito trovi la gerarchia S,L,R relativi a [fatto: str][lingua: str][perifrasi :int]

L'asse collegato al numero di token della frase $P$ è generato dal tokenizer, mentre quello legato alla dimensione latente $D_m$ e del numero di layer $K_m$ del modello saranno fissati dato un particolare modello. A riguardo fisserei Llama3.1-8B come modello iniziale per fare esperimenti.

```json
{
  "history": {
    "en": {
      "1": "Julius Caesar was murdered in 44 BC.",
      "2": "In 44 BC, Julius Caesar met his death by assassination.",
      "3": "The assassination of Julius Caesar occurred in 44 BC.",
      "4": "Julius Caesar lost his life to assassins in 44 BC."
    },
    "fr": {
      "1": "Jules César a été tué en 44 av. J.-C.",
      "2": "En 44 av. J.-C., Jules César a trouvé la mort par assassinat.",
      "3": "L'assassinat de Jules César a eu lieu en 44 av. J.-C.",
      "4": "Jules César a péri sous les coups des assassins en 44 av. J.-C."
    },
    "de": {
      "1": "Julius Cäsar wurde 44 v. Chr. getötet.",
      "2": "Im Jahr 44 v. Chr. kam Julius Cäsar durch einen Mord ums Leben.",
      "3": "Die Ermordung von Julius Cäsar fand 44 v. Chr. statt.",
      "4": "Im Jahre 44 v. Chr. fiel Julius Cäsar einem Attentat zum Opfer."
    },
    "it": {
      "1": "Giulio Cesare fu ucciso nel 44 a.C.",
      "2": "Nel 44 a.C. Giulio Cesare perse la vita a causa di un assassinio.",
      "3": "L'assassinio di Giulio Cesare avvenne nel 44 a.C.",
      "4": "Nel quarantaquattro a.C. Giulio Cesare fu vittima di un attentato mortale."
    }
  },
  "math": {
    "en": {
      "1": "A prime number possesses exactly two unique divisors.",
      "2": "Only two separate divisors belong to any prime number.",
      "3": "Prime numbers are divisible by precisely two distinct numbers.",
      "4": "Exactly two unique factors exist for every prime number."
    },
    "fr": {
      "1": "Un nombre premier ne possède que deux diviseurs uniques.",
      "2": "Seuls deux diviseurs différents caractérisent les nombres premiers.",
      "3": "Les nombres premiers sont divisibles par exactement deux nombres distincts.",
      "4": "Il n'existe que deux facteurs uniques pour chaque nombre premier."
    },
    "de": {
      "1": "Eine Primzahl besitzt genau zwei unterschiedliche Teiler.",
      "2": "Nur zwei verschiedene Faktoren teilen eine Primzahl.",
      "3": "Primzahlen sind durch genau zwei verschiedene Zahlen teilbar.",
      "4": "Jede Primzahl weist exakt zwei einzigartige Teiler auf."
    },
    "it": {
      "1": "Un numero primo possiede esattamente due divisori unici.",
      "2": "Soltanto due fattori differenti caratterizzano i numeri primi.",
      "3": "I numeri primi sono divisibili per precisione da due numeri distinti.",
      "4": "Esistono esattamente due divisori univoci per ogni numero primo."
    }
  },
  "biology": {
    "en": {
      "1": "Mitochondria act as the cellular energy sources.",
      "2": "The cell relies on mitochondria as its power plants.",
      "3": "Mitochondria function as the energy centers of cells.",
      "4": "Cellular energy generation is driven by mitochondria."
    },
    "fr": {
      "1": "Les mitochondries servent de sources d'énergie cellulaire.",
      "2": "La cellule utilise les mitochondries comme centrales électriques.",
      "3": "Les mitochondries fonctionnent comme des foyers énergétiques cellulaires.",
      "4": "La production d'énergie de la cellule est assurée par les mitochondries."
    },
    "de": {
      "1": "Mitochondrien dienen als zelluläre Energiequellen.",
      "2": "Die Zelle nutzt Mitochondrien als ihre Kraftstationen.",
      "3": "Mitochondrien funktionieren als die Energiezentren der Zellen.",
      "4": "Die Energieerzeugung in der Zelle wird von Mitochondrien angetrieben."
    },
    "it": {
      "1": "I mitocondri fungono da fonti di energia cellulare.",
      "2": "La cellula si affida ai mitocondri come sue centrali elettriche.",
      "3": "I mitocondri operano come i centri energetici della cellula.",
      "4": "La generazione di energia cellulare è guidata dai mitocondri."
    }
  },
  "cooking": {
    "en": {
      "1": "Beat the egg whites until they hold firm peaks.",
      "2": "Whip the egg whites until stiff peaks appear.",
      "3": "Keep whisking the egg whites until firm peaks develop.",
      "4": "Stir the egg whites with a whisk until stiff peaks are achieved."
    },
    "fr": {
      "1": "Battez les blancs d'œufs jusqu'à ce qu'ils forment des pics fermes.",
      "2": "Remuez les blancs d'œufs au fouet jusqu'à apparition de pointes rigides.",
      "3": "Continuez de fouetter les blancs d'œufs jusqu'à l'obtention de sommets fermes.",
      "4": "Travaillez les blancs d'œufs au fouet jusqu'à ce qu'ils soient fermes en pointe."
    },
    "de": {
      "1": "Schlagen Sie das Eiweiß, bis es feste Spitzen bekommt.",
      "2": "Bearbeiten Sie das Eiweiß mit dem Schneebesen, bis feste Spitzen entstehen.",
      "3": "Rühren Sie das Eiweiß, bis stabile Spitzen sich formen.",
      "4": "Schlagen Sie das Eiweiß so lange, bis sich feste Spitzen zeigen."
    },
    "it": {
      "1": "Sbattete gli albumi finché non diventano a neve ben ferma.",
      "2": "Lavorate gli albumi con la frusta fino a ottenere una consistenza a neve ferma.",
      "3": "Montate a neve gli albumi fino a formare punte solide.",
      "4": "Sbattete gli albumi finché non si formano picchi consistenti."
    }
  }
}
```

Ora puoi procedere con la matematica completa delle contrazioni come hai correttamente fatto notare, così da avvicinarci verso l'implementazione sperimentale.
Continua scrivendo nella chat e non in un canvas.

## ChatGPT

Il dataset che hai costruito è già sufficiente per definire una prima versione completamente operativa della teoria. Con \(S=L=R=4\), abbiamo infatti 64 realizzazioni linguistiche dello stesso spazio fattoriale, ma con una variabile fondamentale — la lunghezza tokenizzata — che non è fissa. Conviene quindi congelare subito una rappresentazione che conservi integralmente la struttura token-level e introdurre le rappresentazioni aggregate soltanto come contrazioni derivate.

# Specifica tensoriale del primo esperimento

Per Llama 3.1 8B definiamo

\[
S=L=R=4
\]

e

\[
H_{slrpkd}
\in
\mathbb R^{4\times4\times4\times P\times K\times D}.
\]

Qui \(P\) è

\[
P=\max_{s,l,r} P_{slr},
\]

dove \(P_{slr}\) è il numero effettivo di token prodotto dal tokenizer per la realizzazione \((s,l,r)\).

Per Llama 3.1 8B, \(K\) e \(D\) sono fissati dall'architettura; non è necessario che la teoria assuma valori numerici, purché la convenzione su cosa chiamiamo layer \(k\) sia congelata. Io adotterei:

\[
k=0
\]

per lo stato immediatamente successivo agli embedding e

\[
k=1,\ldots,K
\]

per gli output dei transformer blocks. Questa convenzione è importante perché evita che "layer 0" significhi cose diverse in implementazioni diverse.

Il vettore elementare sarà sempre

\[
\boxed{
H_{slrpk:}
}
\]

e ha rango 1, dimensione \(D\).

L'intero oggetto scientifico è quindi

\[
\boxed{
H\in\mathbb R^{4\times4\times4\times P\times K\times D}.
}
\]

Non lo ridurremo mai prima di avere ottenuto una copia persistente dell'originale.

---

# La maschera di validità

Perché \(P\) è il massimo delle lunghezze, dobbiamo accompagnare \(H\) con

\[
M_{slrp}
=
\begin{cases}
1,&p<P_{slr}\\
0,&p\ge P_{slr}.
\end{cases}
\]

Quindi

\[
\boxed{
M\in\{0,1\}^{4\times4\times4\times P}.
}
\]

Il dato sperimentale completo è dunque

\[
\boxed{(H,M)}
\]

e non semplicemente \(H\).

Il padding è una convenzione di storage, non una parte della rappresentazione.

In particolare,

\[
H_{slrpkd}=0
\]

nelle posizioni padded, ma nessuna operazione statistica potrà utilizzare tali zeri senza moltiplicazione per \(M\).

Questo principio va incorporato direttamente nell'API sperimentale: ogni funzione che riduce \(p\) deve ricevere `mask`.

---

# Tre livelli di rappresentazione

Dal medesimo \(H\) definirei tre rappresentazioni derivate.

La prima è token-level:

\[
H_{slrpk:}.
\]

La seconda è una rappresentazione *anchor-level*. Definiamo

\[
p^\star_{slr}
=
\max\{p:M_{slrp}=1\},
\]

cioè l'ultimo token non padded, e poniamo

\[
\boxed{
A_{slrk:}
=
H_{slr p^\star_{slr} k:}.
}
\]

\(A\) ha forma

\[
(4,4,4,K,D).
\]

Questo è il primo rappresentante naturale per un confronto semantico aggregato.

La terza è una rappresentazione mean-pooled ma esplicitamente mascherata:

\[
\boxed{
\bar H_{slrk:}
=
\frac{
\sum_p M_{slrp}H_{slrpk:}
}{
\sum_pM_{slrp}
}.
}
\]

In notazione di Einstein:

\[
\bar H_{slrkd}
=
\frac{
M_{slrp}H_{slrpkd}
}{
M_{slrp}
}.
\]

Qui \(p\) è contratto.

Questa rappresentazione non sarà il dato fondamentale, ma una marginalizzazione del dato fondamentale.

---

# Una cautela fondamentale sull'anchor

L'anchor \(p^\star\) non deve essere interpretato come "la rappresentazione del fatto". È semplicemente una scelta di osservazione.

Per una generazione del testo, l'ultimo token può avere un ruolo particolare: EOS, punteggiatura, morfologia, sintassi finale e informazione richiesta per la continuazione possono contaminarlo.

Per questo il piano sperimentale dovrebbe mantenere tre analisi parallele:

\[
H_{\text{token}},
\qquad
A_{\text{last-token}},
\qquad
\bar H_{\text{masked-mean}}.
\]

Un risultato che appare soltanto in una delle tre non dovrebbe essere considerato una proprietà robusta del residual stream.

---

# Norma: prima contrazione fondamentale

Dal tensore originale definiamo

\[
N^2_{slrpk}
=
H_{slrpkd}H_{slrpkd}.
\]

La contrazione elimina \(d\), quindi

\[
N^2
\in
\mathbb R^{4\times4\times4\times P\times K}.
\]

La norma è

\[
N_{slrpk}
=
\sqrt{H_{slrpkd}H_{slrpkd}}.
\]

Per evitare che il padding introduca un falso zero, la quantità viene utilizzata soltanto dove

\[
M_{slrp}=1.
\]

La prima curva che produrremo sarà quindi

\[
\bar N(k)
=
\frac{
M_{slrp}N_{slrpk}
}{
M_{slrp}
}.
\]

Questa è la curva di *norm growth* globale.

Ma avremo anche

\[
\bar N_s(k),\qquad
\bar N_l(k),\qquad
\bar N_r(k),
\]

per vedere se la norma dipenda sistematicamente da fatto, lingua o realizzazione.

---

# Separazione radiale e angolare

Definiamo

\[
U_{slrpkd}
=
\frac{H_{slrpkd}}
{N_{slrpk}}.
\]

Quindi

\[
U_{slrpkd}U_{slrpkd}=1.
\]

Questo produce il tensore angolare

\[
U\in
\mathbb R^{4\times4\times4\times P\times K\times D}.
\]

Da questo momento ogni esperimento geometrico dovrà essere eseguito almeno in due versioni:

\[
H\quad\text{raw}
\]

e

\[
U\quad\text{normalized}.
\]

È una salvaguardia contro la possibilità che un effetto apparentemente semantico sia semplicemente prodotto dalla crescita della norma con \(k\).

---

# Gram tensor token-level

Il prodotto scalare tra due stati è

\[
G^{k}_{slrp\,s'l'r'p'}
=
H_{slrpkd}
H_{s'l'r'p'kd}.
\]

Il rango è quindi

\[
(4,4,4,P,4,4,4,P,K).
\]

Non scriveremo \(G_{ij}\), perché perderemmo l'informazione sulla provenienza di \(i\) e \(j\).

In implementazione, per evitare collisioni di nomi useremo ad esempio

\[
(s,l,r,p)
\]

e

\[
(t,j,q,u)
\]

per la seconda rappresentazione:

```python
G = torch.einsum(
    "slrpkd,tjqukd->slrptjquk",
    H,
    H,
)
```

La stessa contrazione sull'unit-normalized tensor produce

\[
\boxed{
C^{k}_{slrp\,tjq u}
=
U_{slrpkd}U_{tjqukd}.
}
\]

Poiché i due vettori hanno norma 1,

\[
C
\]

è precisamente il cosine.

---

# Non materializzare il Gram tensor completo

Matematicamente lo definiamo.

Computazionalmente, per il dataset \(4\times4\times4\), si può anche materializzare senza problemi particolari; ma il framework generale non dovrebbe dipendere da questa possibilità.

Quando aumenteremo \(S,L,R,P\), il numero di coppie cresce quadraticamente:

\[
(SLRP)^2.
\]

Quindi la versione definitiva delle contrazioni lavorerà con maschere di coppia senza necessariamente materializzare \(G\).

Questo ci permette di mantenere invariata la teoria indipendentemente dalla scala.

---

# Le quattro classi SS, SD, DS, DD

Consideriamo due stati

\[
(s,l,r,p)
\]

e

\[
(t,j,q,u).
\]

Definiamo le quattro maschere:

\[
M_{SS}^{slptju}
=
\mathbf1[s=t]\mathbf1[l=j],
\]

\[
M_{SD}^{slptju}
=
\mathbf1[s=t]\mathbf1[l\neq j],
\]

\[
M_{DS}^{slptju}
=
\mathbf1[s\neq t]\mathbf1[l=j],
\]

\[
M_{DD}^{slptju}
=
\mathbf1[s\neq t]\mathbf1[l\neq j].
\]

Qui la scelta deliberata dei simboli \(t,j,q,u\) ci ricorda che stiamo creando una seconda copia degli assi fact/language/realization/position.

La statistica SS token-level, ad esempio, è

\[
\boxed{
C_{SS}^{k}
=
\frac{
M_{slrp}M_{t j q u}
M_{SS}^{slptj u}
C^k_{slrp\,tjqu}
}{
M_{slrp}M_{t j q u}
M_{SS}^{slptju}
}.
}
\]

La notazione diventa un po' pesante, ma è proprio ciò che vogliamo: ogni indice contratto è visibile.

In pratica, per il primo esperimento consiglierei di partire dalla rappresentazione anchor \(A\), molto più compatta.

---

# Gram dell'anchor

Definiamo

\[
A_{slrk d}
=
H_{slr p^\star_{slr} k d}.
\]

La normalizzazione è

\[
V_{slrkd}
=
\frac{A_{slrkd}}
{
\sqrt{A_{slrkd}A_{slrkd}}
}.
\]

Il Gram è

\[
\boxed{
G^A_{slr\,tjq\,k}
=
A_{slrkd}A_{tj qkd}.
}
\]

e il cosine

\[
\boxed{
C^A_{slr\,tjq\,k}
=
V_{slrkd}V_{tjqkd}.
}
\]

Questo ha rango

\[
(4,4,4,4,4,4,K).
\]

Quindi soltanto

\[
4^6K
\]

elementi, totalmente gestibili.

È il punto di partenza ideale.

---

# Il primo esperimento reale: SS, SD, DS, DD all'anchor

Ora definiamo direttamente

\[
C^A_{SS}(k),
\qquad
C^A_{SD}(k),
\qquad
C^A_{DS}(k),
\qquad
C^A_{DD}(k).
\]

Ma dobbiamo decidere come trattare \(r\).

La prima versione scientificamente corretta dovrebbe produrre due famiglie.

Per \(r=q\):

\[
C^A_{XY,\mathrm{sameR}}(k).
\]

Per \(r\neq q\):

\[
C^A_{XY,\mathrm{diffR}}(k).
\]

Questo produce una struttura

\[
4\times2\times K.
\]

Perciò vedremo, ad esempio,

\[
SS_{\mathrm{sameR}}
\]

e

\[
SS_{\mathrm{diffR}}.
\]

La seconda è la quantità più interessante: stesso fatto e stessa lingua, ma parafrasi differenti.

---

# Contrazione di stabilità alle parafrasi

Per lo stesso fatto e la stessa lingua, definiamo

\[
\boxed{
C_{\mathrm{surf}}^k
=
\frac{
\mathbf 1[s=t]\mathbf1[l=j]\mathbf1[r\neq q]
V_{slrkd}V_{tj qkd}
}{
\mathbf1[s=t]\mathbf1[l=j]\mathbf1[r\neq q]
}.
}
\]

Questa curva risponde a:

> quanto la rappresentazione rimane invariata cambiando soltanto la realizzazione linguistica?

È un test preliminare necessario alla nozione di \(a_s^k\).

Se questa quantità non è maggiore del fondo \(DD\), la nostra nozione di fact feature è già problematica.

---

# Fact prototype

Definiamo ora una prima stima della componente fattuale:

\[
\boxed{
F_{skd}
=
\frac1{LR}
H_{slr p^\star_{slr} k d}.
}
\]

Quindi

\[
F\in\mathbb R^{4\times K\times D}.
\]

La corrispondente similarità fattuale è

\[
\boxed{
G^F_{st k}
=
F_{skd}F_{tkd}.
}
\]

e

\[
C^F_{st k}
=
\frac{
F_{skd}F_{tkd}
}{
\sqrt{F_{skd}F_{skd}}
\sqrt{F_{tkd}F_{tkd}}
}.
\]

Questa non è ancora \(a_s^k\). È semplicemente una stima marginale della geometria fact.

---

# Language prototype

Analogamente,

\[
\boxed{
L_{lkd}
=
\frac1{SR}
H_{slr p^\star_{slr} k d}.
}
\]

e

\[
G^L_{jk}
=
L_{lkd}L_{jkd}.
\]

Questa quantità misura la struttura media associata alla lingua.

Il confronto interessante è quindi

\[
C^F_{st}(k)
\]

contro

\[
C^L_{lj}(k).
\]

Vedremo empiricamente quale struttura è più forte a ciascun layer.

---

# Una quantità ancora più utile: marginal variance

Le similarità non raccontano tutta la storia.

Definiamo

\[
\mu_{kd}
=
\frac1{SLR}
H_{slr p^\star_{slr}kd}.
\]

Poi la varianza totale nel residual space è

\[
V_{\mathrm{tot}}^k
=
(H-\mu)(H-\mu).
\]

Più precisamente:

\[
\boxed{
V_{\mathrm{tot}}^k
=
\frac1{SLR}
\sum_{slr}
(H_{slr: k:}-\mu_{k:})
(H_{slr: k:}-\mu_{k:}).
}
\]

A questo possiamo confrontare una varianza tra fatti, una tra lingue e una entro-parafrasi.

Per esempio:

\[
V_F^k
=
\frac1S
(F_{sk:}-\mu_{k:})(F_{sk:}-\mu_{k:}),
\]

e analogamente

\[
V_L^k,
\qquad
V_R^k.
\]

Questo ci dà una prima versione quantitativa di:

\[
\text{quanto della geometria varia per fatto?}
\]

\[
\text{quanto per lingua?}
\]

\[
\text{quanto per superficie?}
\]

È probabilmente più robusto di dedurlo esclusivamente dai quattro cosine.

---

# Contrasto factoriale

Dal quartetto

\[
C_{SS},C_{SD},C_{DS},C_{DD}
\]

definiamo

\[
\boxed{
\Delta_F
=
C_{SS}-C_{SD}
}
\]

e

\[
\boxed{
\Delta_L
=
C_{SS}-C_{DS}.
}
\]

Con la convenzione che abbiamo stabilito, \(\Delta_F\) misura la perdita di similarità quando cambiamo lingua a fatto fisso, mentre \(\Delta_L\) misura la perdita quando cambiamo fatto a lingua fissa.

Più precisamente:

\[
C_{SS}-C_{SD}
\]

isola la componente associata alla lingua, mentre

\[
C_{SS}-C_{DS}
\]

isola la componente associata al fatto.

Per evitare ambiguità, nei plot userei nomi espliciti come

\[
\Delta_{\mathrm{language}}
\]

e

\[
\Delta_{\mathrm{fact}}.
\]

---

# Interaction contrast

Definiamo

\[
\boxed{
\Delta_{\mathrm{int}}^k
=
C_{SS}^k-C_{SD}^k-C_{DS}^k+C_{DD}^k.
}
\]

Questa quantità deve essere calcolata già nel primo esperimento.

Non perché ci aspettiamo necessariamente zero, ma perché costituisce il ponte tra il modello additivo e la teoria più generale.

Con un modello perfettamente additivo:

\[
\Delta_{\mathrm{int}}^k=0.
\]

Con un'interazione fact-language:

\[
\Delta_{\mathrm{int}}^k\neq0.
\]

Quindi una singola curva contiene già una predizione molto più raffinata della semplice gerarchia

\[
SS>SD,DS>DD.
\]

---

# Il controllo del modello nullo \(DD\)

Con soltanto quattro fatti e quattro lingue abbiamo un problema statistico: \(DD\) non è un vero "rumore bianco".

Per ciascuna coppia di fatti diversi e lingue diverse possiamo infatti avere:

\[
\text{same domain},
\]

\[
\text{similar syntax},
\]

\[
\text{shared vocabulary}.
\]

Per esempio *history* e *math* sono molto differenti, ma *history* e *biology* possono condividere più struttura linguistica e grammaticale del previsto.

Perciò dobbiamo avere almeno due baseline.

La prima è

\[
DD_{\mathrm{natural}},
\]

ottenuta dai dati originali.

La seconda è

\[
DD_{\mathrm{perm}},
\]

ottenuta rompendo casualmente l'associazione fatto-lingua mentre preserviamo le marginali.

Questo è un punto molto importante: il modello nullo deve preservare quanto più possibile le statistiche non rilevanti.

---

# Permutation null

Una permutazione \(\pi\) sulle lingue produce

\[
H^\pi_{slrpkd}
=
H_{s,\pi(l),r,p,k,d}.
\]

Ma questa definizione va usata con cautela perché il parametro \(r\) appartiene alla realizzazione della lingua originale.

La procedura più pulita è randomizzare soltanto le etichette semantiche nell'associazione utilizzata dalle maschere statistiche, lasciando intatti i tensori.

Quindi non modifichiamo \(H\).

Modifichiamo

\[
M_{SS},
M_{SD},
M_{DS},
M_{DD}.
\]

Questo produce una famiglia

\[
\Delta_{\mathrm{int}}^{k,\pi}.
\]

Possiamo quindi ottenere

\[
p_k
=
\Pr_\pi
\left(
|\Delta_{\mathrm{int}}^{k,\pi}|
\ge
|\Delta_{\mathrm{int}}^k|
\right).
\]

Questo è un permutation test molto più pulito.

---

# Position geometry: non la eliminiamo

Una volta ottenuta la prima geometria anchor, torniamo al tensore completo.

Definiamo

\[
Q_{pkd}
=
\frac1{SLR}
M_{slrp}H_{slrpkd}.
\]

Più correttamente con normalizzazione della quantità di osservazioni:

\[
\boxed{
Q_{pkd}
=
\frac{
M_{slrp}H_{slrpkd}
}{
M_{slrp}
}.
}
\]

Resta

\[
Q\in\mathbb R^{P\times K\times D}.
\]

La geometria posizionale è

\[
\boxed{
G^P_{pp'k}
=
Q_{pkd}Q_{p'kd}.
}
\]

Quindi la sua struttura è

\[
(P,P,K).
\]

Questa quantità ci dirà se esiste una vera struttura sistematica della posizione.

---

# Position-controlled residual

Una prima residualizzazione è

\[
H^{(-P)}_{slrpkd}
=
H_{slrpkd}
-
Q_{pkd}.
\]

Ma questa formula può essere troppo aggressiva: \(Q\) include anche informazione semantica media aggregata attraverso i fatti e le lingue.

Per il primo framework userei quindi una decomposizione centrata:

\[
Q_{pkd}^{\mathrm{center}}
=
\frac1{SLR}
H_{slrpkd}
-
\frac1P
H_{slrpkd},
\]

con le relative maschere.

In altre parole, separiamo una struttura che è specificamente associata alla posizione da una componente media globale.

Questo problema di identificabilità dovrà essere formalizzato meglio nella seconda versione del metodo; per ora il test descrittivo \(G^P\) è sufficiente.

---

# Un esperimento token-level particolarmente importante

Possiamo calcolare

\[
C_{SS,pp'}^k
\]

mantenendo le posizioni libere.

Per esempio:

\[
\boxed{
C_{SS,pp'}^k
=
\frac{
\sum_{slrr'} 
U_{slrpkd}
U_{slr' p'kd}
}{
\text{numero di coppie valide}
}.
}
\]

con \(r\neq r'\) per studiare la stabilità semantica alle parafrasi.

Il risultato è una matrice

\[
P\times P
\]

per ogni layer.

Quindi avremo

\[
C_{SS}^1(p,p'),
C_{SS}^2(p,p'),
\ldots,
C_{SS}^K(p,p').
\]

Questo può essere visualizzato come una mappa di calore per layer.

Potremmo trovare, per esempio:

\[
p\approx p'
\]

con alta similarità, oppure bande diagonali, oppure una struttura completamente diversa.

Sarebbe una prima evidenza diretta che la posizione è una componente del sistema e non semplicemente una nuisance variable.

---

# Gestione delle lunghezze diverse

Qui farei una distinzione netta fra tre confronti.

Il primo è *same absolute position*:

\[
p=p'.
\]

È il più facile da calcolare ma il meno semanticamente pulito.

Il secondo è *relative position*:

\[
\tau_{slrp}
=
\frac{p}{P_{slr}-1}.
\]

Quindi possiamo confrontare token che occupano una posizione relativa simile nella frase.

Il terzo è anchor/span alignment.

Per il primo studio userei il primo e il terzo, e conserverei il secondo come analisi di robustezza.

Non cercherei ancora un allineamento sofisticato tra parole semanticamente corrispondenti. Questo introdurrebbe un ulteriore modello nella pipeline e renderebbe meno pulita la causalità delle conclusioni.

---

# La struttura completa delle statistiche iniziali

Per ciascun layer \(k\), il runner del primo esperimento dovrebbe produrre almeno:

\[
\bar N(k),
\]

\[
C_{SS}(k),
\quad
C_{SD}(k),
\quad
C_{DS}(k),
\quad
C_{DD}(k),
\]

\[
\Delta_{\mathrm{fact}}(k),
\]

\[
\Delta_{\mathrm{language}}(k),
\]

\[
\Delta_{\mathrm{int}}(k),
\]

\[
C_{\mathrm{surface}}(k),
\]

\[
C^P_{pp'}(k),
\]

\[
V_F(k),
\quad
V_L(k),
\quad
V_R(k).
\]

Queste sono tutte contrazioni o riduzioni di \(H\) e \(M\). Non c'è ancora alcun fitting di feature.

Questo punto è importante: **il primo livello della teoria deve poter essere testato senza fare machine learning aggiuntivo.**

---

# La traduzione concettuale in `einops`

Per il dataset iniziale, adotterei precisamente il seguente schema:

```python
H:     "s l r p k d"
mask:  "s l r p"
```

e manterrei queste convenzioni in tutto il codice.

Norm:

```python
norm2 = einsum(
    H, H,
    "s l r p k d, s l r p k d -> s l r p k",
)
```

Normalized residual:

```python
U = H / norm.clamp_min(eps)[..., None]
```

Anchor extraction, concettualmente:

```python
A = ...
# "s l r k d"
```

Anchor Gram:

```python
G = einsum(
    U_anchor, U_anchor,
    "s l r k d, t j q k d -> s l r t j q k",
)
```

Position prototype:

```python
Q = einsum(
    mask, H,
    "s l r p, s l r p k d -> k p d",
)
```

con la normalizzazione per il numero di elementi validi.

Non è necessario che queste stringhe siano quelle definitive: la cosa importante è che **gli assi del codice rimangano isomorfi agli indici della teoria**.

---

# Il catalogo delle contrazioni

A questo punto possiamo formalizzare il primo piccolo "linguaggio tensoriale" del progetto.

Il tensore primitivo è

\[
H_{slrpkd}.
\]

Le contrazioni primitive sono:

\[
\mathcal N:
H_{slrpkd}H_{slrpkd}
\rightarrow
N_{slrpk},
\]

\[
\mathcal G:
H_{slrpkd}H_{tjqukd}
\rightarrow
G_{slrptjquk},
\]

\[
\mathcal U:
H_{slrpkd}\rightarrow U_{slrpkd},
\]

\[
\mathcal M_p:
H_{slrpkd}\rightarrow\bar H_{slrkd},
\]

\[
\mathcal M_r:
H_{slrpkd}\rightarrow\bar H_{slpkd},
\]

\[
\mathcal M_s:
H_{slrpkd}\rightarrow\bar H_{lrpkd},
\]

\[
\mathcal M_l:
H_{slrpkd}\rightarrow\bar H_{srpkd}.
\]

A queste aggiungiamo le maschere relazionali:

\[
\mathcal C_{SS},
\mathcal C_{SD},
\mathcal C_{DS},
\mathcal C_{DD}.
\]

Tutto ciò che faremo nel primo esperimento dovrebbe essere composizione di questi operatori.

---

# Una conseguenza importante della dimensione 4×4×4

Il dataset è piccolo:

\[
S=L=R=4.
\]

Questo non è un limite nella fase geometrica esplorativa, ma è un limite statistico importante.

Per esempio abbiamo soltanto

\[
4\cdot\binom42=24
\]

coppie ordinatamente interpretabili per "same fact, different language" se consideriamo le lingue non ordinate.

E soltanto

\[
\binom42=6
\]

coppie di fatti diversi.

Quindi non dobbiamo presentare la prima versione come una verifica definitiva della teoria.

La interpreterei come:

\[
\boxed{
\text{proof-of-concept geometry}
}
\]

con un dataset progettato per verificare che il pipeline matematico funzioni.

La seconda fase dovrebbe aumentare \(S\) molto più di quanto aumenti \(L\) e \(R\).

Per esempio:

\[
S\gg 4,\qquad L=4\text{--}8,\qquad R\geq4.
\]

La moltiplicazione sul numero di fatti aumenta enormemente la precisione delle stime fact-related.

---

# Ordine esatto delle analisi sul dataset attuale

Con i 64 testi che hai fornito, io non passerei ancora alla decomposizione conica.

L'ordine sarebbe:

\[
H
\rightarrow
\text{norm growth}
\]

poi

\[
H
\rightarrow
U
\rightarrow
SS/SD/DS/DD,
\]

poi

\[
SS
\rightarrow
r=r'
\quad\text{vs}\quad
r\neq r',
\]

poi

\[
H
\rightarrow
Q
\rightarrow
\text{position geometry},
\]

poi

\[
SS/SD/DS/DD
\rightarrow
\Delta_{\mathrm{int}},
\]

poi permutation nulls.

Soltanto se questa geometria appare robusta passerei a

\[
H
\rightarrow
C+F+L+P+X+\epsilon
\]

e infine alla domanda veramente difficile:

\[
H
\approx
\sum_jz_j\phi_j,
\qquad
z_j\ge0.
\]

In questo modo la conicità non viene mai usata per spiegare un fenomeno che non abbiamo ancora dimostrato esistere.

---

## Un punto che modificherei rispetto alla formulazione precedente

Con il dataset concreto che hai ora, introdurrei anche un indice di modello già nella nomenclatura complessiva:

\[
\boxed{
H^{(m)}_{slrpkd}
}
\]

ma **non** lo inserirei nel tensore Llama-specifico.

Per Llama 3.1 8B abbiamo semplicemente

\[
H^{(\mathrm{Llama3.1-8B})}_{slrpkd}.
\]

Quando arriveranno altri modelli, ogni modello produrrà la propria serie di statistiche

\[
C_{XY}^{(m)}(k),
\]

senza mai forzare \(K_m\) o \(D_m\) ad essere uguali.

La teoria comune vive nelle quantità invarianti derivate da \(H\), non nelle coordinate del residual stream.

Questo, a mio giudizio, è il punto metodologico più importante da congelare adesso: **\(H\) è il dato osservato; \(M\) ne definisce il dominio valido; tutte le altre quantità sono osservabili derivati ottenuti attraverso contrazioni dichiarate.**

Da qui il passo successivo può essere molto preciso: costruire una specifica `ExperimentSpec` in Python che descriva ciascuna contrazione con `input_axes`, `contracted_axes`, `output_axes`, `mask_axes`, formula Einstein e implementazione `torch.einsum`, e poi implementare per Llama 3.1 8B il primo runner che produca automaticamente le curve \(N,SS,SD,DS,DD,\Delta_{\mathrm{fact}},\Delta_{\mathrm{language}},\Delta_{\mathrm{int}}\) e i permutation null.

## You

per favore scrivi in una tabella minimale a tre colonne:
ipotesi, esperimento, contrazioni di H sia matematicamente che tramite einops (oppure einsum) con codice minimale

così posso verificare anche io

## ChatGPT

| Ipotesi | Esperimento | Contrazioni di \(H\) — matematica e codice minimale |
|---|---|---|
| **Norm growth** | Misurare se la norma del residual stream cresce sistematicamente con il layer \(k\). | \(\displaystyle N_{slrpk}^2=H_{slrpkd}H_{slrpkd}\) → `norm2 = torch.einsum("slrpkd,slrpkd->slrpk", H, H)` |
| **Separazione radiale/angolare** | Verificare se i pattern di similarità persistono dopo normalizzazione della norma. | \(\displaystyle U_{slrpkd}=\frac{H_{slrpkd}}{\sqrt{H_{slrpkd d}H_{slrpkd}}}\) → `U = H / torch.linalg.vector_norm(H, dim=-1, keepdim=True).clamp_min(1e-8)` |
| **Position geometry** | Verificare se la posizione \(p\) produce una direzione sistematica nel residual stream. | \(\displaystyle Q_{pkd}=\frac{M_{slrp}H_{slrpkd}}{M_{slrp}}\) → `Q = torch.einsum("slrp,slrpkd->pkd", M, H) / M.sum(dim=(0,1,2))[:,None,None]` |
| **Paraphrase invariance** | Stesso fatto, stessa lingua, realizzazioni \(r\neq r'\): verificare convergenza semantica nonostante variazione superficiale. | \(\displaystyle C_{\rm para}^k=\mathbb E_{s,l,r\neq r'}[U_{slrpkd}U_{slr' p'kd}]\) → `G = torch.einsum("slrpkd,slqtkd->slrpqt k", U, U)` *(poi maschera \(r\neq q\))* |
| **Fact geometry** | Verificare se rappresentazioni dello stesso fatto sono più simili di quelle di fatti diversi. | \(\displaystyle F_{skd}=\frac{1}{LRP}\sum_{l,r,p}M_{slrp}H_{slrpkd}\) → `F = torch.einsum("slrp,slrpkd->skd", M, H) / M.sum(dim=(1,2,3))[...,None,None]` |
| **Language geometry** | Verificare se una lingua induce una struttura geometrica stabile attraverso fatti e parafrasi. | \(\displaystyle L_{lkd}=\frac{1}{SRP}\sum_{s,r,p}M_{slrp}H_{slrpkd}\) → `L = torch.einsum("slrp,slrpkd->lkd", M, H) / M.sum(dim=(0,2,3))[None,...,None]` |
| **SS / SD / DS / DD** | Confrontare cosine fra stesso fatto/stessa lingua, stesso fatto/lingua diversa, fatto diverso/stessa lingua, entrambi diversi. | \(\displaystyle C_{sl,tj}^k=U_{slkd}U_{tjkd}\). Le quattro statistiche sono medie di \(C\) con maschere \(\mathbf1[s=t]\mathbf1[l=j]\), \(\mathbf1[s=t]\mathbf1[l\neq j]\), ecc. → `C = torch.einsum("slkd,tjkd->sltjk", U0, U1)` |
| **Fact-vs-language contribution** | Misurare quale fattore domina la similarità. | \(\displaystyle \Delta_{\rm language}^k=C_{SS}^k-C_{SD}^k,\quad \Delta_{\rm fact}^k=C_{SS}^k-C_{DS}^k\) → `delta_lang = C_SS - C_SD; delta_fact = C_SS - C_DS` |
| **Additive factorial structure** | Testare se la geometria fact/language è approssimativamente additiva. | \(\displaystyle \Delta_{\rm int}^k=C_{SS}^k-C_{SD}^k-C_{DS}^k+C_{DD}^k\) → `delta_int = C_SS - C_SD - C_DS + C_DD` |
| **Position control** | Verificare se \(SS/SD/DS/DD\) persistono dopo aver rimosso la componente posizionale stimata. | \(\displaystyle H^{(-P)}_{slrpkd}=H_{slrpkd}-\widehat P_{pkd}\) → `H_res = H - P[None,None,None,:,:,:]`, poi si ripetono le contrazioni \(C_{SS},\ldots,C_{DD}\). |
| **Surface/lexical control** | Verificare se la struttura fact/language sopravvive a variazioni di parafrasi. | \(\displaystyle \bar H_{slpkd}=\frac{M_{slrp}H_{slrpkd}}{M_{slrp}}\) → `H_rmean = torch.einsum("slrp,slrpkd->slpkd", M, H) / M.sum(dim=2)[...,None,None]` |
| **Gram factorization** | Verificare se la matrice di Gram osservata può essere spiegata da componenti fact + language + position + surface. | \(\displaystyle G_{slrp,tjqu}^k=H_{slrpkd}H_{tjqukd}\) → `G = torch.einsum("slrpkd,tjqukd->slrptjquk", H, H)` |
| **Linear representation** | Verificare se gli stati possono essere ricostruiti da un dizionario condiviso di feature. | \(\displaystyle H_{slrpkd}\approx z_{slrpkj}\phi_{jkd}\) → `H_hat = torch.einsum("slrpkj,jkd->slrpkd", Z, Phi)` |
| **Conic representation** | Confrontare rappresentazione lineare \(z_j\in\mathbb R\) con rappresentazione conica \(z_j\ge0\). | Stessa contrazione: \(\displaystyle \hat H_{slrpkd}=Z_{slrpkj}\Phi_{jkd}\), ma imporre \(Z\ge0\). → `H_hat = torch.einsum("slrpkj,jkd->slrpkd", Z.clamp_min(0), Phi)` |
| **Compositionality** | Stimare componenti fact e language separatamente e predire combinazioni \((s,l)\) held-out. | \(\displaystyle \hat H_{slkd}=F_{skd}+L_{lkd}+C_{kd}\) → `H_hat = F[:,None,:,:] + L[None,:,:,:] + C[None,None,:,:]` |
| **Cross-model invariance** | Ripetere tutte le statistiche per \(m=1,\ldots,M\) e confrontare le curve normalizzando la profondità. | \(\displaystyle C_{XY}^{(m)}(k)\), \(\kappa=k/(K_m-1)\) → stesso pipeline su ciascun `H_m`, senza richiedere \(D_m=D_{m'}\) o \(K_m=K_{m'}\). |

Una correzione importante al codice sopra: per le operazioni con \(P\), `M` va sempre contratta come maschera booleana/float e il denominatore va calcolato con gli stessi assi e nello stesso ordine; eviterei di fissare ora le singole righe di broadcasting come API definitiva. La specifica matematica è invece quella da congelare.

Per il dataset iniziale, partirei concretamente da cinque curve per layer:

\[
\bar N(k),\quad
C_{SS}(k),\quad
C_{SD}(k),\quad
C_{DS}(k),\quad
C_{DD}(k),
\]

e immediatamente dopo dalle tre derivate

\[
\Delta_{\rm fact}(k),\quad
\Delta_{\rm language}(k),\quad
\Delta_{\rm int}(k).
\]

Queste sono sufficienti per verificare il primo livello della teoria prima di introdurre qualunque fattorizzazione o vincolo conico.

## You

Calcoliamo la prima curva $\bar{N}(k)$ a partire da H ed M che ho disponibili ora per 32 fatti, 4 lingue e 10 ripetizioni.
Mostrami il codice einsum e dimmi come plottare la curva

## ChatGPT

Per il calcolo concreto mi manca però il file che contiene `H` e `M`: in questa conversazione non risulta allegato né disponibile tra i file correnti. Non voglio inventarne la struttura.

Assumendo comunque la convenzione che abbiamo congelato,

\[
H\in\mathbb R^{S\times L\times R\times P\times K\times D},
\qquad
M\in\{0,1\}^{S\times L\times R\times P},
\]

con

\[
S=32,\qquad L=4,\qquad R=10,
\]

la prima curva è molto semplice.

### 1. Norma token-level

Per ogni elemento:

\[
N_{slrpk}
=
\sqrt{
H_{slrpkd}H_{slrpkd}
}.
\]

Con `einsum`:

```python
import torch

norm2 = torch.einsum(
    "slrpkd,slrpkd->slrpk",
    H,
    H,
)

N = torch.sqrt(norm2.clamp_min(1e-12))
```

Quindi:

```text
H : (32, 4, 10, P, K, D)
N : (32, 4, 10, P, K)
M : (32, 4, 10, P)
```

### 2. Media corretta sulla posizione token

Poiché `P` è ottenuto con zero-padding, dobbiamo usare `M`:

\[
\bar N_{slrk}
=
\frac{
M_{slrp}N_{slrpk}
}{
M_{slrp}
}.
\]

In `einsum`:

```python
num = torch.einsum(
    "slrp,slrpk->slrk",
    M,
    N,
)

den = M.sum(dim=3)  # (S, L, R)

N_mean = num / den[..., None]
```

Otteniamo

```text
N_mean : (32, 4, 10, K)
```

e infine la curva richiesta, media su fatti, lingue e ripetizioni:

\[
\boxed{
\bar N_k
=
\frac1{SLR}
N_{\text{mean},slrk}.
}
\]

In codice:

```python
N_bar = N_mean.mean(dim=(0, 1, 2))
# shape: (K,)
```

Questa è precisamente la curva

\[
k\mapsto\bar N(k).
\]

### 3. Plot

```python
import matplotlib.pyplot as plt

layers = torch.arange(N_bar.shape[0])

plt.figure(figsize=(7, 4))
plt.plot(layers, N_bar.detach().cpu().numpy())
plt.xlabel("Layer k")
plt.ylabel(r"$\bar N(k)$")
plt.title("Residual-stream norm growth")
plt.tight_layout()
plt.show()
```

Per il paper salverei anche la curva normalizzata rispetto al primo layer:

```python
N_bar_rel = N_bar / N_bar[0]

plt.figure(figsize=(7, 4))
plt.plot(layers, N_bar_rel.detach().cpu().numpy())
plt.xlabel("Layer k")
plt.ylabel(r"$\bar N(k) / \bar N(0)$")
plt.title("Relative residual-stream norm growth")
plt.tight_layout()
plt.show()
```

### 4. Una distinzione che farei già adesso

Con i tuoi dati abbiamo abbastanza osservazioni da non fermarci alla media globale. Calcolerei anche

\[
\bar N_{k}^{(s)}
=
\frac1{LR}
N_{\text{mean},slrk},
\]

\[
\bar N_{k}^{(l)}
=
\frac1{SR}
N_{\text{mean},slrk},
\]

e

\[
\bar N_{k}^{(r)}
=
\frac1{SL}
N_{\text{mean},slrk}.
\]

In codice:

```python
N_fact = N_mean.mean(dim=(1, 2))  # (S, K)
N_lang = N_mean.mean(dim=(0, 2))  # (L, K)
N_rep  = N_mean.mean(dim=(0, 1))  # (R, K)
```

Questo ci permette già di controllare che la norm growth globale non sia dominata, per esempio, da una sola lingua o da particolari realizzazioni.

Se mi alleghi il file `H`/`M`, posso passare direttamente dal codice alla verifica numerica della curva e produrre il grafico effettivo.

## You

Ok ora passiamo alle prossime visualizzazioni, come prima con il codice e i plot

## ChatGPT

Procederei mantenendo esattamente la sequenza sperimentale che abbiamo congelato. Dopo \(\bar N(k)\), il passo naturale è separare la geometria radiale da quella angolare e poi costruire le quattro curve \(SS,SD,DS,DD\).

Assumo:

```python
# H:   (S, L, R, P, K, D)
# M:   (S, L, R, P)
```

con nel tuo caso \(S=32,L=4,R=10\).

## 1. Visualizzare la norm growth per fatto, lingua e ripetizione

Prima di passare ai coseni, è utile verificare che la crescita media non sia dominata da un fattore particolare.

Abbiamo già:

```python
norm2 = torch.einsum(
    "slrpkd,slrpkd->slrpk",
    H,
    H,
)

N = torch.sqrt(norm2.clamp_min(1e-12))

num = torch.einsum(
    "slrp,slrpk->slrk",
    M,
    N,
)

den = M.sum(dim=3)

N_mean = num / den[..., None]
```

Ora:

```python
N_bar  = N_mean.mean(dim=(0, 1, 2))  # (K,)
N_fact = N_mean.mean(dim=(1, 2))     # (S, K)
N_lang = N_mean.mean(dim=(0, 2))     # (L, K)
N_rep  = N_mean.mean(dim=(0, 1))     # (R, K)
```

### Plot per lingua

```python
import matplotlib.pyplot as plt

K = N_bar.shape[0]
layers = torch.arange(K)

plt.figure(figsize=(7, 4))

for l in range(N_lang.shape[0]):
    plt.plot(
        layers,
        N_lang[l].detach().cpu().numpy(),
        label=f"language {l}",
    )

plt.plot(
    layers,
    N_bar.detach().cpu().numpy(),
    linewidth=3,
    label="global mean",
)

plt.xlabel("Layer k")
plt.ylabel(r"$\bar N(k)$")
plt.title("Residual-stream norm growth by language")
plt.legend()
plt.tight_layout()
plt.show()
```

### Distribuzione tra i 32 fatti

Più interessante ancora è mostrare la variabilità tra fatti:

```python
plt.figure(figsize=(7, 4))

for s in range(N_fact.shape[0]):
    plt.plot(
        layers,
        N_fact[s].detach().cpu().numpy(),
        alpha=0.15,
    )

plt.plot(
    layers,
    N_bar.detach().cpu().numpy(),
    linewidth=3,
)

plt.xlabel("Layer k")
plt.ylabel(r"$\bar N_s(k)$")
plt.title("Residual-stream norm growth across facts")
plt.tight_layout()
plt.show()
```

Questo ci dice immediatamente se la curva globale è una proprietà collettiva oppure il risultato di pochi fatti.

---

# 2. Normalizzazione: passiamo dalla geometria radiale a quella angolare

Definiamo

\[
U_{slrpkd}
=
\frac{H_{slrpkd}}
{\sqrt{H_{slrpkd'}H_{slrpkd'}}}.
\]

In codice:

```python
U = H / torch.linalg.vector_norm(
    H,
    dim=-1,
    keepdim=True,
).clamp_min(1e-8)
```

Controllo:

```python
check = torch.linalg.vector_norm(U, dim=-1)

print(check[M.bool().unsqueeze(-1).expand_as(check)].mean())
```

Dovrebbe essere circa \(1\) sulle posizioni valide.

---

# 3. La prima visualizzazione veramente importante: \(SS,SD,DS,DD\)

Per il primo esperimento non userei ancora il token-level completo. Usiamo l'ultimo token valido per ogni frase, così evitiamo di mischiare subito le differenze di lunghezza.

Costruiamo l'anchor.

```python
S, L, R, P, K, D = H.shape

lengths = M.sum(dim=-1)          # (S, L, R)
p_last = lengths.long() - 1      # (S, L, R)
```

Per raccogliere il token finale:

```python
idx = p_last[..., None, None].expand(S, L, R, K, D)

A = torch.gather(
    H,
    dim=3,
    index=idx.unsqueeze(3),
).squeeze(3)
```

Ma c'è una forma più pulita con `torch` se vuoi evitare la complessità di `gather`; per il momento possiamo assumere di avere:

```python
# A: (S, L, R, K, D)
```

Normalizziamo:

```python
V = A / torch.linalg.vector_norm(
    A,
    dim=-1,
    keepdim=True,
).clamp_min(1e-8)
```

---

# 4. Gram tensor dell'anchor

Il prodotto scalare è

\[
G_{slr,tjq,k}
=
V_{slrkd}V_{tjqkd}.
\]

Questa è una contrazione Einstein esatta:

```python
G = torch.einsum(
    "slrkd,tjqkd->slrtjqk",
    V,
    V,
)
```

Forma:

```python
print(G.shape)
# (S, L, R, S, L, R, K)
```

Nel nostro caso:

```text
(32, 4, 10, 32, 4, 10, K)
```

Quindi abbiamo conservato esplicitamente la provenienza di entrambi gli stati.

---

# 5. Maschere SS, SD, DS, DD

Creiamo gli indici:

```python
s = torch.arange(S)
l = torch.arange(L)

same_fact = s[:, None, None, None, None, None] == \
            s[None, None, None, :, None, None]

same_lang = l[None, :, None, None, None, None] == \
            l[None, None, None, None, :, None]
```

Per evitare broadcasting poco leggibile, preferirei costruirle direttamente:

```python
same_fact = (
    torch.arange(S)[:, None, None]
    ==
    torch.arange(S)[None, :, None]
)

same_lang = (
    torch.arange(L)[:, None]
    ==
    torch.arange(L)[None, :]
)
```

e poi:

```python
SS = same_fact[:, None, :, :, None, :]
SD = same_fact[:, None, :, :, None, :] & ~same_lang[None, :, None, None, :, :]
```

Per una implementazione robusta, tuttavia, conviene costruire le quattro maschere con `einsum`/broadcasting esplicito una volta sola e conservarle. Il concetto matematico è semplicemente:

\[
M_{SS}=\mathbf1[s=t]\mathbf1[l=j],
\]

\[
M_{SD}=\mathbf1[s=t]\mathbf1[l\neq j],
\]

\[
M_{DS}=\mathbf1[s\neq t]\mathbf1[l=j],
\]

\[
M_{DD}=\mathbf1[s\neq t]\mathbf1[l\neq j].
\]

---

# 6. Separiamo anche \(r=r'\) e \(r\neq r'\)

Questo nel tuo dataset è molto importante.

Definiamo

```python
same_rep = (
    torch.arange(R)[:, None]
    ==
    torch.arange(R)[None, :]
)
```

e avremo quindi, per esempio:

\[
SS_{\mathrm{sameR}}
\]

e

\[
SS_{\mathrm{diffR}}.
\]

Io visualizzerei subito entrambe.

Per esempio, se costruisci una maschera generale:

```python
def masked_mean(x, mask):
    mask = mask.to(dtype=x.dtype)
    return (x * mask).sum(dim=tuple(range(x.ndim - 1))) / mask.sum().clamp_min(1)
```

la statistica relativa può essere calcolata direttamente sulle coppie.

---

# 7. Un modo molto più semplice per il primo plot

Visto che hai solo

\[
32\times4\times10=1280
\]

stati anchor per layer, possiamo anche evitare di materializzare tutte le maschere complicate usando direttamente le matrici \(V\).

Per ogni layer:

```python
C_SS = torch.empty(K)
C_SD = torch.empty(K)
C_DS = torch.empty(K)
C_DD = torch.empty(K)
```

e poi:

```python
for k in range(K):

    Vk = V[:, :, :, k, :]            # (S, L, R, D)
    Vk = Vk.reshape(S * L * R, D)

    Gk = Vk @ Vk.T                   # (SLR, SLR)
```

Ora ci costruiamo le etichette:

```python
facts = torch.arange(S)[:, None, None].expand(S, L, R).reshape(-1)
langs = torch.arange(L)[None, :, None].expand(S, L, R).reshape(-1)
```

e le maschere:

```python
same_s = facts[:, None] == facts[None, :]
same_l = langs[:, None] == langs[None, :]

mask_SS = same_s & same_l
mask_SD = same_s & ~same_l
mask_DS = ~same_s & same_l
mask_DD = ~same_s & ~same_l
```

Escludiamo la diagonale:

```python
eye = torch.eye(len(facts), dtype=torch.bool)

mask_SS &= ~eye
mask_SD &= ~eye
mask_DS &= ~eye
mask_DD &= ~eye
```

e otteniamo:

```python
C_SS[k] = Gk[mask_SS].mean()
C_SD[k] = Gk[mask_SD].mean()
C_DS[k] = Gk[mask_DS].mean()
C_DD[k] = Gk[mask_DD].mean()
```

Questo equivale esattamente alla contrazione tensoriale, ma è molto più facile da verificare nel primo prototipo.

---

# 8. Plot delle quattro curve

```python
plt.figure(figsize=(8, 5))

plt.plot(layers, C_SS.numpy(), label="SS")
plt.plot(layers, C_SD.numpy(), label="SD")
plt.plot(layers, C_DS.numpy(), label="DS")
plt.plot(layers, C_DD.numpy(), label="DD")

plt.xlabel("Layer k")
plt.ylabel("Cosine similarity")
plt.title("Fact-language geometry across layers")
plt.legend()
plt.tight_layout()
plt.show()
```

Questo è probabilmente **il primo plot centrale del paper**.

Quello che cerchiamo inizialmente è qualcosa qualitativamente simile a:

\[
C_{SS}>C_{SD},C_{DS}>C_{DD},
\]

ma non dobbiamo imporre questa gerarchia nel codice.

---

# 9. Plot delle differenze

Subito dopo farei il secondo plot:

\[
\Delta_{\mathrm{language}}
=
C_{SS}-C_{SD},
\]

\[
\Delta_{\mathrm{fact}}
=
C_{SS}-C_{DS}.
\]

```python
delta_language = C_SS - C_SD
delta_fact = C_SS - C_DS

plt.figure(figsize=(8, 5))

plt.plot(
    layers,
    delta_language.numpy(),
    label=r"$\Delta_{\mathrm{language}} = SS-SD$",
)

plt.plot(
    layers,
    delta_fact.numpy(),
    label=r"$\Delta_{\mathrm{fact}} = SS-DS$",
)

plt.axhline(0, linewidth=1)

plt.xlabel("Layer k")
plt.ylabel("Cosine difference")
plt.title("Fact and language contributions")
plt.legend()
plt.tight_layout()
plt.show()
```

Questo è più informativo delle quattro curve quando vogliamo capire come cambia la dominanza relativa dei due fattori.

---

# 10. Il plot che può diventare veramente interessante: interazione

Calcoliamo

\[
\boxed{
\Delta_{\mathrm{int}}(k)
=
SS-SD-DS+DD.
}
\]

```python
delta_int = C_SS - C_SD - C_DS + C_DD

plt.figure(figsize=(8, 5))

plt.plot(
    layers,
    delta_int.numpy(),
)

plt.axhline(0, linewidth=1)

plt.xlabel("Layer k")
plt.ylabel(r"$\Delta_{\mathrm{int}}$")
plt.title("Fact × language interaction")
plt.tight_layout()
plt.show()
```

Qui abbiamo una predizione molto pulita.

Se

\[
\Delta_{\mathrm{int}}\approx0
\]

attraverso i layer, la geometria è compatibile con una struttura additiva.

Se emerge una forma sistematica, ad esempio una regione centrale con forte interazione, avremo una prima indicazione che

\[
H\neq F+L+\epsilon
\]

e che serve un termine

\[
I_{slk:}.
\]

---

## 11. La sequenza di plot che userei adesso

Quindi, a partire dal tuo \(H,M\), produrrei nell'ordine:

\[
\boxed{1.\;\bar N(k)}
\]

norm growth globale;

\[
\boxed{2.\;\bar N_s(k),\bar N_l(k)}
\]

controllo che il norm growth non sia specifico di fatti/lingue;

\[
\boxed{3.\;C_{SS},C_{SD},C_{DS},C_{DD}}
\]

geometria angolare;

\[
\boxed{4.\;\Delta_{\mathrm{fact}},\Delta_{\mathrm{language}}}
\]

separazione dei due fattori;

\[
\boxed{5.\;\Delta_{\mathrm{int}}}
\]

test della fattorialità additiva.

Solo dopo questi cinque plot passerei alle mappe \(p\times p\) della position geometry e alla distinzione sistematica \(r=r'\) versus \(r\neq r'\).

Una precisazione importante rispetto al codice iniziale: **per il prototipo userei l'anchor e un semplice loop sui layer per verificare la correttezza matematica; solo dopo che i risultati sono verificati passerei alla versione interamente vettorializzata con `einsum`**, che sarà quella da usare sul dataset grande. In questo modo possiamo confrontare due implementazioni indipendenti dello stesso osservabile e usarle come controllo interno contro bug di indicizzazione.

## You

come potrei verificare con questi esperimenti una variante LLM/Transformer della ipotesi di Sapir-Whorf?

## ChatGPT

Sì, ma qui farei una distinzione teorica molto netta. La versione interessante di Sapir–Whorf per un Transformer non sarebbe semplicemente:

> «il modello rappresenta diversamente concetti espressi in lingue diverse».

Questo è quasi certamente vero in qualche punto della rete ed è compatibile con una grande quantità di fenomeni banali di codifica linguistica. La versione forte e realmente interessante sarebbe:

\[
\boxed{
\text{la lingua di espressione modifica sistematicamente la traiettoria o la geometria dello stato latente di un medesimo contenuto, anche dopo aver controllato la superficie linguistica.}
}
\]

E, soprattutto nel nostro framework, possiamo rendere questa affermazione molto più precisa.

La letteratura esistente rende il problema particolarmente interessante perché le evidenze sono apparentemente in tensione: alcuni lavori trovano spazi concettuali condivisi e relativamente language-agnostic, mentre altri mostrano persistenti componenti language-sensitive e persino forti effetti della superficie ortografica. Per esempio, Chang et al. trovano sia sottospazi linguistici condivisi sia assi language-sensitive e language-neutral; lavori più recenti su activation patching trovano rappresentazioni concettuali parzialmente language-agnostic; al tempo stesso, studi del 2026 mostrano che nei multilingual LLM la superficie ortografica può avere un ruolo molto forte.  Questo è precisamente il motivo per cui il nostro disegno fattoriale è utile.

## 1. Tradurre Sapir–Whorf nella lingua del tensore \(H\)

Il punto di partenza è il nostro

\[
H_{slrpkd}.
\]

Il fatto \(s\) rappresenta ciò che vogliamo mantenere costante.

La lingua \(l\) è la variabile che manipoliamo.

La realizzazione \(r\) ci permette di cambiare la superficie linguistica mantenendo il contenuto dichiarativamente costante.

La posizione \(p\) e il layer \(k\) devono essere mantenuti espliciti.

La dimensione \(d\) è lo spazio latente.

Quindi, in termini quasi sperimentali,

\[
\boxed{
s=\text{world/content},
\qquad
l=\text{language},
\qquad
r=\text{surface realization}.
}
\]

Questo ci dà già una traduzione molto pulita della domanda Sapir–Whorf:

\[
\text{Does }l\text{ alter the latent representation of }s?
\]

Ma dobbiamo precisare cosa significhi "alter".

Non basta che

\[
H_{sl}\neq H_{sl'}
\]

perché questo potrebbe essere dovuto semplicemente alla diversa lingua.

La domanda interessante è se la differenza linguistica penetri nella componente che rappresenta il fatto.

---

# 2. Tre versioni di Sapir–Whorf per LLM

Proporrei di distinguere tre livelli.

### Versione I: linguistic encoding

La lingua modula la rappresentazione:

\[
H_{sl}\neq H_{sl'}.
\]

Questa è la forma più debole.

Non è ancora una versione interessante di relatività linguistica, perché significa soltanto che "italiano" e "inglese" producono stati diversi.

### Versione II: linguistic modulation of concept geometry

La lingua modifica la rappresentazione del contenuto anche dopo aver separato la componente specificamente linguistica:

\[
F_{s|l}
\neq
F_s.
\]

In altre parole, la feature del fatto dipende dalla lingua in cui il fatto è stato espresso.

Questa è già molto più vicina alla relatività linguistica.

### Versione III: linguistic modulation of latent computation

La lingua non modifica soltanto lo stato in un layer, ma altera la **traiettoria attraverso lo spazio latente**:

\[
k
\mapsto
H_{slrk:}.
\]

Quindi per lo stesso fatto,

\[
\Gamma_{s,l}
=
\left\{
H_{slrk:}
\right\}_{k=1}^{K}
\]

e

\[
\Gamma_{s,l'}
=
\left\{
H_{sl'rk:}
\right\}_{k=1}^{K}
\]

possono avere traiettorie differenti.

Questa sarebbe, a mio avviso, la formulazione più interessante:

\[
\boxed{
\text{language-dependent latent dynamics}.
}
\]

---

# 3. Il nostro \(SS/SD/DS/DD\) diventa già un test di relatività linguistica

Consideriamo:

\[
C_{SS}(k),
\quad
C_{SD}(k),
\quad
C_{DS}(k),
\quad
C_{DD}(k).
\]

La quantità fondamentale per Sapir–Whorf è

\[
\boxed{
\Delta_{\mathrm{lang}}(k)
=
C_{SS}(k)-C_{SD}(k).
}
\]

Perché?

\(SS\) confronta lo stesso fatto nella stessa lingua.

\(SD\) confronta lo stesso fatto in lingue diverse.

Quindi la differenza misura quanto la rappresentazione di **uno stesso contenuto** si separi quando cambiamo lingua.

Se

\[
\Delta_{\mathrm{lang}}(k)\approx0,
\]

allora il contenuto è rappresentato in modo quasi invariato rispetto alla lingua.

Se

\[
\Delta_{\mathrm{lang}}(k)>0,
\]

la lingua ha lasciato una firma geometrica sullo stato del fatto.

Ma ancora una volta: questo non è sufficiente.

Potrebbe essere soltanto language encoding.

---

# 4. Il test veramente interessante è confrontare lingua e fatto

Abbiamo anche

\[
\Delta_{\mathrm{fact}}(k)
=
C_{SS}(k)-C_{DS}(k).
\]

Ora confrontiamo

\[
\Delta_{\mathrm{lang}}(k)
\]

e

\[
\Delta_{\mathrm{fact}}(k).
\]

Possiamo definire

\[
\boxed{
R_{\mathrm{LW}}(k)
=
\frac{
\Delta_{\mathrm{lang}}(k)
}{
\Delta_{\mathrm{fact}}(k)
}
}
\]

come una misura descrittiva del rapporto tra effetto linguistico e contenutistico.

Non lo userei come statistica primaria, ma come visualizzazione.

La domanda diventa:

\[
\text{quando }k\text{ aumenta, la lingua perde peso rispetto al fatto?}
\]

oppure

\[
\text{la lingua rimane una componente permanente della rappresentazione?}
\]

Questo è molto vicino alla distinzione empirica tra una fase language-specific e una fase più abstract concept-like osservata in diversi studi multilingual. 

---

# 5. La vera formulazione tensoriale di Sapir–Whorf

Possiamo fare un passo ulteriore.

Nel nostro modello

\[
H_{slrpk:}
=
c_{k:}
+
\alpha_s^k a_{sk:}
+
\beta_l^k b_{lk:}
+
\gamma_p^k q_{pk:}
+
X_{slrk:}
+
I_{slk:}
+
\epsilon_{slrpk:},
\]

la teoria "lingua non influenza concetto" corrisponde, in forma forte, a

\[
\boxed{
a_{sk:}\ \text{indipendente da }l.
}
\]

La lingua può avere una propria componente

\[
b_{lk:},
\]

ma il vettore che rappresenta il fatto rimane invariato.

Questa sarebbe una specie di **ipotesi di universalità semantica**.

La versione relativista invece ammette

\[
\boxed{
a_{slk:}
}
\]

anziché

\[
a_{sk:}.
\]

Cioè:

\[
H_{slrpk:}
=
c_{k:}
+
\alpha_{sl}^k a_{slk:}
+
\beta_l^k b_{lk:}
+\cdots
\]

Qui la differenza è concettualmente enorme.

La lingua non è più soltanto un termine aggiuntivo:

\[
\text{fact}+\text{language}.
\]

Diventa una variabile che **modula la stessa rappresentazione del fatto**.

Questo è molto più vicino alla relatività linguistica.

---

# 6. Possiamo testare questa distinzione direttamente

Per ogni fatto \(s\), stimiamo una rappresentazione fact-specifica per ciascuna lingua:

\[
F_{slkd}.
\]

Poi decomponiamo:

\[
F_{slkd}
=
\bar F_{skd}
+
\Lambda_{slkd},
\]

dove

\[
\bar F_{skd}
=
\frac1L
F_{slkd}.
\]

La componente

\[
\Lambda_{slkd}
\]

è precisamente la deviazione language-conditioned del fatto.

La quantità

\[
\|\Lambda_{slk:}\|
\]

misura quanto la rappresentazione dello stesso fatto cambia tra lingue.

Ma soprattutto possiamo calcolare il rapporto

\[
\boxed{
R_{\mathrm{rel}}^k
=
\frac{
\sum_{s,l}
\|\Lambda_{slk:}\|^2
}{
\sum_{s,l}
\|F_{slk:}-\bar F_{k:}\|^2
}.
}
\]

Questo è concettualmente molto più vicino alla domanda Sapir–Whorf:

> quale frazione della variabilità fattuale interna al modello è sistematicamente condizionata dalla lingua?

---

# 7. La parte davvero importante: la traiettoria attraverso i layer

Io ritengo che il vero punto di forza del nostro progetto sia proprio l'asse \(k\).

Un classico test Sapir–Whorf osserva il comportamento.

Noi possiamo osservare la dinamica interna:

\[
H^1\rightarrow H^2\rightarrow\cdots\rightarrow H^K.
\]

Per un fatto \(s\), definiamo la traiettoria per lingua

\[
\Gamma_{s,l}(k)
=
U_{slrk:}.
\]

Poi misuriamo

\[
D_{\mathrm{lang}}^s(k)
=
1-
\cos(
\Gamma_{s,l}(k),
\Gamma_{s,l'}(k)
).
\]

Se troviamo

\[
D_{\mathrm{lang}}^s(k)
\]

elevato nei primi layer ma prossimo a zero nei layer intermedi, avremmo qualcosa come:

\[
\text{language encoding}
\rightarrow
\text{shared concept representation}.
\]

Questo è compatibile con l'idea di uno spazio concettuale condiviso e con lavori che hanno trovato una separazione tra fase linguistica e fase concettuale. 

Ma una vera firma Sapir–Whorf sarebbe più interessante:

\[
D_{\mathrm{lang}}^s(k)
\]

che **non scompare dopo l'emersione del contenuto**, oppure che si manifesta come differenze nella traiettoria verso la stessa funzione finale.

In altre parole:

\[
\boxed{
\text{language changes the path, not merely the starting point.}
}
\]

---

# 8. Qui le parafrasi diventano cruciali

Supponiamo che l'italiano abbia una componente lessicale particolare.

Potremmo osservare

\[
H_{\text{Italian},r}
\]

diverso da

\[
H_{\text{English},r}.
\]

Ma se questa differenza dipende soltanto dai token, non abbiamo Sapir–Whorf.

Perciò dobbiamo confrontare le quattro realizzazioni \(r\).

Definiamo

\[
\mu_{slk:}
=
\frac1R
H_{slr:k:}.
\]

E quindi

\[
X_{slrk:}
=
H_{slrk:}-\mu_{slk:}.
\]

La domanda diventa:

\[
\text{la componente language-dependent sopravvive alla media sulle parafrasi?}
\]

Se sì, diventa molto più difficile attribuirla al semplice lessico di una singola frase.

Questo è uno dei motivi per cui il tuo asse \(R=10\) è molto più importante di quanto sembrasse inizialmente.

---

# 9. Il test decisivo: linguistically conditioned fact representation

Possiamo costruire una vera statistica di relatività linguistica.

Per ogni fatto:

\[
\bar F_{sk:}
=
\frac1L
F_{slk:}.
\]

Poi:

\[
\delta_{slk:}
=
F_{slk:}-\bar F_{sk:}.
\]

Misuriamo

\[
V_{\mathrm{lang|fact}}^k
=
\frac1{SL}
\delta_{slkd}\delta_{slkd}.
\]

E confrontiamolo con

\[
V_{\mathrm{fact}}^k.
\]

Se

\[
V_{\mathrm{lang|fact}}^k
\]

è sistematicamente > 0, significa che lo stesso fatto possiede una struttura interna dipendente dalla lingua.

Ma vogliamo anche sapere se questa componente è soltanto linguistica o se è effettivamente **semanticamente organizzata**.

Per questo dobbiamo fare un test ancora più forte.

---

# 10. Il cross-fact test

Supponiamo che l'italiano deformi il concetto di un certo fatto.

Se la deformazione è veramente una proprietà della lingua, dovrebbe manifestarsi in **più fatti**.

Quindi definiamo la deviazione linguistica

\[
\Lambda_{slk:}.
\]

Per due fatti differenti,

\[
s\neq s',
\]

possiamo chiedere se

\[
\Lambda_{slk:}
\]

e

\[
\Lambda_{s'lk:}
\]

condividano una struttura.

In altre parole:

\[
\operatorname{Cov}_s(\Lambda_{slk:})
\]

dovrebbe produrre una componente sistematica associata alla lingua.

Se invece ogni fatto ha una deviazione completamente idiosincratica, potrebbe trattarsi semplicemente di un'interazione frase-specifica.

Questo distingue:

\[
\text{language effect}
\]

da

\[
\text{language}\times\text{fact idiosyncrasy}.
\]

---

# 11. Un criterio ancora più forte: semantic prediction across languages

Qui possiamo fare un test quasi "causale" della relatività.

Prendiamo una rappresentazione del fatto in italiano:

\[
F_{\mathrm{it},s,k}.
\]

Stimiamo la componente language-independent:

\[
\bar F_{s,k}.
\]

Poi cerchiamo di ricostruire la rappresentazione inglese:

\[
\widehat F_{\mathrm{en},s,k}
=
T_{\mathrm{en}\leftarrow\mathrm{it}}
(
F_{\mathrm{it},s,k}
).
\]

Se una semplice trasformazione linguistica spiega tutto, allora:

\[
T_{\mathrm{en}\leftarrow\mathrm{it}}
\]

rimuove la specificità della lingua.

Ma se rimane una differenza strutturale associata al contenuto,

\[
F_{\mathrm{en},s,k}
-
\widehat F_{\mathrm{en},s,k},
\]

questa potrebbe costituire evidenza di una rappresentazione language-conditioned più profonda.

---

# 12. E qui entra l'analogia con Sapir–Whorf umano

È importante non dire che questo "dimostra Sapir–Whorf".

La formulazione classica della linguistic relativity riguarda il rapporto tra struttura linguistica, esperienza e pensiero umano; la letteratura distingue esplicitamente approcci centrati sulla struttura linguistica, sul dominio percettivo/concettuale e sul comportamento. 

Per un LLM possiamo formulare soltanto un'analogia computazionale:

\[
\boxed{
\text{language-dependent representation of a common latent input}
}
\]

invece di

\[
\boxed{
\text{language-dependent human cognition}.
}
\]

La domanda scientifica diventa quindi:

> Se lo stesso stato del mondo viene espresso mediante sistemi linguistici diversi, il Transformer costruisce stati latenti differenti non soltanto per codificare la lingua, ma anche per rappresentare il contenuto?

Questa è una domanda perfettamente sperimentabile.

---

# 13. La predizione più bella che potremmo ottenere

Immaginiamo di avere:

\[
C_{SS}(k),C_{SD}(k),C_{DS}(k),C_{DD}(k).
\]

Supponiamo che emerga:

\[
C_{SD}(k)
\approx
C_{SS}(k)
\]

nei layer intermedi.

Questo direbbe:

\[
\text{same fact}
\approx
\text{same fact across languages}.
\]

Potremmo interpretarlo come convergenza verso una rappresentazione language-invariant.

Ma se invece vedessimo:

\[
C_{SD}(k)<C_{SS}(k)
\]

in modo robusto nei layer semantici, allora la lingua rimane incorporata nella rappresentazione del contenuto.

Ancora più interessante sarebbe trovare:

\[
\Delta_{\mathrm{lang}}(k)
\]

diverso da zero **soltanto per certi domini concettuali**.

Allora avremmo:

\[
\Delta_{\mathrm{lang}}^{(\text{history})}(k)
\neq
\Delta_{\mathrm{lang}}^{(\text{math})}(k).
\]

Questo sarebbe molto più vicino a una teoria di relatività linguistica che a una semplice codifica linguistica.

---

# 14. Il tuo dataset attuale ha già un primo vantaggio, ma anche un limite

I quattro fatti attuali sono:

\[
\{\text{history},\text{math},\text{biology},\text{cooking}\}.
\]

Questo è ottimo perché introduce domini semantici diversi.

Ma non è ancora sufficiente per una conclusione forte su Sapir–Whorf.

Servirà successivamente un dataset in cui i fatti siano scelti in modo specifico per avere strutture concettuali potenzialmente sensibili alla lingua.

Per esempio, concetti relativi a:

\[
\text{spazio},
\quad
\text{tempo},
\quad
\text{quantificazione},
\quad
\text{agency},
\quad
\text{evidentiality},
\quad
\text{motion}.
\]

La ragione è che un fatto come

> "Julius Caesar was murdered in 44 BC"

è quasi idealmente universale e probabilmente non è il miglior oggetto per cercare relatività linguistica.

Il tuo dataset attuale è quindi perfetto come **sanity-check sulla composizionalità cross-lingual**, ma un secondo dataset dovrebbe essere progettato per stressare specificamente l'ipotesi relativista.

---

# 15. La gerarchia sperimentale che proporrei

Partirei dal tuo dataset attuale.

**Livello 1. Encoding.**

Verificare che

\[
\Delta_{\mathrm{lang}}(k)>0
\]

in qualche regione della rete.

**Livello 2. Surface control.**

Ripetere il risultato dopo avere mediato sulle 10 realizzazioni e controllato \(X\).

**Livello 3. Position control.**

Verificare che l'effetto rimanga dopo il controllo di \(P\).

**Livello 4. Fact-conditioned language effect.**

Stimare

\[
V_{\mathrm{lang|fact}}^k.
\]

**Livello 5. Cross-fact generalization.**

Verificare che la stessa struttura language-conditioned emerga attraverso fatti diversi.

**Livello 6. Layer dynamics.**

Studiare

\[
k\mapsto\Delta_{\mathrm{lang}}(k).
\]

**Livello 7. Domain dependence.**

Confrontare domini concettuali differenti.

**Livello 8. Causal intervention.**

Prendere una rappresentazione semantica e fare activation patching o steering fra lingue, chiedendo se possiamo cambiare la rappresentazione linguistica senza alterare il contenuto e viceversa.

Questo ultimo punto sarebbe particolarmente forte perché esistono già lavori che mostrano la possibilità di manipolare indipendentemente concetto e lingua attraverso activation patching. 

---

# 16. La forma tensoriale finale

La formulazione che trovo più elegante è questa:

\[
\boxed{
H_{slrpkd}
=
C_{kd}
+
F_{skd}
+
L_{lkd}
+
P_{pkd}
+
X_{slrkd}
+
I_{slkd}
+
\epsilon_{slrpkd}
}
\]

rappresenta una teoria **non relativista** del contenuto se

\[
F_{skd}
\]

è indipendente da \(l\).

Una teoria **relativista debole** ammette

\[
F_{slkd}
\]

con una componente sistematica \(l\)-dependent.

Una teoria **relativista dinamica** ammette inoltre che

\[
F_{sl(k+1)d}-F_{slkd}
\]

dipenda da \(l\).

Quindi non studiamo semplicemente

\[
F_{sl}.
\]

Studiamo la derivata discreta lungo la profondità:

\[
\boxed{
\Delta_kF_{slkd}
=
F_{sl(k+1)d}-F_{slkd}.
}
\]

Ed emerge una connessione molto bella con l'idea che avevamo iniziato a discutere della "derivata" del tensore lungo il layer.

La domanda Sapir–Whorf diventa:

\[
\boxed{
\Delta_kF_{slkd}
\stackrel{?}{=}
\Delta_kF_{skd}
}
\]

oppure

\[
\boxed{
\Delta_kF_{slkd}
\neq
\Delta_kF_{skd}.
}
\]

Nel primo caso la lingua non modifica la dinamica concettuale, ma solo la codifica linguistica.

Nel secondo caso la lingua modifica il modo in cui il contenuto viene trasformato nello spazio latente.

Questa seconda situazione sarebbe una formulazione computazionalmente molto forte della relatività linguistica.

## 17. Una predizione che considero particolarmente interessante

Io formulerei come predizione principale non

\[
C_{SS}>C_{SD},
\]

ma questa:

\[
\boxed{
\text{Dopo il controllo per superficie e posizione, la lingua modula sistematicamente la traiettoria latente del contenuto lungo }k.
}
\]

Formalmente, vogliamo mostrare che

\[
\operatorname{Var}_l
\left[
\Delta_kF_{slkd}
\mid s
\right]
>0
\]

in modo significativo e replicabile attraverso fatti e modelli.

E poi chiedere se tale quantità sia:

\[
\text{domain-independent}
\]

oppure

\[
\text{domain-dependent}.
\]

Se è la seconda, potremmo essere davanti a qualcosa di ancora più interessante: non una generica "language-specific representation", ma una **linguistic modulation of particular semantic geometries**.

A quel punto il collegamento con Sapir–Whorf diventerebbe molto più serio.

Il punto concettuale decisivo è quindi questo: **la nostra teoria non deve cercare di dimostrare che i modelli "pensano in lingue diverse". Deve verificare se la variabile lingua \(l\), a contenuto \(s\) controllato, entra nella legge dinamica con cui il Transformer costruisce il proprio stato latente.** Questa è una proprietà interna osservabile, formalizzabile e falsificabile; e il tuo tensore \(H_{slrpkd}\) contiene esattamente le variabili necessarie per testarla.

## You

Io ho già una libreria che può lavorare sul tensor H ed M per un oggetto che ora ho aumentato a 32 fatti, 4 lingue e 10 parafrasi per lingua con un modello QWen3-0.6B che ha 29 layers e 1024 di dimensione latente. Il numero totale di token è circa 30 quando prendo il massimo su tutte le parafrasi su tutte le lingue e la maschera M mi aiuta a gestire i calcoli.
Mi puoi provare a proporre gli esperimenti con torch.einsum per testare Sapir-Whorf?

## ChatGPT

Sì. Con \(H:(32,4,10,30,29,1024)\) e \(M:(32,4,10,30)\) possiamo fare un programma molto pulito: prima misurare l’effetto linguistico, poi dimostrare che non è lessicale/posizionale, infine verificare se altera la traiettoria concettuale lungo i layer.

## ChatGPT

Sì. Con il tuo oggetto attuale possiamo costruire una batteria Sapir–Whorf molto pulita, tutta espressa come riduzioni di

\[
H_{slrpkd},
\qquad
M_{slrp},
\]

e con una distinzione fondamentale tra:

1. effetto puramente linguistico;
2. effetto linguistico sulla rappresentazione del fatto;
3. persistenza dell'effetto lungo \(k\);
4. controllo per parafrasi e posizione;
5. falsificazione tramite permutation null.

Per il primo passaggio userei come rappresentazione primaria l'ultimo token valido, non il mean pooling. Il mean pooling lo terrei come robustness check, proprio perché vogliamo evitare di cancellare la struttura posizionale.

## 0. Convenzioni

Assumiamo:

```python
# H: (S, L, R, P, K, D)
# M: (S, L, R, P)

S, L, R, P, K, D = H.shape
# S=32, L=4, R=10, P≈30, K=29, D=1024
```

### Estrazione dell'ultimo token valido

```python
lengths = M.sum(dim=-1).long()          # (S, L, R)
p_last = (lengths - 1).clamp_min(0)

idx = p_last[..., None, None, None].expand(S, L, R, 1, K, D)

H_last = H.gather(dim=3, index=idx).squeeze(3)
# (S, L, R, K, D)
```

Quindi definiamo

\[
X_{slrkd}=H_{slr p^\star_{slr}kd}.
\]

---

# 1. Esperimento SW-1: stesso fatto, stessa lingua vs stesso fatto, lingua diversa

Questo è il primo test diretto.

Vogliamo confrontare:

\[
SS:
(s,l)\leftrightarrow(s,l)
\]

contro

\[
SD:
(s,l)\leftrightarrow(s,l'),
\qquad l\neq l'.
\]

Per prima cosa eliminiamo l'asse delle parafrasi facendo il prototype medio:

\[
F_{slkd}
=
\frac1R\sum_r H_{slrkd}.
\]

Con `einsum`:

```python
F = torch.einsum(
    "slrkd->slkd",
    H_last,
) / R
```

Normalizziamo:

```python
U = F / torch.linalg.vector_norm(
    F,
    dim=-1,
    keepdim=True,
).clamp_min(1e-8)
```

Quindi

\[
U\in\mathbb R^{S\times L\times K\times D}.
\]

La geometria same-fact/cross-language è:

\[
C_{sll'k}
=
U_{slkd}U_{sl'kd}.
\]

In `einsum`:

```python
C_lang = torch.einsum(
    "slkd,sjkd->sljk",
    U,
    U,
)
# (S, L, L, K)
```

La diagonale \(l=l'\) è \(SS\), mentre \(l\neq l'\) è \(SD\).

```python
same_l = ~torch.eye(L, dtype=torch.bool, device=H.device)

C_SD = C_lang[:, same_l, :]
```

Più comodamente:

```python
mask_offdiag = ~torch.eye(L, dtype=torch.bool, device=H.device)

C_SD = C_lang[:, mask_offdiag, :]
C_SD = C_SD.reshape(S, L * (L - 1), K).mean(dim=1)
```

Otteniamo:

\[
\boxed{
C_{SD}(k).
}
\]

Per \(SS\), non userei semplicemente \(1\): dopo la media delle parafrasi vogliamo verificare la stabilità intra-lingua in funzione delle realizzazioni. Questo porta direttamente al prossimo esperimento.

---

# 2. Esperimento SW-2: il contenuto è stabile rispetto alla parafrasi?

Qui vogliamo verificare:

\[
H_{slr}
\approx
H_{slr'}
\]

per

\[
r\neq r'.
\]

Manteniamo quindi l'asse \(R\).

Normalizziamo ogni parafrasi:

```python
U_r = H_last / torch.linalg.vector_norm(
    H_last,
    dim=-1,
    keepdim=True,
).clamp_min(1e-8)
```

Ora:

\[
C^{\mathrm{para}}_{slrr'k}
=
U_{slrkd}U_{slr'kd}.
\]

```python
C_para = torch.einsum(
    "slrkd,slqkd->slrqk",
    U_r,
    U_r,
)
# (S, L, R, R, K)
```

Selezioniamo \(r\neq r'\):

```python
mask_r = ~torch.eye(R, dtype=torch.bool, device=H.device)

C_para_diff = C_para[:, :, mask_r, :]
C_para_diff = C_para_diff.reshape(S, L, R * (R - 1), K).mean(dim=2)

C_para_mean = C_para_diff.mean(dim=(0, 1))
# (K,)
```

La quantità

\[
\boxed{
C_{\mathrm{para}}(k)
}
\]

ci dice quanto la rappresentazione dello stesso contenuto sia invariata sotto variazione della superficie.

Questo è un controllo indispensabile prima di interpretare \(C_{SD}\) come effetto Sapir–Whorf.

---

# 3. Esperimento SW-3: separare "lingua" da "fatto"

Questo è probabilmente il test più interessante con il dataset attuale.

Partiamo da

\[
F_{slkd}.
\]

Per ciascun fatto calcoliamo il suo centro attraverso le lingue:

\[
\bar F_{skd}
=
\frac1L\sum_l F_{slkd}.
\]

```python
F_fact = torch.einsum(
    "slkd->skd",
    F,
) / L
```

La deviazione language-conditioned è

\[
\boxed{
\Lambda_{slkd}
=
F_{slkd}-\bar F_{skd}.
}
\]

```python
Lambda = F - F_fact[:, None, :, :]
# (S, L, K, D)
```

Ora calcoliamo la potenza della componente language-conditioned:

\[
V_{\mathrm{lang|fact}}^k
=
\frac1{SL}
\Lambda_{slkd}\Lambda_{slkd}.
\]

Con Einstein:

```python
V_lang_fact = torch.einsum(
    "slkd,slkd->slk",
    Lambda,
    Lambda,
).mean(dim=(0, 1))
# (K,)
```

Questa quantità è molto più vicina alla versione computazionale di Sapir–Whorf:

\[
\boxed{
\text{quanto varia la rappresentazione dello stesso fatto al variare della lingua?}
}
\]

Naturalmente bisogna ancora stabilire se tale deviazione sia una semplice firma linguistica o una vera modulazione del contenuto. Ma abbiamo ora una misura diretta.

---

# 4. Esperimento SW-4: confronto con la variabilità tra fatti

Per capire se la lingua produce una modulazione forte o marginale, misuriamo la variabilità fact-conditioned e language-conditioned.

La media globale è

\[
\bar F_{kd}
=
\frac1{SL}
\sum_{s,l}F_{slkd}.
\]

```python
F_global = torch.einsum(
    "slkd->kd",
    F,
) / (S * L)
```

La componente fattuale:

\[
\Phi^F_{skd}
=
\bar F_{skd}-\bar F_{kd}.
\]

```python
Fact_effect = F_fact - F_global[None, :, :]
```

e la relativa energia:

```python
V_fact = torch.einsum(
    "skd,skd->sk",
    Fact_effect,
    Fact_effect,
).mean(dim=0)
# (K,)
```

Possiamo quindi confrontare

\[
V_{\mathrm{lang|fact}}(k)
\]

e

\[
V_{\mathrm{fact}}(k).
\]

Il rapporto descrittivo:

\[
\boxed{
R_{\mathrm{SW}}(k)
=
\frac{
V_{\mathrm{lang|fact}}(k)
}{
V_{\mathrm{fact}}(k)
}
}
\]

può essere molto utile.

```python
R_SW = V_lang_fact / V_fact.clamp_min(1e-12)
```

Non lo userei come test statistico primario, ma come figura.

Interpretazione:

- \(R_{\mathrm{SW}}\ll1\): il contenuto domina la lingua;
- \(R_{\mathrm{SW}}\sim1\): la lingua induce una variazione comparabile a quella fra fatti;
- \(R_{\mathrm{SW}}>1\): risultato molto forte, anche se richiederebbe controlli molto severi.

---

# 5. Esperimento SW-5: la geometria additiva fact + language

Qui possiamo testare direttamente il modello

\[
F_{slkd}
\approx
C_{kd}+F_{skd}+L_{lkd}+\epsilon_{slkd}.
\]

Costruiamo prima le tre marginali.

Media globale:

\[
\mu_{kd}
=
\frac1{SL}F_{slkd}.
\]

Fact effect:

\[
A_{skd}
=
\frac1L F_{slkd}-\mu_{kd}.
\]

Language effect:

\[
B_{lkd}
=
\frac1S F_{slkd}-\mu_{kd}.
\]

In codice:

```python
mu = F.mean(dim=(0, 1))          # (K, D)
A = F.mean(dim=1) - mu           # (S, K, D)
B = F.mean(dim=0) - mu           # (L, K, D)
```

La previsione additiva diventa:

\[
\boxed{
\widehat F_{slkd}
=
\mu_{kd}
+
A_{skd}
+
B_{lkd}.
}
\]

```python
F_hat = (
    mu[None, None, :, :]
    + A[:, None, :, :]
    + B[None, :, :, :]
)
```

Il residuo di interazione:

\[
\boxed{
I_{slkd}
=
F_{slkd}-\widehat F_{slkd}.
}
\]

```python
I = F - F_hat
```

La sua energia:

```python
V_interaction = torch.einsum(
    "slkd,slkd->slk",
    I,
    I,
).mean(dim=(0, 1))
```

Quindi abbiamo tre curve:

\[
V_F(k),
\qquad
V_L(k),
\qquad
V_I(k).
\]

Questo è molto importante per Sapir–Whorf.

Se troviamo

\[
V_L>0
\]

ma

\[
V_I\approx0,
\]

la lingua influenza la rappresentazione in maniera prevalentemente additiva.

Se invece

\[
V_I
\]

è forte, abbiamo qualcosa di più interessante:

\[
\boxed{
\text{the representation of the fact depends on the language through an interaction.}
}
\]

Questo è concettualmente più vicino alla relatività linguistica che a un semplice language embedding aggiunto alla rappresentazione.

---

# 6. Esperimento SW-6: la lingua modifica la traiettoria lungo i layer?

Questo è quello che considero il test più interessante.

Abbiamo già

\[
F_{slkd}.
\]

Definiamo lo stato normalizzato:

```python
U_F = F / torch.linalg.vector_norm(
    F,
    dim=-1,
    keepdim=True,
).clamp_min(1e-8)
```

Per ogni fatto confrontiamo due lingue:

\[
D_{\mathrm{lang}}(k)
=
1-
\operatorname{mean}_{s,l\neq l'}
\left[
U_{slkd}U_{sl'kd}
\right].
\]

L'einsum è già quello dell'esperimento 1:

```python
C_lang = torch.einsum(
    "slkd,sjkd->sljk",
    U_F,
    U_F,
)
```

e quindi:

```python
C_SD = C_lang[:, mask_offdiag, :]
C_SD = C_SD.reshape(S, L * (L - 1), K).mean(dim=1)

D_lang = 1.0 - C_SD
```

Questo produce

\[
\boxed{
D_{\mathrm{lang}}(k).
}
\]

Ora facciamo l'analogo cambiando fatto ma mantenendo la lingua:

\[
D_{\mathrm{fact}}(k)
=
1-C_{DS}(k).
\]

```python
C_fact = torch.einsum(
    "slkd,t l k d->stlk",
    U_F,
    U_F,
)
# (S, S, L, K)
```

Escludiamo \(s=t\):

```python
mask_fact = ~torch.eye(S, dtype=torch.bool, device=H.device)

C_DS = C_fact[mask_fact].reshape(
    S * (S - 1), L, K
).mean(dim=(0, 1))

D_fact = 1.0 - C_DS
```

Otteniamo quindi direttamente

\[
\boxed{
D_{\mathrm{lang}}(k)
\quad\text{vs}\quad
D_{\mathrm{fact}}(k).
}
\]

Il plot è molto semplice:

```python
plt.figure(figsize=(8, 5))

plt.plot(
    range(K),
    D_lang.cpu(),
    label="language distance",
)

plt.plot(
    range(K),
    D_fact.cpu(),
    label="fact distance",
)

plt.xlabel("Layer k")
plt.ylabel("Angular distance")
plt.legend()
plt.tight_layout()
plt.show()
```

La forma che sarebbe particolarmente interessante è una \(D_{\mathrm{lang}}(k)\) che non si limiti a essere alta nei primi layer, ma che mostri una struttura sistematica proprio nella regione in cui emerge la rappresentazione fattuale.

---

# 7. Ancora meglio: studiare la derivata discreta della traiettoria

Possiamo definire

\[
\Delta_kF_{slkd}
=
F_{sl(k+1)d}-F_{slkd}.
\]

In PyTorch:

```python
dF = F[:, :, 1:, :] - F[:, :, :-1, :]
# (S, L, K-1, D)
```

Normalizziamo la variazione:

```python
dU = dF / torch.linalg.vector_norm(
    dF,
    dim=-1,
    keepdim=True,
).clamp_min(1e-8)
```

Ora possiamo chiedere se la **direzione dell'aggiornamento** dipenda dalla lingua:

\[
C_{\Delta,\mathrm{lang}}^k
=
dU_{slkd}dU_{sl'kd}.
\]

```python
C_dlang = torch.einsum(
    "slkd,sjkd->sljk",
    dU,
    dU,
)
```

Questa è una versione molto più forte della domanda Sapir–Whorf:

\[
\boxed{
\text{language-dependent latent dynamics}
}
\]

anziché semplicemente

\[
\text{language-dependent latent state}.
\]

Se due lingue hanno rappresentazioni differenti ma gli aggiornamenti successivi sono sostanzialmente uguali, potremmo avere semplicemente diversi punti di partenza.

Se invece

\[
\Delta_kF_{sl:}
\]

dipende sistematicamente da \(l\), allora la lingua sta modulando il calcolo stesso.

---

# 8. Controllo fondamentale: superficie linguistica

Ripetiamo tutto con le singole parafrasi invece che con

\[
F=\operatorname{mean}_r H.
\]

Questo ci permette di distinguere:

\[
\text{language effect}
\]

da

\[
\text{surface realization effect}.
\]

Per esempio possiamo confrontare

\[
C_{\mathrm{same-language,different-paraphrase}}
\]

con

\[
C_{\mathrm{same-fact,different-language}}.
\]

La domanda è:

\[
\boxed{
C_{\mathrm{same-fact,different-language}}
<
C_{\mathrm{same-fact,different-paraphrase}}?
}
\]

Se sì, la differenza tra lingue è maggiore della variabilità normalmente introdotta da una parafrasi nella stessa lingua.

Questo sarebbe un controllo molto convincente.

---

# 9. Un permutation test che userei sicuramente

Per Sapir–Whorf la permutazione più pulita è:

> per ogni fatto \(s\), permutare casualmente le quattro etichette linguistiche fra le quattro lingue.

Quindi preserviamo completamente la struttura dei fatti e la distribuzione marginale delle lingue, ma rompiamo la corrispondenza linguistica.

Per una statistica \(T(H)\), ad esempio

\[
T=
\frac1K\sum_k D_{\mathrm{lang}}(k),
\]

generiamo

\[
T^{(1)},\ldots,T^{(B)}
\]

dai dati permutati.

Schema minimale:

```python
B = 1000
null = torch.empty(B)

for b in range(B):
    F_perm = F.clone()

    for s in range(S):
        perm = torch.randperm(L)
        F_perm[s] = F_perm[s, perm]

    U_perm = F_perm / torch.linalg.vector_norm(
        F_perm,
        dim=-1,
        keepdim=True,
    ).clamp_min(1e-8)

    C_perm = torch.einsum(
        "slkd,sjkd->sljk",
        U_perm,
        U_perm,
    )

    C_SD_perm = C_perm[:, mask_offdiag, :]
    C_SD_perm = C_SD_perm.reshape(
        S, L * (L - 1), K
    ).mean(dim=1)

    D_perm = 1 - C_SD_perm

    null[b] = D_perm.mean()

T_obs = D_lang.mean()

p_value = (null >= T_obs).float().mean()
```

Per il paper userei una versione vettorializzata, ma questo è sufficiente per verificare la logica.

Ancora meglio: non farei solo il test globale. Calcolerei una distribuzione nulla per ogni \(k\), ottenendo una curva di significatività.

---

# 10. Il test che secondo me dovrebbe diventare il cuore del paper

Alla fine costruirei un pannello di quattro curve:

\[
\boxed{
D_{\mathrm{lang}}(k)
}
\]

\[
\boxed{
D_{\mathrm{paraphrase}}(k)
}
\]

\[
\boxed{
D_{\mathrm{fact}}(k)
}
\]

\[
\boxed{
D_{\mathrm{interaction}}(k).
}
\]

E cercherei precisamente questa situazione:

\[
D_{\mathrm{paraphrase}}
<
D_{\mathrm{lang}}
<
D_{\mathrm{fact}}
\]

oppure, più realisticamente, una regione dei layer nella quale

\[
D_{\mathrm{lang}}
\]

è sistematicamente superiore alla normale variabilità di parafrasi ma inferiore alla separazione tra fatti.

La situazione diventerebbe ancora più forte se

\[
D_{\mathrm{lang}}(k)
\]

persiste dopo aver mediato \(r\), controllato \(p\), normalizzato la norma e superato il permutation null.

A quel punto potremmo formulare la conclusione in modo molto preciso:

> A common semantic content is represented by a family of language-conditioned latent states, rather than by a language-invariant state plus an independent language code.

Questa è una formulazione computazionale molto più forte di "il modello conosce quattro lingue".

---

## Pipeline concreta per il tuo Qwen3-0.6B

Con il tuo

\[
H:(32,4,10,30,29,1024)
\]

io eseguirei esattamente quest'ordine:

\[
H
\rightarrow
H_{\rm last}
\rightarrow
F_{slkd}
\rightarrow
U_{slkd}
\]

e poi:

\[
\boxed{\text{1. paraphrase invariance}}
\]

\[
\boxed{\text{2. }SS/SD/DS/DD}
\]

\[
\boxed{\text{3. language-conditioned fact variance}}
\]

\[
\boxed{\text{4. fact/language ANOVA}}
\]

\[
\boxed{\text{5. permutation null}}
\]

\[
\boxed{\text{6. layerwise trajectory}}
\]

\[
\boxed{\text{7. discrete derivative of trajectory}}.
\]

Il punto 7 è quello che terrei particolarmente d'occhio: se emerge una dipendenza da \(l\) non solo in \(F_{slkd}\), ma nella variazione

\[
F_{sl(k+1)d}-F_{slkd},
\]

allora abbiamo fatto un salto concettuale importante. Non stiamo più dicendo che "la lingua lascia una firma nel residual stream"; stiamo osservando una **modulazione language-conditioned della dinamica con cui il Transformer costruisce il contenuto**.

Questa, tra le possibili versioni LLM di Sapir–Whorf, è quella che secondo me merita di essere perseguita più seriamente.

## You

Puoi aiutarmi a migliorare la scrittura di questa parte in latex? Per me è veramente troppo concisa.
non si capisce cosa siano e come si calcolino le quantità C

\---

```
latex
We write $\langle\cdot\rangle$ for the mask-weighted averaging over stated indices. 
The global mean over model layers and latent dimension result in $C_{kd} = \langle H_{slrpkd} \rangle_{s,l,r,p}$
The marginals over fact and languages are $F_{skd} = \langle H\rangle_{l,r,p} - C_{kd}$ and $L_{lkd} = \langle H\rangle_{s,r,p} - C_{kd}$ respectively.
A cell, which is a combination of fact and language, has a mean $\bar{H}_{slkd} = \langle H\rangle_{r,p}$.
To compute activation vector norms we denote token norms as the contraction $N_{slrpk} = \sqrt{H_{slrpkd}H_{slrpk}^d}$.
We separate radial and angular components obtaining the unit length tensor $U_{slrpkd} = H_{slrpkd}/N_{slrpk}$ for the angular structure, where $\|U_{slrpkd}\|_2=1$.
Sentence representations $p$ are specific reductions on the position axis.
We call the \emph{anchor} $a_{slrk} = H_{slrt^\star(s,l,r)k\cdot}$ as the operation that selects the last valid token $t^\star$ in the sentence, while the \emph{span mean} averages valid tokens $p \ge 1$.
For the span mean, we exclude the first token $p=0$, as it behaves like attention sink, distorting the interpretations.

There are multiple Gram matrices that can be obtained from $H$ and $M$.
First, cosine geometry uses the sentence Gram matrix over the $N$ normalized items.
Driven by the factorial experimental design, we name 4 fact--language pairs, and study the grand averages of relative Grams into 4 distinct factorial classes: $SS$ (same fact, same language), $SD$ (same fact, different language), $DS$, and $DD$, over with $r{\ne}q$ (different paraphrases) and the diagonal excluded.  

Class means $C_{\mathrm{SS}}(k)$ etc.\ yield the layer dependent \emph{contrasts}:
\begin{equation}
\Delta^F = C_{\mathrm{SS}} - C_{\mathrm{DS}}, \quad
\Delta^L = C_{\mathrm{SS}} - C_{\mathrm{SD}}, \quad
\Delta^{\mathrm{int}} = C_{\mathrm{SS}} - C_{\mathrm{SD}} - C_{\mathrm{DS}} + C_{\mathrm{DD}},
\label{eq:contrasts}
\end{equation}
measuring fact effect, language effect, and non-additivity per layer.
ANOVA-style Type-I decompositions partition sums of squares sequentially: $C \to P \to X$ at token level ($P$ position, $X$ sentence identity) and $C \to F \to L \to I$ at sentence level ($I$ the $F{\times}L$ interaction), on raw and unit-normalized inputs.
```

## ChatGPT

Sì. La parte che hai scritto contiene già quasi tutti gli oggetti necessari, ma presenta tre problemi: introduce alcuni simboli senza definirne precisamente dominio e assi; usa \(C\) sia per la componente comune sia, successivamente, per le classi di cosine; e soprattutto salta troppo rapidamente dal Gram tensor alle quantità \(C_{\mathrm{SS}},C_{\mathrm{SD}},C_{\mathrm{DS}},C_{\mathrm{DD}}\).

Separerei quindi nettamente le definizioni in: tensore e maschera, marginalizzazioni, rappresentazioni di frase, norma/normalizzazione, Gram tensor, classi factoriali e infine contrasti. Inoltre eviterei \(C\) per la componente comune e la chiamerei, ad esempio, \(\mu\), così \(C_{\mathrm{SS}}\) può essere riservato alla similarità.

Ti propongo questa versione, già in una forma abbastanza vicina a quella che metterei nel paper.

:::writing{variant="document" id="41736" title="Tensorial representation and cosine geometry — revised LaTeX"}
We represent the internal states of a model by a tensor
\begin{equation}
H_{slrpkd}
\in
\mathbb{R}^{S\times L\times R\times P\times K\times D},
\label{eq:H}
\end{equation}
where $s$ indexes facts, $l$ languages, $r$ paraphrastic realizations of a fact, $p$ token positions, $k$ Transformer layers, and $d$ latent dimensions. The corresponding validity mask is
\begin{equation}
M_{slrp}\in\{0,1\},
\label{eq:M}
\end{equation}
with $M_{slrp}=1$ for a valid token position and $M_{slrp}=0$ for zero-padded positions. All reductions over the token axis are therefore mask-weighted. Throughout, repeated latent indices are summed according to Einstein's convention, whereas indices written explicitly as free indices remain in the resulting tensor. We use the symbol ``$:$'' when a complete axis is retained as a vector, e.g. $H_{slrpk:}\in\mathbb{R}^{D}$.

For any tensor quantity $X_{slrpkd}$, we write
\begin{equation}
\left\langle X_{slrpkd}\right\rangle_{s,l,r,p}
=
\frac{
\sum_{s,l,r,p} M_{slrp}X_{slrpkd}
}{
\sum_{s,l,r,p} M_{slrp}
},
\label{eq:maskedmean}
\end{equation}
for the mask-weighted average over the indicated indices. Analogous notation is used for partial reductions, with the mask included whenever the position index $p$ is contracted. This convention makes explicit that zero-padded positions contribute neither to numerators nor denominators.

At each layer $k$, the global mean residual state is
\begin{equation}
\mu_{kd}
=
\left\langle H_{slrpkd}\right\rangle_{s,l,r,p},
\label{eq:globalmean}
\end{equation}
which retains the layer index $k$ and latent dimension $d$ and therefore has shape $(K,D)$. We then define the fact and language marginals by subtracting this global component:
\begin{align}
F_{skd}
&=
\left\langle H_{slrpkd}\right\rangle_{l,r,p}
-
\mu_{kd},
\label{eq:factmarginal}
\\
L_{lkd}
&=
\left\langle H_{slrpkd}\right\rangle_{s,r,p}
-
\mu_{kd}.
\label{eq:languagemarginal}
\end{align}
The corresponding fact--language cell mean is
\begin{equation}
\bar H_{slkd}
=
\left\langle H_{slrpkd}\right\rangle_{r,p},
\label{eq:cellmean}
\end{equation}
which represents the average state associated with fact $s$ and language $l$, while marginalizing over the available paraphrases and valid token positions.

We distinguish the radial magnitude of an activation from its angular direction. The squared norm of a token-level residual state is defined by the contraction
\begin{equation}
N_{slrpk}^{2}
=
H_{slrpkd}H_{slrpkd},
\label{eq:norm2}
\end{equation}
and hence
\begin{equation}
N_{slrpk}
=
\sqrt{
H_{slrpkd}H_{slrpkd}
}.
\label{eq:norm}
\end{equation}
The corresponding unit-normalized state is
\begin{equation}
U_{slrpkd}
=
\frac{H_{slrpkd}}
{N_{slrpk}},
\qquad
U_{slrpkd}U_{slrpkd}=1.
\label{eq:unit}
\end{equation}
The tensor $H$ therefore retains both radial and angular information, whereas $U$ is used to isolate angular geometry independently of residual-stream norm growth.

Sentence-level representations are obtained by explicit reductions over the position axis rather than by assuming a fixed sequence length. We consider two representations. First, the \emph{anchor} selects the last valid token in each sentence. Let
\begin{equation}
p^\star(s,l,r)
=
\max\{p:M_{slrp}=1\}
\label{eq:anchorposition}
\end{equation}
denote the last valid position. The corresponding anchor representation is
\begin{equation}
A_{slrkd}
=
H_{slr\,p^\star(s,l,r)\,kd}.
\label{eq:anchor}
\end{equation}
Second, the \emph{span mean} averages the valid token states over a prescribed span. In the present experiments we exclude the first token, $p=0$, and define
\begin{equation}
\widetilde M_{slrp}
=
M_{slrp}\mathbf{1}[p\geq1],
\label{eq:spanmask}
\end{equation}
followed by
\begin{equation}
S_{slrkd}
=
\frac{
\widetilde M_{slrp}H_{slrpkd}
}{
\widetilde M_{slrp}
}.
\label{eq:spanmean}
\end{equation}
The span mean and anchor are therefore two alternative sentence-level reductions of the same token-level tensor $H$; neither replaces the original token-level representation.

We next define the pairwise geometry directly from $H$. For two token-level states, indexed by $(s,l,r,p)$ and $(s',l',r',p')$, the raw Gram tensor is
\begin{equation}
G_{slrp\,s'l'r'p'}^{k}
=
H_{slrpkd}H_{s'l'r'p'kd}.
\label{eq:gram}
\end{equation}
The latent index $d$ is contracted, while the fact, language, paraphrase, position, and layer indices of both states remain explicit. The corresponding cosine tensor is obtained from the normalized states:
\begin{equation}
C_{slrp\,s'l'r'p'}^{k}
=
U_{slrpkd}U_{s'l'r'p'kd}.
\label{eq:cosine}
\end{equation}
Because each $U_{slrpk:}$ has unit Euclidean norm, $C$ is the cosine similarity between the two corresponding residual-stream states.

For the factorial analysis, we classify pairs of states according to whether their fact and language indices are equal or different. Let
\begin{align}
\mathcal{I}_{\mathrm{SS}}
&=
\{(s,l,s',l'):s=s',\,l=l'\},
\\
\mathcal{I}_{\mathrm{SD}}
&=
\{(s,l,s',l'):s=s',\,l\neq l'\},
\\
\mathcal{I}_{\mathrm{DS}}
&=
\{(s,l,s',l'):s\neq s',\,l=l'\},
\\
\mathcal{I}_{\mathrm{DD}}
&=
\{(s,l,s',l'):s\neq s',\,l\neq l'\}.
\end{align}
The first letter refers to the fact relation and the second to the language relation. To isolate semantic invariance across paraphrases, the main analysis considers pairs with $r\neq r'$; the exact diagonal $r=r'$ is excluded so that the result is not dominated by comparing a representation with itself.

For each layer $k$ and each factorial class $XY\in\{\mathrm{SS},\mathrm{SD},\mathrm{DS},\mathrm{DD}\}$, we define the corresponding class-mean cosine as
\begin{equation}
C_{XY}(k)
=
\frac{1}{|\mathcal{P}_{XY}|}
\sum_{(s,l,r,p;\,s',l',r',p')\in\mathcal{P}_{XY}}
C_{slrp\,s'l'r'p'}^{k},
\label{eq:classmean}
\end{equation}
where $\mathcal{P}_{XY}$ denotes the set of valid state pairs satisfying the corresponding fact--language relation, the paraphrase constraint $r\neq r'$, and any additional position-validity requirements. Thus, $C_{\mathrm{SS}}(k)$ is the average cosine between different paraphrases of the same fact expressed in the same language; $C_{\mathrm{SD}}(k)$ is the average cosine between different paraphrases of the same fact expressed in different languages; $C_{\mathrm{DS}}(k)$ compares different facts expressed in the same language; and $C_{\mathrm{DD}}(k)$ compares different facts expressed in different languages.

These four quantities provide a direct factorial decomposition of the observed angular geometry. We define the layer-dependent fact and language contrasts as
\begin{equation}
\Delta^{F}(k)
=
C_{\mathrm{SS}}(k)-C_{\mathrm{DS}}(k),
\qquad
\Delta^{L}(k)
=
C_{\mathrm{SS}}(k)-C_{\mathrm{SD}}(k),
\label{eq:contrasts}
\end{equation}
where $\Delta^{F}(k)$ measures the loss of similarity induced by changing the fact while holding language fixed, whereas $\Delta^{L}(k)$ measures the loss induced by changing the language while holding the fact fixed.

We additionally define the fact$\times$language interaction contrast
\begin{equation}
\Delta^{\mathrm{int}}(k)
=
C_{\mathrm{SS}}(k)
-
C_{\mathrm{SD}}(k)
-
C_{\mathrm{DS}}(k)
+
C_{\mathrm{DD}}(k).
\label{eq:interactioncontrast}
\end{equation}
Under a purely additive fact--language geometry, the corresponding interaction contrast is expected to be close to zero. A systematic deviation from zero indicates that the effect of changing the language depends on which fact is being represented, or equivalently that a fact--language interaction term is required to explain the observed geometry.

All quantities above can be computed either on the raw tensor $H$, thereby retaining radial and angular information, or on the normalized tensor $U$, thereby isolating angular structure. In the latter case, the class means $C_{\mathrm{SS}}(k)$, $C_{\mathrm{SD}}(k)$, $C_{\mathrm{DS}}(k)$, and $C_{\mathrm{DD}}(k)$ are bounded in $[-1,1]$ and directly quantify layer-dependent cosine geometry.
:::

Questa versione chiarisce soprattutto una cosa che nel testo originale rimaneva implicita: **\(C_{\mathrm{SS}}\) non è una contrazione elementare di \(H\), ma è una media di elementi di un Gram tensor già ottenuto contraendo l'indice latente \(d\), ulteriormente selezionati da una relazione combinatoria sugli indici \(s,l,r\)**.

C'è anche un punto matematico che correggerei rispetto alla formulazione originale: avevi scritto "The global mean over model layers and latent dimension", ma non vogliamo mediare né \(k\) né \(d\). La quantità comune è \(\mu_{kd}\), quindi gli assi \(k,d\) sono liberi. Inoltre eviterei la frase "Sentence representations \(p\) are specific reductions": \(p\) è l'indice di posizione, non la rappresentazione della frase.

Infine, per il paper suggerirei di chiamare la prima quantità \(\mu_{kd}\), non \(C_{kd}\). Così avrai una nomenclatura pulita:

\[
\mu=\text{global/common component},
\]

\[
F,L=\text{marginal effects},
\]

\[
G=\text{raw Gram tensor},
\]

\[
C=\text{cosine tensor / class cosine},
\]

\[
\Delta^F,\Delta^L,\Delta^{\mathrm{int}}
=\text{contrasts}.
\]

La distinzione diventa particolarmente importante quando arriveremo al modello conico, perché lì vorremo usare \(C\) liberamente per gli osservabili di cosine geometry senza confonderlo con la componente comune del residual stream.
