# Installare Proximity Effect

## Windows (64 bit)

1. Estrai `ProximityEffect-Windows-x64.zip`.
2. Copia la cartella **`Proximity Effect.vst3`** in `C:\Program Files\Common Files\VST3\` (servono i permessi di amministratore).
3. Apri la DAW e fai una nuova scansione dei plugin.

`Proximity Effect.exe` è la versione standalone: si avvia senza DAW e usa la scheda audio scelta nelle impostazioni (icona ingranaggio). Al primo avvio Windows SmartScreen può mostrare "PC protetto": clicca **Ulteriori informazioni → Esegui comunque**. Il programma non è firmato digitalmente.

Non serve installare il Visual C++ Redistributable: il runtime è incluso nel plugin.

## macOS (Apple Silicon e Intel)

Il pacchetto `ProximityEffect-macOS-Universal.zip` contiene binari universali (arm64 + x86_64).

1. Estrai lo zip.
2. Copia:
   - **`Proximity Effect.vst3`** in `~/Library/Audio/Plug-Ins/VST3/`
   - **`Proximity Effect.component`** (Audio Unit) in `~/Library/Audio/Plug-Ins/Components/`
   - **`Proximity Effect.app`** (standalone) in `Applicazioni`, se ti serve
3. I file non sono firmati con un certificato Apple Developer, quindi macOS li blocca. Apri il Terminale ed esegui:

   ```sh
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Proximity Effect.vst3"
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"Proximity Effect.component"
   xattr -dr com.apple.quarantine /Applications/"Proximity Effect.app"
   ```

4. Riavvia la DAW (per gli AU in Logic a volte serve anche un riavvio del Mac o il comando `killall -9 AudioComponentRegistrar`).

## FL Studio (Windows e macOS)

1. Installa il VST3 come sopra.
2. In FL Studio: **Options → Manage plugins**.
3. Verifica che `C:\Program Files\Common Files\VST3` (Windows) o `~/Library/Audio/Plug-Ins/VST3` (macOS) sia tra le cartelle di ricerca, poi clicca **Find plugins** (attiva "Verify plugins").
4. Cerca "Proximity Effect" nella lista, metti la stellina per averlo nel browser dei plugin preferiti.
5. Nel Mixer seleziona una traccia, clicca su uno slot vuoto e scegli **Proximity Effect**.

## Altre DAW

| DAW | Formato | Dove trovarlo |
|---|---|---|
| Ableton Live | VST3 / AU | Preferenze → Plug-Ins: attiva "Use VST3 Plug-In System Folders", poi "Rescan". Lo trovi in Plug-Ins → Sambito. |
| Logic Pro | AU | Plug-in Manager (Logic Pro → Impostazioni → Plug-in Manager), verifica che sia "successfully validated". Lo trovi in Audio Units → Sambito. |
| Reaper | VST3 / AU | Preferences → Plug-ins → VST → "Re-scan". |
| Cubase / Nuendo | VST3 | Studio → VST Plug-in Manager → "Rescan". |
| Studio One | VST3 / AU | Options → Locations → VST Plug-ins → "Reset Blacklist" e riavvio. |
| Pro Tools | non supportato | Servirebbe il formato AAX, che richiede un SDK e una firma Avid. |

## Uso rapido

- Trascina il personaggio sulla sedia verso il microfono: più è vicino, più bassi, volume e calore. A 100 cm il suono è inalterato.
- **Polar Pattern** decide quanto è forte l'effetto: Omni nessuno, Cardioid circa +5 dB di bassi, Figure 8 circa +10 dB (a 0 cm con Low-End Push a 0).
- **Ego Killer -20 dB** è un pad in ingresso, come su un microfono vero: abbassa il segnale prima della saturazione. Se il livello d'uscita cala, alza Output.
- Il plugin aggiunge una piccola latenza (circa 61 campioni a 48 kHz, per l'oversampling). Le DAW la compensano automaticamente.
- Doppio clic sul personaggio per riportarlo alla distanza di default (60 cm).
