# Modernisation 2026 — dema-rc / SkyController 2

## Périmètre et versions

- Fork `didlawowo/dema-rc` : ancien `master` au commit `63e9471b912eb7958b31ac83283abf90c46e79cc` (2020).
- Base de cette branche : dernier upstream connu `ArduPilot/dema-rc@198661a7d9495fcdfa3284c914c0ef25216d700d` (2023), soit les 12 commits manquants intégrés par ascendance Git, **sans mise à jour de master**.
- L'upstream n'ayant pas de nouveaux commits après 2023 lors de l'audit, « moderne » signifie maintenir et tester le fork en 2026, **pas** présenter l'ancien logiciel comme un projet activement maintenu.
- Langage C, Meson >= 0.57, sous-modules c-ini et c-stdaux ; SDK historique Buildroot 2021.02.1 / Linaro GCC 7.3.1 pour ARMv7.

## Architecture confirmée par le code

1. `controller.c` lit les axes et boutons Linux `evdev`, avec une table de correspondance **spécifique SkyController 2** : cinq axes, onze boutons.
2. Les axes sont convertis en commandes PWM, et les appuis de bouton basculent la valeur de leur canal entre 1000 et 2000. La sortie est prévue à 10 ms d'intervalle via `timerfd` (`REMOTE_UPDATE_INTERVAL`), pas une fréquence physique mesurée.
3. `remote.c` sérialise seize canaux sur **UDP 777 par défaut**, avec version 3, timestamp et séquence en format RCINPUT_UDP ; un mode SITL est aussi présent.
4. Ce programme **n'est ni un pilote automatique, ni une passerelle de télémétrie MAVLink, ni un récepteur vidéo**. Failsafe et RTH doivent être explicitement qualifiés dans ArduPlane et le chemin radio complet.

## Anomalies et décisions

| Priorité | Élément | Observation dans le code | Traitement |
|---|---|---|---|
| P0 | Quantification des axes RC | Division entière `1000 / (rmax-rmin)` avant multiplication ; toute plage > 1000 donne une pente nulle | **Corrigé** avec calcul 64 bits, saturation et tests |
| P0 | Plage `evdev` dégénérée | La plage nulle provoquait une division par zéro | **Corrigé** : rejet au démarrage et garde défensive |
| P0 | Sécurité perte de liaison | `remote.c` ignore certains échecs UDP ; aucun déclenchement failsafe/RTH démontré dans ce dépôt | **BLOQUÉ — banc matériel / SITL** |
| P1 | Destination UDP | `remote_init` utilise `inet_addr` et convertit le port avec `htons` sans borne 1–65535 explicite | À durcir avec tests réseau dédiés |
| P1 | Entrée matérielle | `ioctl` des caractéristiques et lecture `evdev` n'ont pas de banc de tests simulés | À couvrir par tests d'événements `uinput` ou mocks |
| P1 | Boucle d'événements | En dehors de `EINTR`, les erreurs `epoll_wait` ne produisent pas d'échec explicite | À qualifier avant modification |
| P1 | Format réseau | Les structures UDP sont packed et documentées little-endian ; la compatibilité exacte avec RCInput ArduPlane récent doit être revalidée | Test protocolaire demandé |
| P2 | Build historique | Debian Buster et miroirs Buildroot périmés, exécution ARM64 Docker non adaptée à Linaro x86_64 | Dockerfile corrigé pour archives/HTTPS et tâches `--platform linux/amd64` |
| P2 | Workflow GitHub | `checkout@v2`, `setup-python@v1`, aucune vraie assertion sur les commandes RC | CI native modernisée et tests C ajoutés |

## Commandes de validation

Les tâches sont **non intrusives** ; aucune ne communique avec le SkyController, ne flashe, n'arme ou ne démarre un drone.

```bash
git submodule update --init --recursive
task check          # tests C indépendants : GCC/Clang
task native         # build Meson natif
task builder        # image Docker Buildroot x86_64 (construction longue)
task arm            # ARMv7 sur le SDK Buildroot
task nvidia:arm     # variante distante via SSH et copie du binaire sur l'hôte local
```

L'exécutable attendu en build croisé est `.build/armv7/src/dema-rc` ; contrôler l'ELF, l'interpréteur dynamique et la SHA256. Les builds natif et ARMv7 ne prouvent pas la compatibilité matérielle ni la sécurité en vol.

### Validation effective au 21/10/2026

- **PASS** : compilation et exécution des tests C isolés de `rc_scale_axis` avec `cc -std=gnu11 -Wall -Wextra -Werror` (bornes, plage nulle, monotonie, mi-course, entrée 32 bits).
- **PASS antérieur, hors de cette branche** : `dema-rc` upstream `198661a` a produit un binaire ARMv7 dans le projet Discovery, SHA256 `6705025c3af94278e0ef972ccf8323d4dc75ade3bfc2781d45fcf6994ea72572`. **Cela ne valide pas le nouveau binaire issu de ce fork modernisé.**
- **NON TESTÉ** : build Meson complet et ARMv7 de cette branche, démarrage sur SkyController 2, mapping physique des axes et boutons, perte radio, failsafe/RTH, compatibilité avec ArduPlane actuel.

## Conditions avant essais réels

Comparer les séquences UDP, commandes de boutons et plages PWM à la version validée, avec joystick entièrement parcouru et hélice retirée. Qualifier la perte de liaison et le mode de vol avant toute installation ou vol. Relever les dépendances dynamiques réellement présentes sur SkyController 2 ; ne jamais présumer qu'un ELF ARMv7 compilé suffit.
