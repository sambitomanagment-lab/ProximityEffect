# Proximity Effect: stato del lavoro

## 1. Progetto e percorsi

- **Nome:** Proximity Effect (plugin audio VST3 / AU / Standalone, JUCE 8.0.4, C++17, CMake)
- **Copia locale persistente (cartella condivisa del progetto Claude):** `/mnt/project-files/proximity-effect/ProximityEffect/`
- **Copia di lavoro della sessione cloud (temporanea):** `/home/claude/ProximityEffect/`
- **Copia su GitHub (temporanea):** repository privato `sambitomanagment-lab/PRODUCER-OS`, branch `proximity-effect`, cartella `proximity-effect/`. Vedi §9.

## 2. Ultimo aggiornamento

2026-10-22 09:40 UTC (risultati CI aggiunti alle 09:37)

## 3. Stato generale

Codice, GUI, DSP, test automatici, script di build/packaging e workflow CI completi.
Il personaggio ora è l'artwork originale del mockup, animato (sessione 3, §6).
Verificato su Linux x64 (build, 173 test, pluginval) e su GitHub Actions con la versione attuale (run 39754412189, commit eaab638): Windows x64 e macOS universale (arm64 + x86_64) compilati, testati, validati (pluginval; auval su macOS) e impacchettati in zip.

## 4. Funzionalità presenti e verificate

| Funzione | Verifica |
|---|---|
| Personaggio = artwork originale del mockup (ritagliato, non ridisegnato) | Test: a riposo l'immagine coincide con l'originale; screenshot |
| Avvicinamento al microfono col parametro distanza (trascinamento, automazione, doppio clic = 60 cm) | Screenshot a 100 / 50 / 5 / 0 cm |
| Sedia che si comprime + leggera inclinazione verso il microfono | Test: scarpe e ruote restano sul pavimento, busto si abbassa; screenshot |
| Volto che si gonfia gradualmente (guance e mascella più della fronte) | Test: cambia solo la zona del viso; screenshot |
| Rossore progressivo su guance, naso, orecchie | Test: guancia più rossa, felpa invariata; screenshot |
| Espressione arrabbiata ottenuta deformando l'artwork (sopracciglia giù, occhi socchiusi, bocca a denti stretti) + vena comica | Test + screenshot |
| Tremolio controllato e vapore dalla testa agli estremi o con Angry | Screenshot (vapore); tremolio solo visto nel codice, vedi §5 |
| Transizioni morbide (easing 60–150 ms, timer GUI a 60 Hz) | Test: un passo del 2% cambia l'immagine circa la metà di un passo del 4% |
| Rendering abbastanza leggero | Test: circa 7 ms per fotogramma a piena risoluzione in questo ambiente Linux; a zoom 100% la GUI renderizza a metà risoluzione (circa 4 volte meno lavoro) |
| Pattern polari, Ego Killer -20 dB, Input/Output, Low-End, Gain Boost, Mix, HPF, Air, Polarity | Test DSP (Fig 8 +10,06 dB, Cardioid +5,12 dB, Omni +0,01 dB) |
| Bypass host allineato alla latenza (61 campioni a 48 kHz) | Test |
| Sample rate 22,05–192 kHz, buffer 1–2048, mono/stereo, automazione senza click, stato | Test |
| VST3 valido | pluginval strictness 10: SUCCESS (Linux; Windows al primo run CI) |

## 5. Funzionalità incomplete, non verificate, problemi visivi rimasti

- **Prova in DAW reale (FL Studio ecc.):** non fatta. In particolare non ho visto l'animazione dal vivo: tremolio e vapore sono verificati solo su screenshot statici e leggendo il codice.
- **Risoluzione dell'artwork:** il ritaglio dal mockup è 510×630 px. A zoom 100% è ridotto a metà (nitido); a zoom 200% su schermo normale o 100% su Retina è a dimensione nativa; oltre, leggermente morbido. Serve l'illustrazione originale ad alta risoluzione per andare oltre.
- **Fumetto:** con il personaggio vicino al microfono il fumetto copre la parte alta del microfono (come prima).
- **Vapore sinistro:** quando c'è il fumetto "#@%!!" lo sbuffo di sinistra resta in parte dietro il fumetto.
- **Bocca arrabbiata:** è il sorriso originale deformato (angoli in giù, denti stretti). Non è stata disegnata una bocca nuova, per non snaturare l'artwork.
- **Ritaglio:** resta un puntino color crema tra le due scarpe, invisibile sullo sfondo crema del plugin.
- Firma: macOS solo ad-hoc, Windows non firmato. AAX non previsto.

## 6. Modifiche effettuate

Sessione 1: progetto JUCE/CMake, DSP, GUI comica (personaggio vettoriale disegnato nel codice).

Sessione 2: calibrazione DSP, bypass con latenza compensata, runtime MSVC statico, 162 test automatici, script di build/packaging, docs/INSTALL.md (FL Studio e altre DAW), workflow GitHub Actions, questo file.

Sessione 3 (personaggio e reazioni visive; GUI, layout e DSP non toccati):
- `Assets/Layout-mockup.png`: copia intatta del mockup originale (fonte dell'artwork).
- `scripts/extract_character.py`: ritaglia il personaggio dal mockup togliendo solo lo sfondo (flood fill fermato dai contorni neri del disegno). Risultato `Assets/character.png` (510×700, con margine sopra la testa), incluso nel plugin come BinaryData.
- `Source/CharacterArtwork.h/.cpp` (nuovo): anima l'artwork deformandolo e colorandolo, senza ridisegnarlo: compressione della sedia, inclinazione, gonfiore del viso, rossore con maschera della pelle, espressione arrabbiata con punti di controllo sul viso originale. Cache: ridisegna solo quando la posa cambia, alla risoluzione effettiva sullo schermo.
- `Source/PluginEditor.h/.cpp` (ChairGuy): usa l'artwork; legge distanza, pattern, Low-End Push, Gain Boost e Angry e ne ricava le reazioni con transizioni morbide; tremolio con sinusoidi (niente salti casuali), vapore sfumato, vena comica che "spunta" solo a rabbia piena. Timer GUI da 30 a 60 Hz. Il personaggio vettoriale precedente resta nel codice come riserva se l'immagine non si carica.
- Area del componente del personaggio allargata (stesso binario e stesse posizioni sul righello); a 100 cm si ferma 13 px prima per non coprire le scritte del Mix.
- `Tests/ProximityEffectTests.cpp`: 11 nuovi controlli sull'artwork; screenshot a 100, 50, 5, 0 cm (manopole al massimo) e Angry.

Come le manopole guidano il personaggio (in `ChairGuy::tick`):
- vicinanza = curva della distanza usata dal DSP (0 a 100 cm);
- intensità = vicinanza × (0,85 + 0,15 × media di pattern, Low-End Push, Gain Boost);
- sedia e inclinazione seguono la vicinanza; gonfiore e rossore l'intensità;
- rabbia da intensità 0,75 a 0,92 (con le manopole di default arriva a circa metà a 5 cm, piena a 0 cm con manopole alte), oppure piena con Angry;
- tremolio e vapore da intensità 0,9 a 1, oppure con Angry.

## 7. Build e test eseguiti (risultati reali)

| Data | Ambiente | Comando | Risultato |
|---|---|---|---|
| 2026-10-22 | Linux x64, g++ 13 | build Release con `-DPE_BUILD_TESTS=ON` | OK |
| 2026-10-22 | Linux x64 | `ProximityEffectTests --snapshots …` | 173 controlli, 0 falliti (sessione 3) |
| 2026-10-22 | Linux x64 | `pluginval --strictness-level 10 --skip-gui-tests` sul VST3 | SUCCESS (sessione 3) |
| 2026-10-22 | GitHub Actions, windows-latest | workflow completo (run 39746711340) | OK: build, ctest, report, pluginval 10, zip (versione sessione 2) |
| 2026-10-22 | GitHub Actions, macos-latest | stesso run | build universale e ctest OK; fallito lo step "DSP report" per un errore nello script (corretto in 06accbd) |
| 2026-10-22 | GitHub Actions, Windows + macOS | run 39750468726 (06accbd, versione sessione 2) | tutto verde; auval: "AU VALIDATION SUCCEEDED"; lipo: x86_64 arm64 |
| 2026-10-22 | GitHub Actions, Windows + macOS | run 39754412189 (eaab638, nuovo personaggio) | tutto verde: build, ctest, report DSP, pluginval (Windows e macOS), auval, zip |

## 8. Problemi aperti e tentativi

- Nessun repository dedicato: soluzione temporanea concordata, branch `proximity-effect` di PRODUCER-OS (main non toccato).
- Runner macOS su repo privato: consumano minuti 10× (circa 25 minuti a run).
- CI macOS "DSP report": `ls` di un file .exe inesistente faceva fallire lo step; sostituito con un controllo esplicito.

## 9. GitHub

- Accesso push a `sambitomanagment-lab/PRODUCER-OS` verificato; creazione nuovi repository non disponibile da questa sessione.
- Workflow nel branch: `.github/workflows/proximity-effect.yml` (copia di `proximity-effect/.github/workflows/build.yml` con percorsi adattati), si attiva solo sul branch `proximity-effect` e sui tag `proximity-effect-v*`.
- Ultimo run verde con il personaggio nuovo: https://github.com/sambitomanagment-lab/PRODUCER-OS/actions/runs/39754412189 (artifact `ProximityEffect-Windows-x64` e `ProximityEffect-macOS-Universal`, conservati da GitHub per 90 giorni).
- Artifacts (zip Windows e macOS): scaricabili dalla pagina del run; da questa sessione il download è bloccato dal proxy di rete, quindi non sono copiati in locale.

## 10. Repository locale e remoto

- Locale: repo git in `ProximityEffect/` (branch `main`) e `git bundle` in `/mnt/project-files/proximity-effect/ProximityEffect.bundle`.
- Remoto: PRODUCER-OS, branch `proximity-effect`.

## 11. File generati e artefatti

- Sorgenti: `/mnt/project-files/proximity-effect/ProximityEffect/`
- Pacchetto Linux (test): `/mnt/project-files/proximity-effect/dist/ProximityEffect-Linux-x64.zip`
- Screenshot GUI: `/mnt/project-files/proximity-effect/screenshots/` (`ui-far`, `ui-mid`, `ui-close`, `ui-max`, `ui-angry`)
- Log: `/mnt/project-files/proximity-effect/logs/`
- Pacchetti Windows/macOS: Actions Artifacts del run su GitHub.

## 12. Ultima attività completata

Personaggio originale animato con tutte le reazioni richieste; test, screenshot e pluginval superati su Linux.

## 13. Prossima attività consigliata

1. Provare il plugin in FL Studio (zip dagli artifact del run CI) e guardare l'animazione dal vivo: trascinamento, automazione della distanza, Angry, manopole al massimo.
2. Se disponibile, sostituire `Assets/Layout-mockup.png` con l'illustrazione originale ad alta risoluzione (stessa inquadratura) e rilanciare `python3 scripts/extract_character.py`: i punti di riferimento del viso in `CharacterArtwork.cpp` vanno ricontrollati se cambia l'inquadratura.
3. Spostare il progetto nel repository dedicato e creare il tag `v1.0.0`.

## 14. Come riprendere il lavoro

- Build: `scripts\build.ps1` (Windows) o `scripts/build.sh` (macOS/Linux). Requisiti nel README.
- Ritaglio del personaggio: `python3 scripts/extract_character.py` (Python 3 con Pillow e NumPy).
- Screenshot: `build/ProximityEffectTests_artefacts/Release/ProximityEffectTests --snapshots cartella`.
- Tornare al personaggio vettoriale: basta che `Assets/character.png` non sia incluso (ChairGuy usa `paintVector`).
- Per spostare il progetto in un repo nuovo, dalla cartella `proximity-effect/`:
  ```sh
  git init && git add -A && git commit -m "Proximity Effect"
  git remote add origin https://github.com/<utente>/proximity-effect.git
  git push -u origin main
  ```
- In una nuova chat con Claude: indica questo file e il percorso del progetto.
