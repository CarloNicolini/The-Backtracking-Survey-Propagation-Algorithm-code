In breve: abbiamo trovato un miglioramento reale e motivato fisicamente, ma non parlerei ancora di “breakthrough pubblicabile”. Abbiamo una buona ipotesi, un meccanismo validato e risultati promettenti. Manca ancora la dimostrazione che sposti davvero la soglia algoritmica \(\alpha_a\).

## Che cosa non funzionava

Nel BSP originale, le survey vengono ricalcolate soltanto per le variabili libere. Una variabile fissata conserva quindi il valore `_sC` che aveva quando fu fissata.

Quando il codice esegue il backtracking, ordina le variabili fissate usando questi valori vecchi. Confronta quindi quantità calcolate su formule residue diverse. In pratica, `_sC` diventa in parte una misura dell’età della scelta, non della sua correttezza attuale.

Parisi propone invece una quantità diversa:

\[
I(k)=\text{frazione dei cluster attuali compatibili con il valore assegnato a }k.
\]

Se \(I(k)\) è piccolo, liberare \(k\) dovrebbe aumentare il numero di cluster di un fattore circa \(1/I(k)\), quindi:

\[
\Delta\Sigma_{\text{release}}\simeq-\log I(k).
\]

## La modifica vincente

Ho ricostruito le survey “virtuali” di ogni variabile fissata usando i messaggi correnti dei suoi vicini, senza liberarla realmente e senza eseguire un’altra SP.

Se \(k\) è fissata a vero:

\[
I(k)=1-s_F(k),
\]

mentre, se è fissata a falso:

\[
I(k)=1-s_T(k).
\]

Il nuovo backtracking libera la variabile con il più piccolo \(I(k)\) attuale. Il resto del BSP — frequenza del backtracking, decimazione e direzione — rimane invariato.

La modalità è disponibile con:

```bash
--dynamic-i-backtrack
```

## Verifica fondamentale

Il nuovo stimatore di \(I(k)\) è stato confrontato con 120 esperimenti costosi:

1. liberazione reale della variabile;
2. riconvergenza completa di SP;
3. misura della variazione di \(\Sigma\).

Risultato:

- correlazione tra guadagno previsto e reale: \(r=0.99684\);
- 120 segni corretti su 120;
- errore medio assoluto: 0.00056.

Questa è probabilmente la parte scientificamente più forte del lavoro: la quantità locale ricostruita misura quasi esattamente il guadagno globale dovuto alla liberazione.

## Risultati alla scala corretta

Per K=3, N=10000, \(\alpha=4.2\), cinque seed:

- dynamic-\(I\): 5/5 SAT;
- certainty: 4/5;
- polarization: 5/5.

Rispetto a certainty:

- discesa media di \(\Sigma\): −26.0%;
- roughness a parità di variabili fissate: −17.7%;
- massimo drop a profondità fissata: −17.9%;
- tempo: +5.8%.

Per K=4, N=1000, \(\alpha=9.5\), dieci seed:

- dynamic-\(I\): 6/10 SAT;
- certainty: 5/10;
- polarization: 6/10.

Rispetto a certainty:

- \(|\Delta\Sigma|\) medio: −8.6%;
- discesa media: −23.2%;
- massimo drop a profondità fissata: −39.8%;
- roughness: −4.8%;
- tempo: 1.94×.

Tutte le undici soluzioni dynamic-\(I\) sono state verificate clausola per clausola.

## L’ipotesi di Parisi era vera?

La risposta corretta è: sì nella sua forma specifica, non ancora nella forma forte.

È fortemente supportata questa idea:

> mantenere molti cluster disponibili a ogni profondità, liberando le assegnazioni che oggi ne distruggono di più, migliora il BSP.

Il successo di \(I(k)\), la discesa più lenta di \(\Sigma\) e la maggiore frequenza di soluzione vanno tutti nella stessa direzione.

Non abbiamo invece dimostrato:

> basta rendere \(\Sigma(t)\) più piatta per avvicinare necessariamente \(\alpha_a\) alla soglia SAT–UNSAT.

Abbiamo trovato diversi controesempi:

- il lookahead che massimizzava direttamente \(\Sigma_{\text{after}}\) era costosissimo e quasi inutile;
- alcuni metodi riducevano il massimo drop ma diminuivano il successo;
- a K=4, \(\alpha=9.7\), dynamic-\(I\) produceva ancora la curva più regolare, ma tutti i metodi fallivano 0/5.

Quindi una discesa lenta sembra essere una condizione favorevole, non sufficiente. Conta anche restare su un punto fisso SP stabile e scegliere assegnazioni che non conducano a futuri spinodal.

## È già pubblicabile?

Non ancora come breakthrough. Parisi aveva già formulato concettualmente \(I(k)\); la possibile novità è la ricostruzione operativa corrente, la sua validazione quasi esatta e l’evidenza algoritmica.

Per una rivendicazione pubblicabile servono ancora:

- almeno 30–100 seed per punto;
- una griglia di \(\alpha\) vicina alla soglia;
- stima di \(\alpha_a\) con intervalli di confidenza;
- test a più valori di N;
- confronto con il BSP del 2016 e verifica della novità bibliografica;
- conferma che il vantaggio persista su formule tenute completamente fuori dallo sviluppo.

Oggi possiamo sostenere: “dynamic-\(I\) migliora significativamente la dinamica della complexity e mostra migliori risultati a \(\alpha\) fissato”. Non possiamo ancora sostenere: “abbiamo spostato la soglia algoritmica”.