# Project Eldoria

Prototype d'action-aventure à la troisième personne (Unreal Engine 5.8, C++), inspiré de l'exploration et de la physique de BOTW/TOTK — univers et assets originaux.

Tranche verticale actuelle : la jungle de Tharum, une divinité-éléphant colossale contrôlée par un mécanisme. Deux façons d'atteindre son dos (la tour en ruine ou l'aqueduc), une libération non violente, la capacité FORCE en récompense, et un rocher autrefois immobile à déplacer.

## Ouvrir le projet
1. Unreal Engine 5.8 + Visual Studio 2022 (C++ desktop, workload Unreal).
2. `git lfs install`, puis cloner.
3. Clic droit sur `ProjectEldoria.uproject` → *Generate Visual Studio project files*, puis compiler `ProjectEldoriaEditor Win64 Development` (ou ouvrir le .uproject et accepter la compilation).
4. Niveau principal : `Content/Project/Levels/Lvl_TharumJungle`.

## Contrôles
ZQSD / WASD déplacement · Maj sprint · Espace saut · E interagir (maintenir pour pousser/porter) · clic gauche lancer · F FORCE (après Tharum).

## Commandes de debug (console, en jeu)
`EldoriaTeleport X Y Z Yaw` (mètres) · `EldoriaAdvanceDevice` · `EldoriaHeavyStun` · `EldoriaDeityState N` · `EldoriaUseAbility` · `EldoriaSetStrength N` · `EldoriaCollapseAll`

## Arborescence
- `Source/ProjectEldoria` — tout le gameplay en C++ (personnage, interactions, physique, divinité/IA, mécanisme, rencontre, capacités, HUD).
- `SourceArt/Scripts` — scripts Python (éditeur Unreal et Blender) qui génèrent le terrain, la végétation, les matériaux et les modèles de Tharum.
- `SourceArt/Generated` — sorties générées (terrain, modèles Blender en glTF).

## Crédits art
Packs CC0 de [Quaternius](https://quaternius.com) (Stylized Nature MegaKit, Medieval Village, Fantasy Props). Seuls les assets utilisés sont versionnés ; les archives d'origine (`SourceArt/Downloads`) ne le sont pas.
